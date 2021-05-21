/* wl-clipboard
 *
 * Copyright © 2019 Sergey Bugaev <bugaevc@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "types/copy-action.h"
#include "types/source.h"
#include "types/device.h"
#include "types/device-manager.h"
#include "types/registry.h"
#include "types/popup-surface.h"

#include "util/files.h"
#include "util/string.h"
#include "util/misc.h"

#include <wayland-client.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h> // open
#include <libgen.h>
#include <getopt.h>
#include <signal.h>

static struct {
    int stay_in_foreground;
    int clear;
    char *mime_type;
    int trim_newline;
    int paste_once;
    int primary;
    const char *seat_name;
} options;

static void did_set_selection_callback(struct copy_action *copy_action) {
    if (options.clear) {
        exit(0);
    }

    if (!options.stay_in_foreground) {
        /* Move to background.
         * We fork our process and leave the
         * child running in the background,
         * while exiting in the parent.
         * Also replace stdin/stdout with
         * /dev/null so the stdout file
         * descriptor isn't kept alive.
         */
        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDIN_FILENO);
            close(devnull);
        } else {
            /* If we cannot open /dev/null, just close stdin/stdout */
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
        }
        signal(SIGHUP, SIG_IGN);
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            /* Proceed without forking */
        }
        if (pid > 0) {
            exit(0);
        }
    }
}

static void cleanup_and_exit(struct copy_action *copy_action, int code) {
    /* We're done copying!
     * All that's left to do now is to
     * clean up after ourselves and exit.*/
    char *temp_file = (char *) copy_action->file_to_copy;
    if (temp_file != NULL) {
        /* Clean up our temporary file */
        execlp("rm", "rm", "-r", dirname(temp_file), NULL);
        perror("exec rm");
        exit(1);
    } else {
        exit(code);
    }
}

static void cancelled_callback(struct copy_action *copy_action) {
    cleanup_and_exit(copy_action, 0);
}

static void pasted_callback(struct copy_action *copy_action) {
    if (options.paste_once) {
        cleanup_and_exit(copy_action, 0);
    }
}

static void print_usage(FILE *f, const char *argv0) {
    fprintf(
        f,
        "Usage:\n"
        "\t%s [options] text to copy\n"
        "\t%s [options] < file-to-copy\n\n"
        "Copy content to the Wayland clipboard.\n\n"
        "Options:\n"
        "\t-o, --paste-once\tOnly serve one paste request and then exit.\n"
        "\t-f, --foreground\tStay in the foreground instead of forking.\n"
        "\t-c, --clear\t\tInstead of copying anything, clear the clipboard.\n"
        "\t-p, --primary\t\tUse the \"primary\" clipboard.\n"
        "\t-n, --trim-newline\tDo not copy the trailing newline character.\n"
        "\t-t, --type mime/type\t"
        "Override the inferred MIME type for the content.\n"
        "\t-s, --seat seat-name\t"
        "Pick the seat to work with.\n"
        "\t-v, --version\t\tDisplay version info.\n"
        "\t-h, --help\t\tDisplay this message.\n"
        "Mandatory arguments to long options are mandatory"
        " for short options too.\n\n"
        "See wl-clipboard(1) for more details.\n",
        argv0,
        argv0
    );
}

static void parse_options(int argc, argv_t argv) {
    if (argc < 1) {
        bail("Empty argv");
    }

    static struct option long_options[] = {
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {"primary", no_argument, 0, 'p'},
        {"trim-newline", no_argument, 0, 'n'},
        {"paste-once", no_argument, 0, 'o'},
        {"foreground", no_argument, 0, 'f'},
        {"clear", no_argument, 0, 'c'},
        {"type", required_argument, 0, 't'},
        {"seat", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };
    while (1) {
        int option_index;
        const char *opts = "vhpnofct:s:";
        int c = getopt_long(argc, argv, opts, long_options, &option_index);
        if (c == -1) {
            break;
        }
        if (c == 0) {
            c = long_options[option_index].val;
        }
        switch (c) {
        case 'v':
            print_version_info();
            exit(0);
        case 'h':
            print_usage(stdout, argv[0]);
            exit(0);
        case 'p':
            options.primary = 1;
            break;
        case 'n':
            options.trim_newline = 1;
            break;
        case 'o':
            options.paste_once = 1;
            break;
        case 'f':
            options.stay_in_foreground = 1;
            break;
        case 'c':
            options.clear = 1;
            break;
        case 't':
            options.mime_type = strdup(optarg);
            break;
        case 's':
            options.seat_name = strdup(optarg);
            break;
        default:
            /* getopt has already printed an error message */
            print_usage(stderr, argv[0]);
            exit(1);
        }
    }
}

int main(int argc, argv_t argv) {
    parse_options(argc, argv);

    struct wl_display *wl_display = wl_display_connect(NULL);
    if (wl_display == NULL) {
        bail("Failed to connect to a Wayland server");
    }

    struct registry *registry = calloc(1, sizeof(struct registry));
    registry->wl_display = wl_display;
    registry_init(registry);

    /* Wait for the initial set of globals to appear */
    wl_display_roundtrip(wl_display);

    struct seat *seat = registry_find_seat(registry, options.seat_name);
    if (seat == NULL) {
        if (options.seat_name != NULL) {
            bail("No such seat");
        } else {
            bail("Missing a seat");
        }
    }

    /* Create the device */
    struct device_manager *device_manager
        = registry_find_device_manager(registry, options.primary);
    if (device_manager == NULL) {
        complain_about_selection_support(options.primary);
    }

    struct device *device = device_manager_get_device(device_manager, seat);

    if (!device_supports_selection(device, options.primary)) {
        complain_about_selection_support(options.primary);
    }

    /* Create and initialize the copy action */
    struct copy_action *copy_action = calloc(1, sizeof(struct copy_action));
    copy_action->device = device;
    copy_action->primary = options.primary;

    if (!options.clear) {
        if (optind < argc) {
            /* Copy our command-line arguments */
            copy_action->argv_to_copy = &argv[optind];
        } else {
            /* Copy data from our stdin.
             * It's important that we only do this
             * after going through the initial stages
             * that are likely to result in errors,
             * so that we don't forget to clean up
             * the temp file.
             */
            char *temp_file = dump_stdin_into_a_temp_file();
            if (options.trim_newline) {
                trim_trailing_newline(temp_file);
            }
            if (options.mime_type == NULL) {
                options.mime_type = infer_mime_type_from_contents(temp_file);
            }
            copy_action->file_to_copy = temp_file;
        }

        /* Create the source */
        copy_action->source = device_manager_create_source(device_manager);
        if (options.mime_type != NULL) {
            source_offer(copy_action->source, options.mime_type);
        }
        if (options.mime_type == NULL || mime_type_is_text(options.mime_type)) {
            /* Offer a few generic plain text formats */
            source_offer(copy_action->source, text_plain);
            source_offer(copy_action->source, text_plain_utf8);
            source_offer(copy_action->source, "TEXT");
            source_offer(copy_action->source, "STRING");
            source_offer(copy_action->source, "UTF8_STRING");
        }
        free(options.mime_type);
        options.mime_type = NULL;
    }

    if (device->needs_popup_surface) {
        copy_action->popup_surface = calloc(1, sizeof(struct popup_surface));
        copy_action->popup_surface->registry = registry;
        copy_action->popup_surface->seat = seat;
    }

    copy_action->did_set_selection_callback = did_set_selection_callback;
    copy_action->pasted_callback = pasted_callback;
    copy_action->cancelled_callback = cancelled_callback;
    copy_action_init(copy_action);

    while (wl_display_dispatch(wl_display) >= 0);

    perror("wl_display_dispatch");
    cleanup_and_exit(copy_action, 1);
    return 1;
}

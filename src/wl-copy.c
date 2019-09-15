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
#include <wayland-util.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <getopt.h>

static struct {
    int stay_in_foreground;
    int clear;
    char *mime_type;
    int trim_newline;
    int paste_once;
    int primary;
    int regular;
    struct wl_array seat_names;
} options;

static struct {
    int unset;
    int live;
} copy_actions_cnt;

static void did_set_selection_callback(struct copy_action *copy_action) {
    copy_actions_cnt.unset--;

    if (options.clear) {
        copy_actions_cnt.live--;
        if (copy_actions_cnt.live == 0) {
            exit(0);
        }
    }

    if (!options.stay_in_foreground && copy_actions_cnt.unset == 0) {
        /* Move to background.
         * We fork our process and leave the
         * child running in the background,
         * while exiting in the parent.
         */
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

static void cleanup_and_exit(struct copy_action *copy_action) {
    /* We're done copying!
     * All that's left to do now is to
     * clean up after ourselves and exit.*/
    char *temp_file = (char *) copy_action->copy_source.file_path;
    if (temp_file != NULL) {
        /* Clean up our temporary file */
        execlp("rm", "rm", "-r", dirname(temp_file), NULL);
        perror("exec rm");
        exit(1);
    } else {
        exit(0);
    }
}

static void cancelled_callback(struct copy_action *copy_action) {
    copy_actions_cnt.live--;
    if (copy_actions_cnt.live == 0) {
        cleanup_and_exit(copy_action);
    }
}

static void pasted_callback(struct copy_action *copy_action) {
    if (options.paste_once) {
        cleanup_and_exit(copy_action);
    }
}

static void set_up_selection_for_seat(
    struct wl_display *wl_display,
    struct registry *registry,
    struct seat *seat,
    struct copy_source copy_source,
    int primary
) {
    /* Create the device */
    struct device_manager *device_manager
        = registry_find_device_manager(registry, primary);
    if (device_manager == NULL) {
        complain_about_selection_support(primary);
    }

    struct device *device = device_manager_get_device(device_manager, seat);

    if (!device_supports_selection(device, primary)) {
        complain_about_selection_support(primary);
    }

    copy_actions_cnt.live++;
    copy_actions_cnt.unset++;

    /* Create and initialize the copy action */
    struct copy_action *copy_action = calloc(1, sizeof(struct copy_action));
    copy_action->device = device;
    copy_action->primary = primary;
    copy_action->copy_source = copy_source;

    /* Create the source */
    if (!options.clear) {
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
}

static void set_up_selection(
    struct wl_display *wl_display,
    struct registry *registry,
    struct copy_source copy_source,
    int primary
) {
    struct seat *seat = NULL;

    /* Go over the requested seat names */
    for (
        const char *seat_name = options.seat_names.data;
        seat_name != options.seat_names.data + options.seat_names.size;
        seat_name += strlen(seat_name) + 1
    ) {
        seat = registry_find_seat(registry, seat_name);
        if (seat == NULL) {
            bail("No such seat");
        }
        set_up_selection_for_seat(
            wl_display,
            registry,
            seat,
            copy_source,
            primary
        );
    }

    /* See if any seat was requested at all */
    if (seat == NULL) {
        /* No seat was explicitly requested.
         * Try to find any seat.
         */
        seat = registry_find_seat(registry, NULL);
        if (seat == NULL) {
            bail("Missing a seat");
        }
        set_up_selection_for_seat(
            wl_display,
            registry,
            seat,
            copy_source,
            primary
        );
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
        "\t-r, --regular\t\tUse the regular clipboard.\n"
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

    wl_array_init(&options.seat_names);

    static struct option long_options[] = {
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {"primary", no_argument, 0, 'p'},
        {"trim-newline", no_argument, 0, 'n'},
        {"paste-once", no_argument, 0, 'o'},
        {"foreground", no_argument, 0, 'f'},
        {"clear", no_argument, 0, 'c'},
        {"regular", no_argument, 0, 'r'},
        {"type", required_argument, 0, 't'},
        {"seat", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };
    while (1) {
        int option_index;
        const char *opts = "vhpnofcrt:s:";
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
        case 'r':
            options.regular = 1;
            break;
        case 't':
            options.mime_type = strdup(optarg);
            break;
        case 's':;
            char *ptr = wl_array_add(&options.seat_names, strlen(optarg) + 1);
            strcpy(ptr, optarg);
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

    struct copy_source copy_source = { 0 };

    if (!options.clear) {
        if (optind < argc) {
            /* Copy our command-line arguments */
            copy_source.argv = &argv[optind];
        } else {
            /* Copy data from our stdin */
            char *temp_file = dump_stdin_into_a_temp_file();
            if (options.trim_newline) {
                trim_trailing_newline(temp_file);
            }
            if (options.mime_type == NULL) {
                options.mime_type = infer_mime_type_from_contents(temp_file);
            }
            copy_source.file_path = temp_file;
        }
    }

    /* By default, or if the regular clipboard
     * is explicitly requested, use the regular
     * clipboard.
     */
    if (!options.primary || options.regular) {
        set_up_selection(wl_display, registry, copy_source, 0);
    }
    if (options.primary) {
        set_up_selection(wl_display, registry, copy_source, 1);
    }

    while (wl_display_dispatch(wl_display) >= 0);

    perror("wl_display_dispatch");
    return 1;
}

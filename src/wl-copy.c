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

#include "boilerplate.h"
#include "types/source.h"
#include "types/device.h"
#include "types/device-manager.h"

static struct {
    int stay_in_foreground;
    int clear;
    char *mime_type;
    int trim_newline;
    int paste_once;
    int primary;
} options;

static char * const *data_to_copy = NULL;
static char *temp_file_to_copy = NULL;

static struct device_manager *device_manager = NULL;
static struct device *device = NULL;
static struct source *source = NULL;

static void cancelled_callback(struct source *source) {
    /* We're done! */
    if (temp_file_to_copy != NULL) {
        execlp("rm", "rm", "-r", dirname(temp_file_to_copy), NULL);
        perror("exec rm");
        exit(1);
    } else {
        exit(0);
    }
}

static void send_callback(
    struct source *source,
    const char *mime_type,
    int fd
) {
    /* Unset O_NONBLOCK */
    fcntl(fd, F_SETFL, 0);
    if (data_to_copy != NULL) {
        /* Copy the specified data, separated by spaces */
        FILE *f = fdopen(fd, "w");
        if (f == NULL) {
            perror("fdopen");
            exit(1);
        }
        char * const *dataptr = data_to_copy;
        for (int is_first = 1; *dataptr != NULL; dataptr++, is_first = 0) {
            if (!is_first) {
                fwrite(" ", 1, 1, f);
            }
            fwrite(*dataptr, 1, strlen(*dataptr), f);
        }
        fclose(f);
    } else {
        /* Copy from the temp file; for that, we delegate to a
         * (hopefully) highly optimized implementation of copying.
         */
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(fd);
            return;
        }
        if (pid == 0) {
            dup2(fd, STDOUT_FILENO);
            execlp("cat", "cat", temp_file_to_copy, NULL);
            perror("exec cat");
            exit(1);
        }
        close(fd);
        wait(NULL);
    }

    if (options.paste_once) {
        cancelled_callback(source);
    }
}

static void set_selection(uint32_t serial) {
    device_set_selection(device, source, serial, options.primary);
    wl_display_roundtrip(display);
    destroy_popup_surface();
}

static void complain_about_missing_keyboard() {
    bail("Setting primary selection is not supported without a keyboard");
}

static void do_offer(char *mime_type, struct source *source) {
    if (mime_type == NULL || mime_type_is_text(mime_type)) {
        /* Offer a few generic plain text formats */
        source_offer(source, text_plain);
        source_offer(source, text_plain_utf8);
        source_offer(source, "TEXT");
        source_offer(source, "STRING");
        source_offer(source, "UTF8_STRING");
    }
    if (mime_type != NULL) {
        source_offer(source, mime_type);
    }
    free(mime_type);
}

static void init_device_manager() {
#ifdef HAVE_WLR_DATA_CONTROL
    if (data_control_manager != NULL) {
        device_manager->proxy = (struct wl_proxy *) data_control_manager;
        device_manager_init_zwlr_data_control_manager_v1(device_manager);
        return;
    }
#endif

    device_manager->proxy = (struct wl_proxy *) data_device_manager;
    device_manager_init_wl_data_device_manager(device_manager);
}

static void init_primary_device_manager() {
    ensure_has_primary_selection();

#ifdef HAVE_WP_PRIMARY_SELECTION
    if (primary_selection_device_manager != NULL) {
        device_manager->proxy
            = (struct wl_proxy *) primary_selection_device_manager;
        device_manager_init_zwp_primary_selection_device_manager_v1(
            device_manager
        );
        return;
    }
#endif

#ifdef HAVE_GTK_PRIMARY_SELECTION
    if (gtk_primary_selection_device_manager != NULL) {
        device_manager->proxy
            = (struct wl_proxy *) gtk_primary_selection_device_manager;
        device_manager_init_gtk_primary_selection_device_manager(
            device_manager
        );
        return;
    }
#endif
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

static void parse_options(int argc, char * const argv[]) {
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
            requested_seat_name = strdup(optarg);
            break;
        default:
            /* getopt has already printed an error message */
            print_usage(stderr, argv[0]);
            exit(1);
        }
    }
}

int main(int argc, char * const argv[]) {
    parse_options(argc, argv);

    init_wayland_globals();

    if (options.primary) {
        ensure_has_primary_selection();
    }

    if (!options.clear) {
        if (optind < argc) {
            /* Copy our command-line arguments */
            data_to_copy = &argv[optind];
        } else {
            /* Copy data from our stdin */
            temp_file_to_copy = dump_stdin_into_a_temp_file();
            if (options.trim_newline) {
                trim_trailing_newline(temp_file_to_copy);
            }
            if (options.mime_type == NULL) {
                options.mime_type
                    = infer_mime_type_from_contents(temp_file_to_copy);
            }
        }
    }

    if (!options.stay_in_foreground && !options.clear) {
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

    device_manager = calloc(1, sizeof(struct device_manager));
    if (!options.primary) {
        init_device_manager();
    } else {
        init_primary_device_manager();
    }

    device = device_manager_get_device(device_manager, seat);

    source = device_manager_create_source(device_manager);
    source->send_callback = send_callback;
    source->cancelled_callback = cancelled_callback;

    do_offer(options.mime_type, source);

    /* See if we can just set the selection directly */
    if (!device->needs_popup_surface) {
        /* If we can, it doesn't actually require
         * a serial, so passing zero will do.
         */
        device_set_selection(device, source, 0, options.primary);
    } else {
        /* If we cannot, schedule to do it later,
         * when our popup surface gains keyboard focus.
         */
        action_on_popup_surface_getting_focus = set_selection;
        action_on_no_keyboard = complain_about_missing_keyboard;
        popup_tiny_invisible_surface();
    }

    if (options.clear) {
        wl_display_roundtrip(display);
        exit(0);
    }

    while (wl_display_dispatch(display) >= 0);

    perror("wl_display_dispatch");
    return 1;
}

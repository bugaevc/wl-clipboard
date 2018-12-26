/* wl-clipboard
 *
 * Copyright © 2018 Sergey Bugaev <bugaevc@gmail.com>
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

char * const *data_to_copy = NULL;
char *temp_file_to_copy = NULL;
int paste_once = 0;

void do_send(const char *mime_type, int fd) {
    // unset O_NONBLOCK
    fcntl(fd, F_SETFL, 0);
    if (data_to_copy != NULL) {
        // copy the specified data, separated by spaces
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
        // copy from the temp file; for that, we delegate to a
        // (hopefully) highly optimized implementation of copying
        if (fork() == 0) {
            dup2(fd, STDOUT_FILENO);
            execlp("cat", "cat", temp_file_to_copy, NULL);
            perror("exec cat");
            exit(1);
        }
        close(fd);
        wait(NULL);
    }

    if (paste_once) {
        exit(0);
    }
}

void do_cancel() {
    // we're done!
    if (temp_file_to_copy != NULL) {
        execlp("rm", "rm", "-r", dirname(temp_file_to_copy), NULL);
        perror("exec rm");
        exit(1);
    } else {
        exit(0);
    }
}

void data_source_target_handler
(
    void *data,
    struct wl_data_source *data_source,
    const char *mime_type
) {}

void data_source_send_handler
(
    void *data,
    struct wl_data_source *data_source,
    const char *mime_type,
    int fd
) {
    do_send(mime_type, fd);
}

void data_source_cancelled_handler
(
    void *data,
    struct wl_data_source *data_source
) {
    do_cancel();
}

const struct wl_data_source_listener data_source_listener = {
    .target = data_source_target_handler,
    .send = data_source_send_handler,
    .cancelled = data_source_cancelled_handler
};

struct wl_data_source *data_source;

void set_data_selection(uint32_t serial) {
    wl_data_device_set_selection(data_device, data_source, serial);
    wl_display_roundtrip(display);
    destroy_popup_surface();
}

void try_setting_data_selection_directly() {
    set_data_selection(get_serial());
}

#ifdef HAVE_GTK_PRIMARY_SELECTION

void primary_selection_source_send_handler
(
    void *data,
    struct gtk_primary_selection_source *primary_selection_source,
    const char *mime_type,
    int fd
) {
    do_send(mime_type, fd);
}

void primary_selection_source_cancelled_handler
(
    void *data,
    struct gtk_primary_selection_source *primary_selection_source
) {
    do_cancel();
}

const struct gtk_primary_selection_source_listener
primary_selection_source_listener = {
    .send = primary_selection_source_send_handler,
    .cancelled = primary_selection_source_cancelled_handler
};

struct gtk_primary_selection_source *primary_selection_source;

void set_primary_selection(uint32_t serial) {

    gtk_primary_selection_device_set_selection(
        primary_selection_device,
        primary_selection_source,
        serial
    );

    wl_display_roundtrip(display);
    destroy_popup_surface();
}

void complain_about_missing_keyboard() {
    bail("Setting primary selection is not supported without a keyboard");
}

#endif

void do_offer
(
    char *mime_type,
    void *source,
    void (*offer_f)(void *source, const char *type)
) {
    if (mime_type == NULL || mime_type_is_text(mime_type)) {
        // offer a few generic plain text formats
        offer_f(source, text_plain);
        offer_f(source, text_plain_utf8);
        offer_f(source, "TEXT");
        offer_f(source, "STRING");
        offer_f(source, "UTF8_STRING");
    }
    if (mime_type != NULL) {
        offer_f(source, mime_type);
    }
    free(mime_type);
}

void print_usage(FILE *f, const char *argv0) {
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

int main(int argc, char * const argv[]) {

    if (argc < 1) {
        bail("Empty argv");
    }

    int stay_in_foreground = 0;
    int clear = 0;
    char *mime_type = NULL;
    int primary = 0;

    static struct option long_options[] = {
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {"primary", no_argument, 0, 'p'},
        {"paste-once", no_argument, 0, 'o'},
        {"foreground", no_argument, 0, 'f'},
        {"clear", no_argument, 0, 'c'},
        {"type", required_argument, 0, 't'},
        {"seat", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };
    while (1) {
        int option_index;
        const char *opts = "vhpofct:s:";
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
            primary = 1;
            break;
        case 'o':
            paste_once = 1;
            break;
        case 'f':
            stay_in_foreground = 1;
            break;
        case 'c':
            clear = 1;
            break;
        case 't':
            mime_type = strdup(optarg);
            break;
        case 's':
            requested_seat_name = strdup(optarg);
            break;
        default:
            // getopt has already printed an error message
            print_usage(stderr, argv[0]);
            exit(1);
        }
    }

    init_wayland_globals();

    if (primary) {
#ifdef HAVE_GTK_PRIMARY_SELECTION
        if (primary_selection_device_manager == NULL) {
            bail("Primary selection is not supported on this compositor");
        }
#else
        bail("wl-clipboard was built without primary selection support");
#endif
    }

    if (!clear) {
        if (optind < argc) {
            // copy our command-line args
            data_to_copy = &argv[optind];
        } else {
            // copy stdin
            temp_file_to_copy = dump_stdin_into_a_temp_file();
            if (mime_type == NULL) {
                mime_type = infer_mime_type_from_contents(temp_file_to_copy);
            }
        }
    }

    if (!stay_in_foreground && !clear) {
        if (fork() != 0) {
            // exit in the parent, but leave the
            // child running in the background
            exit(0);
        }
    }

    if (!primary) {
        data_source = wl_data_device_manager_create_data_source(
            data_device_manager
        );
        wl_data_source_add_listener(data_source, &data_source_listener, NULL);

        do_offer(
            mime_type,
            data_source,
            (void (*)(void *, const char *)) wl_data_source_offer
        );

        action_on_popup_surface_getting_focus = set_data_selection;
        action_on_no_keyboard = try_setting_data_selection_directly;
        popup_tiny_invisible_surface();
    } else {
#ifdef HAVE_GTK_PRIMARY_SELECTION
        primary_selection_source =
            gtk_primary_selection_device_manager_create_source(
                primary_selection_device_manager
            );
        gtk_primary_selection_source_add_listener(
            primary_selection_source,
            &primary_selection_source_listener,
            NULL
        );

        do_offer(
            mime_type,
            primary_selection_source,
            (void (*)(void *, const char *)) gtk_primary_selection_source_offer
        );

        action_on_popup_surface_getting_focus = set_primary_selection;
        action_on_no_keyboard = complain_about_missing_keyboard;
        popup_tiny_invisible_surface();
#endif
    }

    if (clear) {
        wl_display_roundtrip(display);
        exit(0);
    }

    while (wl_display_dispatch(display) >= 0);

    perror("wl_display_dispatch");
    return 1;
}

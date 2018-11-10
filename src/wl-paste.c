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

char *mime_type = NULL;
int no_newline = 0;
int list_types = 0;

void do_paste(int pipefd[2]) {
    destroy_popup_surface();

    wl_display_roundtrip(display);

    if (fork() == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execl("/bin/cat", "cat", NULL);
        // failed to execl
        perror("exec /bin/cat");
        exit(1);
    }
    close(pipefd[0]);
    close(pipefd[1]);
    wait(NULL);
    if (!no_newline) {
        write(STDOUT_FILENO, "\n", 1);
    }
    exit(0);
}

void data_offer_offer
(
    void *data,
    struct wl_data_offer *data_offer,
    const char *offered_mime_type
) {
    if (list_types) {
        printf("%s\n", offered_mime_type);
    }
}

const struct wl_data_offer_listener data_offer_listener = {
    .offer = data_offer_offer
};

void data_device_data_offer
(
    void *data,
    struct wl_data_device *data_device,
    struct wl_data_offer *data_offer
) {
    wl_data_offer_add_listener(data_offer, &data_offer_listener, NULL);
}

void data_device_selection
(
    void *data,
    struct wl_data_device *data_device,
    struct wl_data_offer *data_offer
) {
    if (data_offer == NULL) {
        bail("No selection");
    }

    if (list_types) {
        exit(0);
    }

    int pipefd[2];
    pipe(pipefd);

    if (mime_type != NULL) {
        wl_data_offer_receive(data_offer, mime_type, pipefd[1]);
        free(mime_type);
    } else {
        wl_data_offer_receive(data_offer, "text/plain", pipefd[1]);
    }

    do_paste(pipefd);
}

const struct wl_data_device_listener data_device_listener = {
    .data_offer = data_device_data_offer,
    .selection = data_device_selection
};

#ifdef HAVE_GTK_PRIMARY_SELECTION

void primary_selection_offer_offer
(
    void *data,
    struct gtk_primary_selection_offer *primary_selection_offer,
    const char *offered_mime_type
) {
    if (list_types) {
        printf("%s\n", offered_mime_type);
    }
}

const struct gtk_primary_selection_offer_listener
primary_selection_offer_listener = {
    .offer = primary_selection_offer_offer
};

void primary_selection_device_data_offer
(
    void *data,
    struct gtk_primary_selection_device *primary_selection_device,
    struct gtk_primary_selection_offer *primary_selection_offer
) {
    gtk_primary_selection_offer_add_listener(
        primary_selection_offer,
        &primary_selection_offer_listener,
        NULL
    );
}

void primary_selection_device_selection
(
    void *data,
    struct gtk_primary_selection_device *primary_selection_device,
    struct gtk_primary_selection_offer *primary_selection_offer
) {
    if (primary_selection_offer == NULL) {
        bail("No selection");
    }

    if (list_types) {
        exit(0);
    }

    int pipefd[2];
    pipe(pipefd);

    if (mime_type != NULL) {
        gtk_primary_selection_offer_receive(
            primary_selection_offer,
            mime_type,
            pipefd[1]
        );
        free(mime_type);
    } else {
        gtk_primary_selection_offer_receive(
            primary_selection_offer,
            "text/plain",
            pipefd[1]
        );
    }

    do_paste(pipefd);
}

const struct gtk_primary_selection_device_listener
primary_selection_device_listener = {
    .data_offer = primary_selection_device_data_offer,
    .selection = primary_selection_device_selection
};

#endif

int main(int argc, char * const argv[]) {

    int primary = 0;

    static struct option long_options[] = {
        {"primary", no_argument, 0, 'p'},
        {"no-newline", no_argument, 0, 'n'},
        {"list-types", no_argument, 0, 'l'},
        {"type", required_argument, 0, 't'}
    };
    while (1) {
        int option_index;
        int c = getopt_long(argc, argv, "pnlt:", long_options, &option_index);
        if (c == -1) {
            break;
        }
        if (c == 0) {
            c = long_options[option_index].val;
        }
        switch (c) {
        case 'p':
            primary = 1;
            break;
        case 'n':
            no_newline = 1;
            break;
        case 'l':
            list_types = 1;
            break;
        case 't':
            mime_type = strdup(optarg);
            break;
        default:
            // getopt has already printed an error message
            exit(1);
        }
    }

    char *path = path_for_fd(STDOUT_FILENO);
    if (path != NULL && mime_type == NULL) {
        mime_type = infer_mime_type_from_name(path);
    }
    free(path);

    if (mime_type != NULL && !mime_type_is_text(mime_type)) {
        // never append a newline character to binary content
        no_newline = 1;
    }

    init_wayland_globals();

    if (!primary) {
        wl_data_device_add_listener(data_device, &data_device_listener, NULL);
    } else {
#ifdef HAVE_GTK_PRIMARY_SELECTION
        if (primary_selection_device_manager == NULL) {
            bail("Primary selection is not supported on this compositor");
        }
        gtk_primary_selection_device_add_listener(
            primary_selection_device,
            &primary_selection_device_listener,
            NULL
        );
#else
        bail("wl-clipboard was built without primary selection support");
#endif
    }

    // HACK:
    // pop up a tiny invisible surface to get the keyboard focus,
    // otherwise we won't be notified of the selection
    popup_tiny_invisible_surface();

    while (1) {
        wl_display_dispatch(display);
    }

    // never reached
    return 1;
}

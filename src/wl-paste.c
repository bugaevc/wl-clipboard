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

struct {
    char *explicit_type;
    char *inferred_type;
    int no_newline;
    int list_types;
} options;

struct {
    int explicit_available;
    int inferred_available;
    int plain_text_utf8_available;
    int plain_text_available;
    char *having_explicit_as_prefix;
    char *any_text;
    char *any;
} available_types;

void do_process_offer(const char *offered_type) {
    if (options.list_types) {
        printf("%s\n", offered_type);
    } else {
        if (
            options.explicit_type != NULL &&
            strcmp(offered_type, options.explicit_type) == 0
        ) {
            available_types.explicit_available = 1;
        }
        if (
            options.inferred_type != NULL &&
            strcmp(offered_type, options.inferred_type) == 0
        ) {
            available_types.inferred_available = 1;
        }
        if (
            strcmp(offered_type, text_plain_utf8) == 0) {
            available_types.plain_text_utf8_available = 1;
        }
        if (
            strcmp(offered_type, text_plain) == 0) {
            available_types.plain_text_available = 1;
        }
        if (
            available_types.any_text == NULL &&
            mime_type_is_text(offered_type)
        ) {
            available_types.any_text = strdup(offered_type);
        }
        if (available_types.any == NULL) {
            available_types.any = strdup(offered_type);
        }
        if (
            options.explicit_type != NULL &&
            available_types.having_explicit_as_prefix == NULL &&
            str_has_prefix(offered_type, options.explicit_type)
        ) {
            available_types.having_explicit_as_prefix = strdup(offered_type);
        }
    }
}

#define try_explicit \
if (available_types.explicit_available) \
    return options.explicit_type

#define try_inferred \
if (available_types.inferred_available) \
    return options.inferred_type

#define try_text_plain_utf8 \
if (available_types.plain_text_utf8_available) \
    return text_plain_utf8

#define try_text_plain \
if (available_types.plain_text_available) \
    return text_plain

#define try_prefixed \
if (available_types.having_explicit_as_prefix != NULL) \
    return available_types.having_explicit_as_prefix

#define try_any_text \
if (available_types.any_text != NULL) \
    return available_types.any_text

#define try_any \
if (available_types.any != NULL) \
    return available_types.any

const char *mime_type_to_request() {
    if (options.explicit_type != NULL) {
        if (strcmp(options.explicit_type, "text") == 0) {
            try_text_plain_utf8;
            try_text_plain;
            try_any_text;
        } else if (strchr(options.explicit_type, '/') != NULL) {
            try_explicit;
        } else if (isupper(options.explicit_type[0])) {
            try_explicit;
        } else {
            try_explicit;
            try_prefixed;
        }
    } else {
        // no mime type requested explicitly, try to guess
        if (options.inferred_type == NULL) {
            try_text_plain_utf8;
            try_text_plain;
            try_any_text;
            try_any;
        } else if (mime_type_is_text(options.inferred_type)) {
            try_inferred;
            try_text_plain_utf8;
            try_text_plain;
            try_any_text;
        } else {
            try_inferred;
        }
    }
    bail("No suitable type of content copied");
}

#undef try_explicit
#undef try_inferred
#undef try_text_plain_utf8
#undef try_text_plain
#undef try_prefixed
#undef try_any_text
#undef try_any

void free_types() {
    free(available_types.having_explicit_as_prefix);
    free(available_types.any_text);
    free(available_types.any);
    free(options.explicit_type);
    free(options.inferred_type);
}

void do_paste
(
    void *offer,
    void (*receive_f)(void *offer, const char *mime_type, int fd)
) {
    if (offer == NULL) {
        bail("No selection");
    }

    if (options.list_types) {
        exit(0);
    }

    const char *mime_type = mime_type_to_request();
    // never append a newline character to binary content
    if (!mime_type_is_text(mime_type)) {
        options.no_newline = 1;
    }

    int pipefd[2];
    pipe(pipefd);

    receive_f(offer, mime_type, pipefd[1]);

    free_types();
    destroy_popup_surface();

    wl_display_roundtrip(display);

    if (fork() == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execlp("cat", "cat", NULL);
        perror("exec cat");
        exit(1);
    }
    close(pipefd[0]);
    close(pipefd[1]);
    wait(NULL);
    if (!options.no_newline) {
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
    do_process_offer(offered_mime_type);
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
    do_paste(
        data_offer,
        (void (*)(void *, const char *, int)) wl_data_offer_receive
    );
}

const struct wl_data_device_listener data_device_listener = {
    .data_offer = data_device_data_offer,
    .selection = data_device_selection
};

#ifdef HAVE_GTK_PRIMARY_SELECTION

void gtk_primary_selection_offer_offer
(
    void *data,
    struct gtk_primary_selection_offer *gtk_primary_selection_offer,
    const char *offered_mime_type
) {
    do_process_offer(offered_mime_type);
}

const struct gtk_primary_selection_offer_listener
gtk_primary_selection_offer_listener = {
    .offer = gtk_primary_selection_offer_offer
};

void gtk_primary_selection_device_data_offer
(
    void *data,
    struct gtk_primary_selection_device *gtk_primary_selection_device,
    struct gtk_primary_selection_offer *gtk_primary_selection_offer
) {
    gtk_primary_selection_offer_add_listener(
        gtk_primary_selection_offer,
        &gtk_primary_selection_offer_listener,
        NULL
    );
}

void gtk_primary_selection_device_selection
(
    void *data,
    struct gtk_primary_selection_device *gtk_primary_selection_device,
    struct gtk_primary_selection_offer *gtk_primary_selection_offer
) {
    do_paste(
        gtk_primary_selection_offer,
        (void (*)(void *, const char *, int))
              gtk_primary_selection_offer_receive
    );
}

const struct gtk_primary_selection_device_listener
gtk_primary_selection_device_listener = {
    .data_offer = gtk_primary_selection_device_data_offer,
    .selection = gtk_primary_selection_device_selection
};

#endif

#ifdef HAVE_WLR_DATA_CONTROL
void data_control_offer_offer
(
    void *data,
    struct zwlr_data_control_offer_v1 *data_offer,
    const char *offered_mime_type
) {
    do_process_offer(offered_mime_type);
}

const struct zwlr_data_control_offer_v1_listener data_control_offer_listener = {
    .offer = data_control_offer_offer
};

void data_control_device_data_offer
(
    void *data,
    struct zwlr_data_control_device_v1 *data_control_device,
    struct zwlr_data_control_offer_v1 *data_control_offer
) {
    zwlr_data_control_offer_v1_add_listener(
        data_control_offer,
        &data_control_offer_listener,
        NULL
    );
}

void data_control_device_selection
(
    void *data,
    struct zwlr_data_control_device_v1 *data_control_device,
    struct zwlr_data_control_offer_v1 *data_control_offer
) {
    do_paste(
        data_control_offer,
        (void (*)(void *, const char *, int)) zwlr_data_control_offer_v1_receive
    );
}

const struct zwlr_data_control_device_v1_listener
data_control_device_listener = {
    .data_offer = data_control_device_data_offer,
    .selection = data_control_device_selection
};
#endif

void print_usage(FILE *f, const char *argv0) {
    fprintf(
        f,
        "Usage:\n"
        "\t%s [options]\n"
        "Paste content from the Wayland clipboard.\n\n"
        "Options:\n"
        "\t-n, --no-newline\tDo not append a newline character.\n"
        "\t-l, --list-types\tInstead of pasting, list the offered types.\n"
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
        argv0
    );
}

void init_selection() {
    if (use_wlr_data_control) {
#ifdef HAVE_WLR_DATA_CONTROL
        zwlr_data_control_device_v1_add_listener(
            data_control_device,
            &data_control_device_listener,
            NULL
        );
#endif
    } else {
        wl_data_device_add_listener(data_device, &data_device_listener, NULL);
        popup_tiny_invisible_surface();
    }
}

void init_primary_selection() {
#ifdef HAVE_GTK_PRIMARY_SELECTION
    if (gtk_primary_selection_device != NULL) {
        gtk_primary_selection_device_add_listener(
            gtk_primary_selection_device,
            &gtk_primary_selection_device_listener,
            NULL
        );
        popup_tiny_invisible_surface();
    } else {
        bail("Primary selection is not supported on this compositor");
    }
#else
    bail("wl-clipboard was built without primary selection support");
#endif
}

int main(int argc, char * const argv[]) {

    if (argc < 1) {
        bail("Empty argv");
    }

    int primary = 0;

    static struct option long_options[] = {
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {"primary", no_argument, 0, 'p'},
        {"no-newline", no_argument, 0, 'n'},
        {"list-types", no_argument, 0, 'l'},
        {"type", required_argument, 0, 't'},
        {"seat", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };
    while (1) {
        int option_index;
        const char *opts = "vhpnlt:s:";
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
        case 'n':
            options.no_newline = 1;
            break;
        case 'l':
            options.list_types = 1;
            break;
        case 't':
            options.explicit_type = strdup(optarg);
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

    char *path = path_for_fd(STDOUT_FILENO);
    if (path != NULL && options.explicit_type == NULL) {
        options.inferred_type = infer_mime_type_from_name(path);
    }
    free(path);

    init_wayland_globals();

    if (!primary) {
        init_selection();
    } else {
        init_primary_selection();
    }

    while (wl_display_dispatch(display) >= 0);

    perror("wl_display_dispatch");
    return 1;
}

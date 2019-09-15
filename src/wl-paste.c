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
#include "types/offer.h"
#include "types/device.h"
#include "types/device-manager.h"
#include "types/registry.h"

static struct {
    char *explicit_type;
    char *inferred_type;
    int no_newline;
    int list_types;
    int primary;
} options;

struct types {
    int explicit_available;
    int inferred_available;
    int plain_text_utf8_available;
    int plain_text_available;
    const char *having_explicit_as_prefix;
    const char *any_text;
    const char *any;
};

static struct types classify_offer_types(struct offer *offer) {
    struct types types = { 0 };
    offer_for_each_mime_type(offer, mime_type) {
        if (
            options.explicit_type != NULL &&
            strcmp(mime_type, options.explicit_type) == 0
        ) {
            types.explicit_available = 1;
        }
        if (
            options.inferred_type != NULL &&
            strcmp(mime_type, options.inferred_type) == 0
        ) {
            types.inferred_available = 1;
        }
        if (strcmp(mime_type, text_plain_utf8) == 0) {
            types.plain_text_utf8_available = 1;
        }
        if (strcmp(mime_type, text_plain) == 0) {
            types.plain_text_available = 1;
        }
        if (
            types.any_text == NULL &&
            mime_type_is_text(mime_type)
        ) {
            types.any_text = mime_type;
        }
        if (types.any == NULL) {
            types.any = mime_type;
        }
        if (
            options.explicit_type != NULL &&
            types.having_explicit_as_prefix == NULL &&
            str_has_prefix(mime_type, options.explicit_type)
        ) {
            types.having_explicit_as_prefix = mime_type;
        }
    }
    return types;
}

#define try_explicit \
if (types.explicit_available) \
    return options.explicit_type

#define try_inferred \
if (types.inferred_available) \
    return options.inferred_type

#define try_text_plain_utf8 \
if (types.plain_text_utf8_available) \
    return text_plain_utf8

#define try_text_plain \
if (types.plain_text_available) \
    return text_plain

#define try_prefixed \
if (types.having_explicit_as_prefix != NULL) \
    return types.having_explicit_as_prefix

#define try_any_text \
if (types.any_text != NULL) \
    return types.any_text

#define try_any \
if (types.any != NULL) \
    return types.any

static const char *mime_type_to_request(struct types types) {
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
        /* No mime type requested explicitly,
         * so try to guess.
         */
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

static void selection_callback(struct offer *offer, int primary) {
    /* Ignore events we're not interested in */
    if (primary != options.primary) {
        if (offer != NULL) {
            offer_destroy(offer);
        };
        return;
    }

    if (offer == NULL) {
        bail("No selection");
    }

    if (options.list_types) {
        offer_for_each_mime_type(offer, mime_type) {
            printf("%s\n", mime_type);
        }
        exit(0);
    }

    struct types types = classify_offer_types(offer);
    const char *mime_type = mime_type_to_request(types);

    /* Never append a newline character to binary content */
    if (!mime_type_is_text(mime_type)) {
        options.no_newline = 1;
    }

    int pipefd[2];
    pipe(pipefd);

    offer_receive(offer, mime_type, pipefd[1]);

    destroy_popup_surface();
    wl_display_roundtrip(display);

    /* Spawn a cat to perform the copy */
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
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

    offer_destroy(offer);

    free(options.explicit_type);
    free(options.inferred_type);
    exit(0);
}

static void print_usage(FILE *f, const char *argv0) {
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

static void parse_options(int argc, argv_t argv) {
    if (argc < 1) {
        bail("Empty argv");
    }

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
            options.primary = 1;
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
            /* getopt has already printed an error message */
            print_usage(stderr, argv[0]);
            exit(1);
        }
    }

    if (optind != argc) {
        fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        print_usage(stderr, argv[0]);
        exit(1);
    }
}

int main(int argc, argv_t argv) {
    parse_options(argc, argv);

    char *path = path_for_fd(STDOUT_FILENO);
    if (path != NULL && options.explicit_type == NULL) {
        options.inferred_type = infer_mime_type_from_name(path);
    }
    free(path);

    init_wayland_globals();

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
    device->selection_callback = selection_callback;

    if (device->needs_popup_surface) {
        popup_tiny_invisible_surface();
    }

    while (wl_display_dispatch(display) >= 0);

    perror("wl_display_dispatch");
    return 1;
}

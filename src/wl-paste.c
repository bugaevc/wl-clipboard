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

#include "types/offer.h"
#include "types/device.h"
#include "types/device-manager.h"
#include "types/registry.h"
#include "types/popup-surface.h"

#include "util/files.h"
#include "util/string.h"
#include "util/misc.h"

#include <wayland-client.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/wait.h>
#include <wayland-util.h>

static struct {
    char *explicit_type;
    char *inferred_type;
    int no_newline;
    int list_types;
    int primary;
    int watch;
    argv_t watch_command;
    const char *seat_name;
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

static struct wl_display *wl_display = NULL;
static struct popup_surface *popup_surface = NULL;
static int offer_received = 0;

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
    return NULL;
}

#undef try_explicit
#undef try_inferred
#undef try_text_plain_utf8
#undef try_text_plain
#undef try_prefixed
#undef try_any_text
#undef try_any

static void selection_callback(struct offer *offer, int primary) {
    /* Ignore all but the first non-NULL offer */
    if (offer_received && !options.watch) {
        return;
    }

    /* Ignore events we're not interested in */
    if (primary != options.primary) {
        if (offer != NULL) {
            offer_destroy(offer);
        };
        return;
    }

    if (offer == NULL) {
        if (options.watch) {
            return;
        }
        bail("No selection");
    }

    offer_received = 1;

    if (options.list_types) {
        offer_for_each_mime_type(offer, mime_type) {
            printf("%s\n", mime_type);
        }
        exit(0);
    }

    struct types types = classify_offer_types(offer);
    const char *mime_type = mime_type_to_request(types);

    if (mime_type == NULL) {
        if (options.watch) {
            offer_destroy(offer);
            return;
        }
        bail("No suitable type of content copied");
    }

    /* Never append a newline character to binary content */
    if (!mime_type_is_text(mime_type)) {
        options.no_newline = 1;
    }

    /* Create a pipe which we'll
     * use to receive the data.
     */
    int pipefd[2];
    int rc = pipe(pipefd);
    if (rc < 0) {
        perror("pipe");
        offer_destroy(offer);
        if (options.watch) {
            return;
        }
        exit(1);
    }

    offer_receive(offer, mime_type, pipefd[1]);

    if (popup_surface != NULL) {
        popup_surface_destroy(popup_surface);
        popup_surface = NULL;
    }
    wl_display_roundtrip(wl_display);

    /* Spawn a cat to perform the copy.
     * If watch mode is active, we spawn
     * a custom command instead.
     */
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        if (options.watch) {
            /* Try to cope without exiting completely */
            close(pipefd[0]);
            close(pipefd[1]);
            offer_destroy(offer);
            return;
        }
        exit(1);
    }
    if (pid == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        if (options.watch) {
            execvp(options.watch_command[0], options.watch_command);
            fprintf(
                stderr,
                "Failed to spawn %s: %s",
                options.watch_command[0],
                strerror(errno)
            );
        } else {
            execlp("cat", "cat", NULL);
            perror("exec cat");
        }
        exit(1);
    }
    close(pipefd[0]);
    close(pipefd[1]);
    wait(NULL);
    if (!options.no_newline && !options.watch) {
        rc = write(STDOUT_FILENO, "\n", 1);
        if (rc != 1) {
            perror("write");
        }
    }

    offer_destroy(offer);

    if (!options.watch) {
        free(options.explicit_type);
        free(options.inferred_type);
        exit(0);
    }
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
        "\t-w, --watch command\t"
        "Run a command each time the selection changes.\n"
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
        {"watch", required_argument, 0, 'w'},
        {"type", required_argument, 0, 't'},
        {"seat", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };
    while (1) {
        int option_index;
        const char *opts = "vhpnlw:t:s:";
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
        case 'w':
            options.watch = 1;
            /* We tell getopt that --watch requires an argument,
             * but it's more nuanced than that. We actually take
             * that argument and everything that follows it as a
             * subcommand to spawn. When we get here, getopt sets
             * the optind variable to point to the *next* option
             * to be processed, after both the current option and
             * its argument, if any. So if we're invoked like this:
             * $ wl-paste --primary --watch foo bar baz
             * then optind will point to bar, as it considers foo
             * to be the argument that --watch requires. However,
             * if we get invoked like this:
             * $ wl-paste --watch=foo bar baz
             * or like this:
             * $ wl-paste -wfoo bar baz
             * then getopt will not consider it an error, and optind
             * will also point to bar. We do consider that an error,
             * so detect this case and print an error message.
             */
            options.watch_command = (argv_t) &argv[optind - 1];
            if (options.watch_command[0][0] == '-') {
                fprintf(
                    stderr,
                    "Expected a subcommand instead of an argument"
                    " after --watch\n"
                );
                print_usage(stderr, argv[0]);
                exit(1);
            }
            /* We're going to forward the rest of our
             * arguments to the command we spawn, so stop
             * trying to process further options.
             */
            return;
        case 't':
            options.explicit_type = strdup(optarg);
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

    wl_display = wl_display_connect(NULL);
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
    /* Set up the callback before checking whether the device
     * actually supports the kind of selection we need, because
     * checking for the support might roundtrip.
     */
    device->selection_callback = selection_callback;

    if (!device_supports_selection(device, options.primary)) {
        complain_about_selection_support(options.primary);
    }

    if (device->needs_popup_surface) {
        if (options.watch) {
            complain_about_watch_mode_support();
        }
        /* If we cannot get the selection directly, pop up
         * a surface. When it gets focus, we'll immediately
         * get the selection events, se we don't need to do
         * anything special on the surface getting focus.
         */
        popup_surface = calloc(1, sizeof(struct popup_surface));
        popup_surface->registry = registry;
        popup_surface->seat = seat;
        popup_surface_init(popup_surface);
    }

    while (wl_display_dispatch(wl_display) >= 0);

    perror("wl_display_dispatch");
    return 1;
}

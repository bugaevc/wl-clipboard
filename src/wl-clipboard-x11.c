/* wl-clipboard
 *
 * Copyright © 2019-2021 Sergey Bugaev <bugaevc@gmail.com>
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
#include "types/offer.h"

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
    int in;
    int out;
    int filter;
    int append;
    int follow;
    int zero_flush;
    int stay_in_foreground;
    int clear;
    char *mime_type;
    int trim_newline;
    size_t loops;
    int primary;
    unsigned long timeout;
    int sensitive;
} options;

static struct wl_display *wl_display = NULL;
static struct popup_surface *popup_surface = NULL;
static size_t remaining_loops = 0;

static void selection_callback(struct offer *offer, int primary) {
    /* Ignore events we're not interested in */
    if (!options.out || primary != options.primary) {
        if (offer != NULL) {
            offer_destroy(offer);
        };
	return;
    }

    if (offer == NULL) {
        bail("No selection");
    }

    // TODO
    printf("Would receive the selection here\n");
}

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
    if (remaining_loops == 0) {
        /* Unlimited loops */
        return;
    }
    remaining_loops--;
    if (remaining_loops == 0) {
        cleanup_and_exit(copy_action, 0);
    }
}

static void print_usage(FILE *f, const char *argv0) {
    fprintf(
        f,
        "Usage as xsel:\n"
        "\t%s [options]\n\n"
        "Options:\n"
        "\t-i, --input\tCopy to the clipboard.\n"
        "\t-o, --output\tPaste from the clipboard.\n"
        "\t-a, --append\tAppend to the clipboard.\n"
        "\t-f, --follow\tContinuously append to the clipboard.\n"
        "\t-c, --clear\tInstead of copying anything, clear the clipboard.\n"
        "\t-p, --primary\tUse the \"primary\" clipboard (default).\n"
        "\t-b, --clipboard\tUse the regular clipboard.\n"
        "\t-n, --nodetach\tStay in the foreground instead of forking.\n"
        "\t-v, --version\tDisplay version info.\n"
        "\t-h, --help\tDisplay this message.\n"
        "Some arcane options not listed, see xsel(1) for more details.\n\n"
        "Usage as xclip:\n"
        "\t%s [options] [files]\n"
        "Options:\n"
        "\t-in\t\tCopy to the clipboard.\n"
        "\t-out\t\tPaste from the clipboard.\n"
        "\t-loops number\tOnly serve this many paste requests and then exit.\n"
        "\t-selection name\tClipboard to use,"
        " \"primary\" (default) or \"clipboard\".\n"
        "\t-quiet\t\tStay in the foreground instead of forking.\n"
        "\t-target\t\tSpecify MIME type for the content.\n"
        "\t-rmlastnl\tDo not copy the trailing newline character.\n"
        "\t-version\tDisplay version info.\n"
        "\t-help\t\tDisplay this message.\n"
        "Some arcane options not listed, see xclip(1) for more details.\n",
        argv0,
        argv0
    );
}

static int getopt_xclip(int argc, argv_t argv, const struct option *options) {
    int initial_optind = optind;
    int consumed_args;
    const struct option *matching_option = NULL;

    /* Loop, looking for an option we recognize */
    for (;; optind++) {
        if (optind >= argc) {
            /* We have reached the end of argv */
            return -1;
        }

        const char *arg = argv[optind];
        if (arg[0] != '-') {
            /* This isn't an option */
            continue;
        }

        for (const struct option *option = options; option->name; option++) {
            /* See if this option matches */
            if (!str_has_prefix(option->name, arg + 1)) {
                continue;
            }
            if (matching_option != NULL) {
                /* If multiple options match,
                 * treat it like it doesn't
                 * match at all. */
                matching_option = NULL;
                break;
            }
            matching_option = option;
        }
        if (matching_option == NULL) {
            continue;
        }
        if (matching_option->has_arg == required_argument) {
            if (optind + 1 >= argc) {
                /* We need an argument, but
                 * there are no more arguments.
                 * Do not report an error, just
                 * consider this not to match. */
                return -1;
            }
            optarg = argv[optind + 1];
            consumed_args = 2;
        } else {
            optarg = NULL;
            consumed_args = 1;
        }

        /* We have found an option, stop the loop */
        break;
    }

    /* We started looking at initial_optind,
     * and found the option at optind. Move
     * forward everything in between. */
    if (optind != initial_optind) {
        char *buffer[consumed_args];
        memcpy(
            buffer,
            argv + optind,
            consumed_args * sizeof (char *)
        );
        memmove(
            (char **) argv + initial_optind + consumed_args,
            argv + initial_optind,
            (optind - initial_optind) * sizeof (char *)
        );
        memcpy(
            (char **) argv + initial_optind,
            buffer,
            consumed_args * sizeof (char *)
        );
    }

    optind = initial_optind + consumed_args;

    return matching_option->val;
}

static void parse_options(int argc, argv_t argv) {
    if (argc < 1) {
        bail("Empty argv");
    }

    const char *argv0_basename = get_file_basename(argv[0]);
    int xclip = strcmp(argv0_basename, "xclip") == 0;
    int xsel = strcmp(argv0_basename, "xsel") == 0;

    if (!xclip && !xsel) {
        fprintf(stderr, "Invoked as neither xclip nor xsel\n\n");
        print_usage(stderr, argv[0]);
        exit(1);
    }

    /* This is the default in both tools */
    options.primary = 1;

    if (xsel) {
        /* Parse xsel options */
        static struct option long_options[] = {
            {"version", no_argument, 0, 'V'},
            {"help", no_argument, 0, 'h'},
            {"verbose", no_argument, 0, 'v'},
            {"append", no_argument, 0, 'a'},
            {"input", no_argument, 0, 'i'},
            {"clear", no_argument, 0, 'c'},
            {"output", no_argument, 0, 'o'},
            {"follow", no_argument, 0, 'f'},
            {"zeroflush", no_argument, 0, 'z'},
            {"primary", no_argument, 0, 'p'},
            {"secondary", no_argument, 0, 's'},
            {"clipboard", no_argument, 0, 'b'},
            {"keep", no_argument, 0, 'k'},
            {"exchange", no_argument, 0, 'x'},
            {"display", required_argument, 0, 'D'},
            {"selectionTimeout", required_argument, 0, 't'},
            {"nodetach", no_argument, 0, 'n'},
            {"delete", no_argument, 0, 'd'},
            {"logfile", required_argument, 0, 'l'},
            {0, 0, 0, 0}
        };
        while (1) {
            int option_index;
            const char *opts = "hvaicofzpsbkxt:ndl:";
            int c = getopt_long(argc, argv, opts, long_options, &option_index);
            if (c == -1) {
                break;
            }
            if (c == 0) {
                c = long_options[option_index].val;
            }
            switch (c) {
            case 'V':
                print_version_info();
                exit(0);
            case 'h':
                print_usage(stdout, argv[0]);
                exit(0);
            case 'v':
                /* We don't have a verbose mode */
                break;
            case 'a':
                options.append = 1;
                options.in = 1;
                options.out = 0;
                break;
            case 'i':
                options.in = 1;
                options.out = 0;
                break;
            case 'c':
                options.clear = 1;
                break;
            case 'o':
                options.out = 1;
                options.in = 0;
                break;
            case 'f':
                options.follow = 1;
                options.in = 1;
                options.out = 0;
                break;
            case 'z':
                options.zero_flush = 1;
                options.follow = 1;
                options.in = 1;
                options.out = 0;
                break;
            case 'p':
                options.primary = 1;
                break;
            case 's':
            case 'x':
                bail("No secondary selection on Wayland");
                break;
            case 'b':
                options.primary = 0;
                break;
            case 'k':
                /* Complain, but exit successfully */
                fprintf(stderr, "Keep mode is unsupported\n");
                exit(0);
            case 'D':
                fprintf(stderr, "Ignoring --display on Wayland\n");
                break;
            case 't':
                options.timeout = atol(optarg);
                break;
            case 'n':
                options.stay_in_foreground = 1;
                break;
            case 'd':
                options.clear = 1;
                break;
            case 'l':
                fprintf(stderr, "Ignoring --logfile\n");
                break;
            default:
                /* getopt has already printed an error message */
                print_usage(stderr, argv[0]);
                exit(1);
            }
        }
    } else {
        /* Parse xclip options */
        static struct option xclip_options[] = {
            {"version", no_argument, 0, 'V'},
            {"help", no_argument, 0, 'h'},
            {"loops", required_argument, 0, 'l'},
            {"display", required_argument, 0, 'd'},
            {"selection", required_argument, 0, 's'},
            {"filter", no_argument, 0, 'f'},
            {"in", no_argument, 0, 'i'},
            {"out", no_argument, 0, 'o'},
            {"silent", no_argument, 0, 'S'},
            {"quiet", no_argument, 0, 'q'},
            {"verbose", no_argument, 0, 'v'},
            {"debug", no_argument, 0, 'D'},
            {"noutf8", no_argument, 0, 'n'},
            {"target", no_argument, 0, 't'},
            {"rmlastnl", no_argument, 0, 'r'},
            {"sensitive", no_argument, 0, 'Z'},
            {"wait", required_argument, 0, 'w'},
            {0, 0, 0, 0}
        };

        while (1) {
            int c = getopt_xclip(argc, argv, xclip_options);
            if (c == -1) {
                break;
            }
            switch (c) {
            case 'V':
                print_version_info();
                /* Do not exit */
                break;
            case 'h':
                print_usage(stdout, argv[0]);
                /* Do not exit */
                break;
            case 'l':
                options.loops = atol(optarg);
                break;
            case 'd':
                fprintf(stderr, "Ignoring -display on Wayland\n");
                break;
            case 's':
                /* xclip only really checks the first letter */
                switch (optarg[0]) {
                case 'p':
                case 'P':
                    options.primary = 1;
                    break;
                case 's':
                case 'S':
                    bail("No secondary selection on Wayland");
                    break;
                case 'c':
                case 'C':
                    options.primary = 0;
                    break;
                case 'b':
                case 'B':
                    bail("No cut buffers on Wayland");
                    break;
                default:
                    bail("Unknown clipboard name");
                    break;
                }
                break;
            case 'f':
                options.filter = 1;
                break;
            case 'i':
                options.in = 1;
                options.out = 0;
                break;
            case 'o':
                options.out = 1;
                options.in = 0;
                break;
            case 'S':
                /* We don't have a silent mode */
                break;
            case 'q':
            case 'v':
            case 'D':
                /* We don't have quiet, verbose or
                 * debug modes, but they cause us to
                 * stay in the foreground. */
                options.stay_in_foreground = 1;
                break;
            case 'n':
                /* We don't have a no-UTF-8 mode */
                break;
            case 't':
                options.mime_type = strdup(optarg);
                break;
            case 'r':
                options.trim_newline = 1;
                break;
            case 'Z':
                options.sensitive = 1;
                break;
            case 'w':
                break;
            }
        }
    }

    if (!options.in && !options.out) {
        if (xclip && optind < argc) {
            /* We have some files to copy, default to input */
            options.in = 1;
        } else {
            /* Default to output unless stdin is redirected */
            options.out = isatty(0);
            options.in = !options.out;
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

    struct seat *seat = registry_find_seat(registry, NULL);
    if (seat == NULL) {
        bail("Missing a seat");
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

    struct copy_action *copy_action = NULL;
    if (options.in || options.clear) {
        /* Create and initialize the copy action */
        copy_action = calloc(1, sizeof(struct copy_action));
        copy_action->device = device;
        copy_action->primary = options.primary;

        if (!options.clear) {
            if (optind < argc) {
                /* Copy files from our command-line arguments */
                // TODO
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
                copy_action->file_to_copy = temp_file;
            }

            /* Create the source */
            copy_action->source = device_manager_create_source(device_manager);
            if (options.mime_type != NULL) {
                source_offer(copy_action->source, options.mime_type);
            }
            if (
                options.mime_type == NULL ||
                mime_type_is_text(options.mime_type)
            ) {
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
    }

    if (device->needs_popup_surface) {
        popup_surface = calloc(1, sizeof(struct popup_surface));
        popup_surface->registry = registry;
        popup_surface->seat = seat;
        if (copy_action != NULL) {
            copy_action->popup_surface = popup_surface;
        } else {
            popup_surface_init(popup_surface);
        }
    }

    if (copy_action != NULL) {
        copy_action->did_set_selection_callback = did_set_selection_callback;
        copy_action->pasted_callback = pasted_callback;
        copy_action->cancelled_callback = cancelled_callback;
        copy_action_init(copy_action);
        remaining_loops = options.loops;
    }

    while (wl_display_dispatch(wl_display) >= 0);

    perror("wl_display_dispatch");
    cleanup_and_exit(copy_action, 1);
    return 1;
}

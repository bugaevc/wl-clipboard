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
    // unset O_NONBLOCK
    fcntl(fd, F_SETFL, 0);
    if (data_to_copy != NULL) {
        // copy the specified data, separated by spaces
        FILE *f = fdopen(fd, "w");
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
            // failed to execl
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

void data_source_cancelled_handler
(
    void *data,
    struct wl_data_source *data_source
) {
    // we're done!
    if (temp_file_to_copy != NULL) {
        execlp("rm", "rm", "-r", dirname(temp_file_to_copy), NULL);
        perror("exec rm");
        exit(1);
    } else {
        exit(0);
    }
}

const struct wl_data_source_listener data_source_listener = {
    .target = data_source_target_handler,
    .send = data_source_send_handler,
    .cancelled = data_source_cancelled_handler
};

void offer_plain_text(struct wl_data_source *data_source) {
    wl_data_source_offer(data_source, "text/plain");
    wl_data_source_offer(data_source, "text/plain;charset=utf-8");
    wl_data_source_offer(data_source, "TEXT");
    wl_data_source_offer(data_source, "STRING");
    wl_data_source_offer(data_source, "UTF8_STRING");
}

int main(int argc, char * const argv[]) {

    int stay_in_foreground = 0;
    int clear = 0;
    char *mime_type = NULL;

    static struct option long_options[] = {
        {"paste-once", no_argument, 0, 'o'},
        {"foreground", no_argument, 0, 'f'},
        {"clear", no_argument, 0, 'c'},
        {"mime-type", required_argument, 0, 't'}
    };
    while (1) {
        int option_index;
        int c = getopt_long(argc, argv, "ofct:", long_options, &option_index);
        if (c == -1) {
            break;
        }
        if (c == 0) {
            c = long_options[option_index].val;
        }
        switch (c) {
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
        default:
            // getopt has already printed an error message
            exit(1);
        }
    }

    if (!clear) {
        if (optind < argc) {
            // copy our command-line args
            data_to_copy = &argv[optind];
        } else {
            // copy stdin
            temp_file_to_copy = dump_into_a_temp_file(STDIN_FILENO);
            if (mime_type == NULL) {
                mime_type = infer_mime_type_of_file(temp_file_to_copy);
            }
        }
    }

    init_wayland_globals();

    if (!stay_in_foreground && !clear) {
        if (fork() != 0) {
            // exit in the parent, but leave the
            // child running in the background
            exit(0);
        }
    }

    struct wl_data_source *data_source = wl_data_device_manager_create_data_source(data_device_manager);
    wl_data_source_add_listener(data_source, &data_source_listener, NULL);

    if (mime_type != NULL) {
        if (strcmp(mime_type, "text/plain") == 0) {
            offer_plain_text(data_source);
        } else {
            wl_data_source_offer(data_source, mime_type);
        }
        free(mime_type);
    } else {
        offer_plain_text(data_source);
    }

    wl_data_device_set_selection(data_device, data_source, get_serial());

    if (clear) {
        wl_display_roundtrip(display);
        exit(0);
    }

    while (1) {
        wl_display_dispatch(display);
    }

    bail("Unreachable");
}

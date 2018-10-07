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

void data_device_data_offer
(
    void *data,
    struct wl_data_device *data_device,
    struct wl_data_offer *data_offer
) {}

void data_device_selection
(
    void *data,
    struct wl_data_device *data_device,
    struct wl_data_offer *data_offer
) {
    if (data_offer == NULL) {
        bail("No selection");
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

int main(int argc, char * const argv[]) {

    static struct option long_options[] = {
        {"no-newline", no_argument, 0, 'n'},
        {"mime-type", required_argument, 0, 't'}
    };
    while (1) {
        int option_index;
        int c = getopt_long(argc, argv, "nt:", long_options, &option_index);
        if (c == -1) {
            break;
        }
        if (c == 0) {
            c = long_options[option_index].val;
        }
        switch (c) {
        case 'n':
            no_newline = 1;
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
    if (path != NULL) {
        mime_type = infer_mime_type_of_file(path);
    }
    free(path);

    init_wayland_globals();

    wl_data_device_add_listener(data_device, &data_device_listener, NULL);

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

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

char *mime_type;

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
    exit(0);
}

const struct wl_data_device_listener data_device_listener = {
    .data_offer = data_device_data_offer,
    .selection = data_device_selection
};

int main(int argc, const char *argv[]) {

    mime_type = infer_mime_type_of_file(STDOUT_FILENO);

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

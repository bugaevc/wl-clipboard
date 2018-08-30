/* wl-copy
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
    // delegate to a (hopefully) highly optimized implementation of copying
    if (fork() == 0) {
        dup2(fd, STDOUT_FILENO);
        execl("/bin/cat", "cat", NULL);
        // failed to execl
        perror("exec /bin/cat");
        exit(1);
    }
    wait(NULL);
    exit(0);
}

void data_source_cancelled_handler
(
    void *data,
    struct wl_data_source *data_source
) {
    fprintf(stderr, "Cancelled\n");
    exit(1);
}

const struct wl_data_source_listener data_source_listener = {
    .target = data_source_target_handler,
    .send = data_source_send_handler,
    .cancelled = data_source_cancelled_handler
};

int main(int argc, const char *argv[]) {

    init_wayland_globals();

    struct wl_data_source *data_source = wl_data_device_manager_create_data_source(data_device_manager);
    wl_data_source_add_listener(data_source, &data_source_listener, NULL);

    wl_data_source_offer(data_source, "text/plain");
    wl_data_source_offer(data_source, "text/plain;charset=utf-8");
    wl_data_source_offer(data_source, "TEXT");
    wl_data_source_offer(data_source, "STRING");
    wl_data_source_offer(data_source, "UTF8_STRING");

    wl_data_device_set_selection(data_device, data_source, get_serial());

    while (1) {
        wl_display_dispatch(display);
    }

    // never reached
    return 1;
}

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

#include <wayland-client.h>
#include <stdio.h>
#include <string.h> // strcmp
#include <stdlib.h> // exit
#include <unistd.h> // execl, STDOUT_FILENO

struct wl_data_device_manager *data_device_manager;
struct wl_seat *seat;

int global_serial;

void registry_global_handler
(
    void *data,
    struct wl_registry *registry,
    uint32_t name,
    const char *interface,
    uint32_t version
) {
    if (strcmp(interface, "wl_data_device_manager") == 0) {
        data_device_manager = wl_registry_bind(
            registry,
            name,
            &wl_data_device_manager_interface,
            2
        );
    } else if (strcmp(interface, "wl_seat") == 0) {
        seat = wl_registry_bind(
            registry,
            name,
            &wl_seat_interface,
            5
        );
    }
}

void registry_global_remove_handler
(
    void *data,
    struct wl_registry *registry,
    uint32_t name
) {}



const struct wl_registry_listener registry_listener = {
    .global = registry_global_handler,
    .global_remove = registry_global_remove_handler
};


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
    dup2(fd, STDOUT_FILENO);
    execl("/bin/cat", "cat", NULL);
    // failed to execl
    perror("exec /bin/cat");
    exit(1);
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

void callback_done
(
    void *data,
    struct wl_callback *callback,
    uint32_t serial
) {
    global_serial = serial;
}

const struct wl_callback_listener callback_listener = {
    .done = callback_done
};

int main(int argc, const char *argv[]) {

    struct wl_display *display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "Failed to connect to a Wayland server\n");
        return 1;
    }

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    // wait for the "initial" set of globals to appear
    wl_display_roundtrip(display);

    if (data_device_manager == NULL || seat == NULL) {
        fprintf(stderr, "Missing a required global object\n");
        return 1;
    }

    struct wl_data_source *data_source = wl_data_device_manager_create_data_source(data_device_manager);
    wl_data_source_add_listener(data_source, &data_source_listener, NULL);

    wl_data_source_offer(data_source, "text/plain");
    wl_data_source_offer(data_source, "text/plain;charset=utf-8");
    wl_data_source_offer(data_source, "TEXT");
    wl_data_source_offer(data_source, "STRING");
    wl_data_source_offer(data_source, "UTF8_STRING");

    struct wl_callback *callback = wl_display_sync(display);
    wl_callback_add_listener(callback, &callback_listener, NULL);

    while (global_serial == 0) {
        wl_display_dispatch(display);
    }

    struct wl_data_device *data_device = wl_data_device_manager_get_data_device(data_device_manager, seat);
    wl_data_device_set_selection(data_device, data_source, global_serial);

    while (1) {
        wl_display_dispatch(display);
    }

    // never reached
    return 1;
}
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

#include "types/seat.h"
#include "types/keyboard.h"

#include <wayland-client.h>
#include <string.h>
#include <stdlib.h>

static void wl_seat_capabilities_handler(
    void *data,
    struct wl_seat *seat,
    uint32_t capabilities
) {
    struct seat *self = (struct seat *) data;
    self->capabilities = capabilities;
}

static void wl_seat_name_handler(
    void *data,
    struct wl_seat *seat,
    const char *name
) {
    struct seat *self = (struct seat *) data;
    self->name = strdup(name);
}

static const struct wl_seat_listener wl_seat_listener = {
    .capabilities = wl_seat_capabilities_handler,
    .name = wl_seat_name_handler
};

void seat_init(struct seat *self) {
    wl_seat_add_listener(self->proxy, &wl_seat_listener, self);
}

struct keyboard *seat_get_keyboard(struct seat *self) {
    if ((self->capabilities & WL_SEAT_CAPABILITY_KEYBOARD) == 0) {
        return NULL;
    }
    struct keyboard *keyboard = calloc(1, sizeof(struct keyboard));
    keyboard->proxy = wl_seat_get_keyboard(self->proxy);
    keyboard_init(keyboard);
    return keyboard;
}

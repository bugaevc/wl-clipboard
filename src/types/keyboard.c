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

#include "types/keyboard.h"

#include <unistd.h>

static void wl_keyboard_keymap_handler(
    void *data,
    struct wl_keyboard *keyboard,
    uint32_t format,
    int fd,
    uint32_t size
) {
    close(fd);
}

static void wl_keyboard_enter_handler(
    void *data,
    struct wl_keyboard *keyboard,
    uint32_t serial,
    struct wl_surface *surface,
    struct wl_array *keys
) {
    struct keyboard *self = (struct keyboard *) data;
    if (self->on_focus != NULL) {
        self->on_focus(self, serial);
    }
}

static void wl_keyboard_leave_handler(
    void *data,
    struct wl_keyboard *keyboard,
    uint32_t serial,
    struct wl_surface *surface
) {}

static void wl_keyboard_key_handler(
    void *data,
    struct wl_keyboard *keyboard,
    uint32_t serial,
    uint32_t time,
    uint32_t key,
    uint32_t state
) {}

static void wl_keyboard_modifiers_handler(
    void *data,
    struct wl_keyboard *keyboard,
    uint32_t serial,
    uint32_t mods_depressed,
    uint32_t mods_latched,
    uint32_t mods_locked,
    uint32_t group
) {}

static const struct wl_keyboard_listener wl_keyboard_listener = {
    .keymap = wl_keyboard_keymap_handler,
    .enter = wl_keyboard_enter_handler,
    .leave = wl_keyboard_leave_handler,
    .key = wl_keyboard_key_handler,
    .modifiers = wl_keyboard_modifiers_handler,
};

void keyboard_init(struct keyboard *self) {
    wl_keyboard_add_listener(self->proxy, &wl_keyboard_listener, self);
}

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

#ifndef TYPES_KEYBOARD_H
#define TYPES_KEYBOARD_H

#include <stdint.h>
#include <wayland-client.h>

struct keyboard {
    /* These fields are initialized by the creator */
    struct wl_keyboard *proxy;
    void (*on_focus)(struct keyboard *self, uint32_t serial);
    void *data;
};

void keyboard_init(struct keyboard *self);

#endif /* TYPES_KEYBOARD_H */

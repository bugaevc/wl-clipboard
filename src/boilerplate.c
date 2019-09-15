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

#include "boilerplate.h"
#include "types/shell-surface.h"
#include "types/shell.h"
#include "types/popup-surface.h"
#include "types/registry.h"

void init_wayland_globals() {
    display = wl_display_connect(NULL);
    if (display == NULL) {
        bail("Failed to connect to a Wayland server");
    }

    registry = calloc(1, sizeof(struct registry));
    registry->wl_display = display;
    registry_init(registry);

    /* Wait for the "initial" set of globals to appear */
    wl_display_roundtrip(display);

    seat = registry_find_seat(registry, requested_seat_name);
    if (seat == NULL) {
        if (requested_seat_name != NULL) {
            bail("No such seat");
        } else {
            bail("Missing a seat");
        }
    }
}

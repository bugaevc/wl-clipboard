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

static struct popup_surface *popup_surface = NULL;

static void forward_on_focus(struct popup_surface *popup_surface, uint32_t serial) {
    if (action_on_popup_surface_getting_focus != NULL) {
        action_on_popup_surface_getting_focus(serial);
    }
}

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

void popup_tiny_invisible_surface() {
    /* HACK:
     * Pop up a tiny invisible surface to get the keyboard focus,
     * otherwise we won't be notified of the selection.
     */

    popup_surface = calloc(1, sizeof(struct popup_surface));
    popup_surface->registry = registry;
    popup_surface->seat = seat;
    popup_surface->on_focus = forward_on_focus;

    popup_surface_init(popup_surface);
}

void destroy_popup_surface() {
    if (popup_surface != NULL) {
        popup_surface_destroy(popup_surface);
        popup_surface = NULL;
    }
}

static uint32_t global_serial;

static void callback_done
(
    void *data,
    struct wl_callback *callback,
    uint32_t serial
) {
    global_serial = serial;
}

static const struct wl_callback_listener callback_listener = {
    .done = callback_done
};

uint32_t get_serial() {
    struct wl_callback *callback = wl_display_sync(display);
    wl_callback_add_listener(callback, &callback_listener, NULL);

    while (global_serial == 0) {
        wl_display_dispatch(display);
    }

    return global_serial;
}

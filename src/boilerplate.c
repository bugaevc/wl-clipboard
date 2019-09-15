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
#include "types/keyboard.h"
#include "types/shell-surface.h"
#include "types/shell.h"
#include "types/popup-surface.h"
#include "types/registry.h"

static struct popup_surface *popup_surface = NULL;

static void forward_on_focus(struct keyboard *keyboard, uint32_t serial) {
    struct wl_seat *this_seat = (struct wl_seat *) keyboard->data;
    /* When we get to here, global seat is already initialized */
    if (this_seat != seat) {
        return;
    }
    if (action_on_popup_surface_getting_focus != NULL) {
        action_on_popup_surface_getting_focus(serial);
    }
}

static void seat_capabilities_handler
(
    void *data,
    struct wl_seat *this_seat,
    uint32_t capabilities
) {
    /* Stash the capabilities of this seat for later */
    void *user_data = (void *) (unsigned long) capabilities;
    wl_seat_set_user_data(this_seat, user_data);

    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        struct keyboard *keyboard = calloc(1, sizeof(struct keyboard));
        keyboard->proxy = wl_seat_get_keyboard(this_seat);
        keyboard->data = (void *) this_seat;
        keyboard->on_focus = forward_on_focus;
        keyboard_init(keyboard);
    }
}

static void seat_name_handler
(
    void *data,
    struct wl_seat *this_seat,
    const char *name
) {
    if (requested_seat_name == NULL) {
        return;
    }
    if (strcmp(name, requested_seat_name) == 0) {
        seat = this_seat;
    }
}

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities_handler,
    .name = seat_name_handler
};

#define UNSET_CAPABILITIES ((void *) (uint32_t) 35)

void process_new_seat(struct wl_seat *new_seat) {
    if (seat != NULL) {
        wl_seat_destroy(new_seat);
        return;
    }
    if (requested_seat_name == NULL) {
        seat = new_seat;
    }
    wl_seat_add_listener(new_seat, &seat_listener, UNSET_CAPABILITIES);
}

int ensure_seat_has_keyboard() {
    void *user_data = wl_seat_get_user_data(seat);
    while (user_data == UNSET_CAPABILITIES) {
        wl_display_roundtrip(display);
        user_data = wl_seat_get_user_data(seat);
    }

    uint32_t capabilities = (uint32_t) (unsigned long) user_data;
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        return 1;
    }

    if (action_on_no_keyboard != NULL) {
        action_on_no_keyboard();
    }
    return 0;
}

#undef UNSET_CAPABILITIES

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

    if (seat == NULL && requested_seat_name != NULL) {
        wl_display_roundtrip(display);
    }
    if (seat == NULL) {
        if (requested_seat_name == NULL) {
            bail("No seat available");
        }
        bail("Cannot find the requested seat");
    }
}

void popup_tiny_invisible_surface() {
    /* HACK:
     * Pop up a tiny invisible surface to get the keyboard focus,
     * otherwise we won't be notified of the selection.
     */

    if (!ensure_seat_has_keyboard()) {
        return;
    }

    /* Make sure that we get the keyboard
     * object before creating the surface,
     * so that we get the enter event.
     */
    wl_display_dispatch(display);

    popup_surface = calloc(1, sizeof(struct popup_surface));
    popup_surface->registry = registry;

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

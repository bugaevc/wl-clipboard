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

static struct shell_surface *shell_surface = NULL;
static struct shell *shell = NULL;

static void process_new_seat(struct wl_seat *new_seat);

static void registry_global_handler
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
            1
        );
    } else if (strcmp(interface, "wl_seat") == 0) {
        struct wl_seat *new_seat = wl_registry_bind(
            registry,
            name,
            &wl_seat_interface,
            2
        );
        process_new_seat(new_seat);
    } else if (strcmp(interface, "wl_compositor") == 0) {
        compositor = wl_registry_bind(
            registry,
            name,
            &wl_compositor_interface,
            3
        );
    } else if (strcmp(interface, "wl_shm") == 0) {
        shm = wl_registry_bind(
            registry,
            name,
            &wl_shm_interface,
            1
        );
    } else if (strcmp(interface, "wl_shell") == 0) {
        wl_shell = wl_registry_bind(
            registry,
            name,
            &wl_shell_interface,
            1
        );
    }
#ifdef HAVE_XDG_SHELL
    else if (strcmp(interface, "xdg_wm_base") == 0) {
        xdg_wm_base = wl_registry_bind(
            registry,
            name,
            &xdg_wm_base_interface,
            1
        );
    }
#endif
#ifdef HAVE_GTK_PRIMARY_SELECTION
    else if (strcmp(interface, "gtk_primary_selection_device_manager") == 0) {
        gtk_primary_selection_device_manager = wl_registry_bind(
            registry,
            name,
            &gtk_primary_selection_device_manager_interface,
            1
        );
    }
#endif
#ifdef HAVE_WP_PRIMARY_SELECTION
    else if (strcmp(interface, "zwp_primary_selection_device_manager_v1") == 0) {
        primary_selection_device_manager = wl_registry_bind(
            registry,
            name,
            &zwp_primary_selection_device_manager_v1_interface,
            1
        );
    }
#endif
#ifdef HAVE_WLR_DATA_CONTROL
    else if (strcmp(interface, "zwlr_data_control_manager_v1") == 0) {
        data_control_manager = wl_registry_bind(
            registry,
            name,
            &zwlr_data_control_manager_v1_interface,
            1
        );
    }
#endif
}

static void registry_global_remove_handler
(
    void *data,
    struct wl_registry *registry,
    uint32_t name
) {}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global_handler,
    .global_remove = registry_global_remove_handler
};

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

static void process_new_seat(struct wl_seat *new_seat) {
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

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    /* Wait for the "initial" set of globals to appear */
    wl_display_roundtrip(display);

    if (
        data_device_manager == NULL ||
        compositor == NULL ||
        shm == NULL ||
        (shell == NULL
#ifdef HAVE_XDG_SHELL
         && xdg_wm_base == NULL
#endif
        )
    ) {
        bail("Missing a required global object");
    }

    shell = calloc(1, sizeof(struct shell));
    if (wl_shell != NULL) {
        /* Use wl_shell */
        shell->proxy = (struct wl_proxy *) wl_shell;
        shell_init_wl_shell(shell);
    } else {
#ifdef HAVE_XDG_SHELL
        /* Use xdg-shell */
        shell->proxy = (struct wl_proxy *) xdg_wm_base;
        shell_init_xdg_shell(shell);
#else
        bail("Unreachable: HAVE_XDG_SHELL undefined and no wl_shell");
#endif
    }

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

void ensure_has_primary_selection() {
#ifdef HAVE_GTK_PRIMARY_SELECTION
    if (gtk_primary_selection_device_manager != NULL) {
        return;
    }
#endif
#ifdef HAVE_WP_PRIMARY_SELECTION
    if (primary_selection_device_manager != NULL) {
        return;
    }
#endif

#if defined(HAVE_GTK_PRIMARY_SELECTION) || defined(HAVE_WP_PRIMARY_SELECTION)
    bail("Primary selection is not supported on this compositor");
#else
    bail("wl-clipboard was built without primary selection support");
#endif
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

    surface = wl_compositor_create_surface(compositor);
    shell_surface = shell_create_shell_surface(shell, surface);
    /* Signal that the surface is ready to be configured */
    wl_surface_commit(surface);
    /* Wait for the surface to be configured */
    wl_display_roundtrip(display);

    if (surface == NULL) {
        /* It's possible that we've been given focus without us
         * ever commiting a buffer, in which case the handlers
         * may have already destroyed the surface; there's no
         * way or need for us to commit a buffer in that case.
         */
        return;
    }

    int width = 1;
    int height = 1;
    int stride = width * 4;
    int size = stride * height;  // bytes

    /* Open an anonymous file and write some zero bytes to it */
    int fd = create_anonymous_file();
    ftruncate(fd, size);

    /* Turn it into a shared memory pool */
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);

    /* Allocate the buffer in that pool */
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool,
        0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    /* Zeros in ARGB8888 mean fully transparent */

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_damage(surface, 0, 0, width, height);
    wl_surface_commit(surface);
}

void destroy_popup_surface() {
    if (shell_surface != NULL) {
        shell_surface_destroy(shell_surface);
        shell_surface = NULL;
    }
    if (surface != NULL) {
        wl_surface_destroy(surface);
        surface = NULL;
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

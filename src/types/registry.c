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

#include "types/registry.h"
#include "types/shell.h"
#include "types/device-manager.h"
#include "includes/shell-protocols.h"
#include "includes/selection-protocols.h"
#include "util/misc.h"

#include <string.h>
#include <stdlib.h>

void process_new_seat(struct wl_seat *new_seat);

#define BIND(interface_name, known_version) \
if (strcmp(interface, #interface_name) == 0) { \
    self->interface_name = wl_registry_bind( \
        wl_registry, \
        name, \
        &interface_name ## _interface, \
        known_version \
    ); \
}

static void wl_registry_global_handler(
    void *data,
    struct wl_registry *wl_registry,
    uint32_t name,
    const char *interface,
    uint32_t version
) {
    struct registry *self = (struct registry *) data;

    BIND(wl_compositor, 2)
    BIND(wl_shm, 1)

    /* Shells */

    BIND(wl_shell, 1)

#ifdef HAVE_XDG_SHELL
    BIND(xdg_wm_base, 1)
#endif

    /* Device managers */

    BIND(wl_data_device_manager, 1)

#ifdef HAVE_GTK_PRIMARY_SELECTION
    BIND(gtk_primary_selection_device_manager, 1)
#endif

#ifdef HAVE_WP_PRIMARY_SELECTION
    BIND(zwp_primary_selection_device_manager_v1, 1)
#endif

#ifdef HAVE_WLR_DATA_CONTROL
    BIND(zwlr_data_control_manager_v1, 1)
#endif

    if (strcmp(interface, "wl_seat") == 0) {
        struct wl_seat *seat = wl_registry_bind(
            wl_registry,
            name,
            &wl_seat_interface,
            2
        );
        process_new_seat(seat);
    }
}

static void wl_registry_global_remove_handler(
    void *data,
    struct wl_registry *wl_registry,
    uint32_t name
) {}

static const struct wl_registry_listener wl_registry_listener = {
    .global = wl_registry_global_handler,
    .global_remove = wl_registry_global_remove_handler
};

void registry_init(struct registry *self) {
    self->proxy = wl_display_get_registry(self->wl_display);
    wl_registry_add_listener(self->proxy, &wl_registry_listener, self);
}

struct shell *registry_find_shell(struct registry *self) {
    struct shell *shell = calloc(1, sizeof(struct shell));

    if (self->wl_shell != NULL) {
        shell->proxy = (struct wl_proxy *) self->wl_shell;
        shell_init_wl_shell(shell);
        return shell;
    }

#ifdef HAVE_XDG_SHELL
    if (self->xdg_wm_base != NULL) {
        shell->proxy = (struct wl_proxy *) self->xdg_wm_base;
        shell_init_xdg_shell(shell);
        return shell;
    }
#endif

    free(shell);
    return NULL;
}

#define TRY(type) \
if (self->type != NULL) { \
    device_manager->proxy = (struct wl_proxy *) self->type; \
    device_manager_init_ ## type(device_manager); \
    return device_manager; \
}

struct device_manager *registry_find_device_manager(
    struct registry *self,
    int primary
) {
    struct device_manager *device_manager
        = calloc(1, sizeof(struct device_manager));

    /* We prefer wlr-data-control, as it doesn't require
     * us to use the popup surface hack.
     */

    if (!primary) {
#ifdef HAVE_WLR_DATA_CONTROL
        TRY(zwlr_data_control_manager_v1)
#endif
        TRY(wl_data_device_manager)

        free(device_manager);
        return NULL;
    }

#ifdef HAVE_WP_PRIMARY_SELECTION
    TRY(zwp_primary_selection_device_manager_v1)
#endif

#ifdef HAVE_GTK_PRIMARY_SELECTION
    TRY(gtk_primary_selection_device_manager)
#endif

    free(device_manager);
    return NULL;
}

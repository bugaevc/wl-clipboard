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
#include "types/seat.h"
#include "types/shell.h"
#include "types/device-manager.h"
#include "includes/shell-protocols.h"
#include "includes/selection-protocols.h"
#include "util/misc.h"

#include <string.h>
#include <stdlib.h>

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
    BIND(zwlr_data_control_manager_v1, version > 2 ? 2 : version)
#endif

    if (strcmp(interface, "wl_seat") == 0) {
        struct seat *seat = calloc(1, sizeof(struct seat));
        seat->proxy = wl_registry_bind(
            wl_registry,
            name,
            &wl_seat_interface,
            2
        );
        seat_init(seat);
        struct seat **ptr = wl_array_add(&self->seats, sizeof(struct seat *));
        *ptr = seat;
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
    device_manager->wl_display = self->wl_display;

    /* For regular selection, we just look at the two supported
     * protocols. We prefer wlr-data-control, as it doesn't require
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

    /* For primary selection, it's a bit more complicated. We also
     * prefer wlr-data-control, but we don't know in advance whether
     * the compositor supports primary selection, as unlike with
     * other protocols here, the mere presence of wlr-data-control
     * does not imply primary selection support. However, we assume
     * that if a compositor supports primary selection at all, then
     * if it supports wlr-data-control v2 it also supports primary
     * selection over wlr-data-control; which is only reasonable.
     */

#ifdef HAVE_WLR_DATA_CONTROL
    if (self->zwlr_data_control_manager_v1 != NULL) {
        struct wl_proxy *proxy
            = (struct wl_proxy *) self->zwlr_data_control_manager_v1;
        if (wl_proxy_get_version(proxy) >= 2) {
            device_manager->proxy = proxy;
            device_manager_init_zwlr_data_control_manager_v1(device_manager);
            return device_manager;
        }
    }
#endif

#ifdef HAVE_WP_PRIMARY_SELECTION
    TRY(zwp_primary_selection_device_manager_v1)
#endif

#ifdef HAVE_GTK_PRIMARY_SELECTION
    TRY(gtk_primary_selection_device_manager)
#endif

    free(device_manager);
    return NULL;
}

struct seat *registry_find_seat(
    struct registry *self,
    const char *name
) {
    /* Ensure we get all the seat info */
    wl_display_roundtrip(self->wl_display);

    struct seat **ptr;
    wl_array_for_each(ptr, &self->seats) {
        struct seat *seat = *ptr;
        if (name == NULL || strcmp(seat->name, name) == 0) {
            return seat;
        }
    }
    return NULL;
}

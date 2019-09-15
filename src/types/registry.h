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

#ifndef TYPES_REGISTRY_H
#define TYPES_REGISTRY_H

#include "includes/shell-protocols.h"
#include "includes/selection-protocols.h"

#include <wayland-client.h>

struct shell;
struct device_manager;
struct seat;

struct registry {
    /* This field is initialized by the creator */
    struct wl_display *wl_display;

    /* These fields are initialized by the implementation */

    struct wl_registry *proxy;
    struct wl_array seats;

    struct wl_compositor *wl_compositor;
    struct wl_shm *wl_shm;

    /* Shells */

    struct wl_shell *wl_shell;
#ifdef HAVE_XDG_SHELL
    struct xdg_wm_base *xdg_wm_base;
#endif

    /* Device managers */

    struct wl_data_device_manager *wl_data_device_manager;
#ifdef HAVE_GTK_PRIMARY_SELECTION
    struct gtk_primary_selection_device_manager
        *gtk_primary_selection_device_manager;
#endif
#ifdef HAVE_WP_PRIMARY_SELECTION
    struct zwp_primary_selection_device_manager_v1
        *zwp_primary_selection_device_manager_v1;
#endif
#ifdef HAVE_WLR_DATA_CONTROL
    struct zwlr_data_control_manager_v1
        *zwlr_data_control_manager_v1;
#endif
};

void registry_init(struct registry *self);

struct shell *registry_find_shell(struct registry *self);

struct device_manager *registry_find_device_manager(
    struct registry *self,
    int primary
);

struct seat *registry_find_seat(
    struct registry *self,
    const char *name
);

#endif /* TYPES_REGISTRY_H */

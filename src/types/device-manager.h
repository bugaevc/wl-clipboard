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

#ifndef TYPES_DEVICE_MANAGER_H
#define TYPES_DEVICE_MANAGER_H

#include "includes/selection-protocols.h"

#include <wayland-client.h>

struct seat;
struct device;
struct source;

struct device_manager {
    /* These fields are initialized by the creator */
    struct wl_proxy *proxy;
    struct wl_display *wl_display;

    /* These fields are initialized by the implementation */
    struct source *(*do_create_source)(struct device_manager *self);
    struct device *(*do_get_device)(
        struct device_manager *self,
        struct seat *seat
    );
};

struct source *device_manager_create_source(struct device_manager *self);

struct device *device_manager_get_device(
    struct device_manager *self,
    struct seat *seat
);

/* Initializers */

void device_manager_init_wl_data_device_manager(struct device_manager *self);

#ifdef HAVE_GTK_PRIMARY_SELECTION
void device_manager_init_gtk_primary_selection_device_manager(
    struct device_manager *self
);
#endif

#ifdef HAVE_WP_PRIMARY_SELECTION
void device_manager_init_zwp_primary_selection_device_manager_v1(
    struct device_manager *self
);
#endif

#ifdef HAVE_WLR_DATA_CONTROL
void device_manager_init_zwlr_data_control_manager_v1(
    struct device_manager *self
);
#endif

#endif /* TYPES_DEVICE_MANAGER_H */

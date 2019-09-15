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

#include "types/device-manager.h"
#include "types/device.h"
#include "types/source.h"
#include "types/seat.h"
#include "includes/selection-protocols.h"

#include <stdlib.h>


struct source *device_manager_create_source(struct device_manager *self) {
    return self->do_create_source(self);
}

struct device *device_manager_get_device(
    struct device_manager *self,
    struct seat *seat
) {
    return self->do_get_device(self, seat);
}

/* Macros to reduce implementation boilerplate */

#define CREATE_SOURCE(type, source_type, method_name) \
static struct source *device_manager_ ## type ## _do_create_source( \
    struct device_manager *self \
) { \
    struct type *proxy = (struct type *) self->proxy; \
    struct source *source = calloc(1, sizeof(struct source)); \
    source->proxy = (struct wl_proxy *) type ## _ ## method_name(proxy); \
    source_init_ ## source_type(source); \
    return source; \
}

#define GET_DEVICE(type, device_type, method_name) \
static struct device *device_manager_ ## type ## _do_get_device( \
    struct device_manager *self, \
    struct seat *seat_wrapper \
) { \
    struct type *proxy = (struct type *) self->proxy; \
    struct wl_seat *seat = (struct wl_seat *) seat_wrapper->proxy; \
    struct device *device = calloc(1, sizeof(struct device)); \
    device->proxy = (struct wl_proxy *) type ## _ ## method_name(proxy, seat); \
    device->wl_display = self->wl_display; \
    device_init_ ## device_type(device); \
    return device; \
}

#define INIT(type) \
void device_manager_init_ ## type(struct device_manager *self) { \
    self->do_create_source = device_manager_ ## type ## _do_create_source; \
    self->do_get_device = device_manager_ ## type ## _do_get_device; \
}


/* Core Wayland implementation */

CREATE_SOURCE(wl_data_device_manager, wl_data_source, create_data_source)
GET_DEVICE(wl_data_device_manager, wl_data_device, get_data_device)
INIT(wl_data_device_manager)


/* gtk-primary-selection implementation */

#ifdef HAVE_GTK_PRIMARY_SELECTION

CREATE_SOURCE(
    gtk_primary_selection_device_manager,
    gtk_primary_selection_source,
    create_source
)

GET_DEVICE(
    gtk_primary_selection_device_manager,
    gtk_primary_selection_device,
    get_device
)

INIT(gtk_primary_selection_device_manager)

#endif /* HAVE_GTK_PRIMARY_SELECTION */


/* wp-primary-selection implementation */

#ifdef HAVE_WP_PRIMARY_SELECTION

CREATE_SOURCE(
    zwp_primary_selection_device_manager_v1,
    zwp_primary_selection_source_v1,
    create_source
)

GET_DEVICE(
    zwp_primary_selection_device_manager_v1,
    zwp_primary_selection_device_v1,
    get_device
)

INIT(zwp_primary_selection_device_manager_v1)

#endif /* HAVE_WP_PRIMARY_SELECTION */


/* wlr-data-control implementation */

#ifdef HAVE_WLR_DATA_CONTROL

CREATE_SOURCE(
    zwlr_data_control_manager_v1,
    zwlr_data_control_source_v1,
    create_data_source
)

GET_DEVICE(
    zwlr_data_control_manager_v1,
    zwlr_data_control_device_v1,
    get_data_device
)

INIT(zwlr_data_control_manager_v1)

#endif /* HAVE_WLR_DATA_CONTROL */

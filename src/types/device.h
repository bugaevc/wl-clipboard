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

#ifndef TYPES_DEVICE_H
#define TYPES_DEVICE_H

#include "includes/selection-protocols.h"

#include <stdint.h>

struct offer;
struct source;

struct device {
    /* These fields are initialized by the creator */
    void (*new_offer_callback)(struct offer *offer);
    void (*selection_callback)(struct offer *offer, int primary);
    void *data;

    struct wl_proxy *proxy;
    struct wl_display *wl_display;

    /* These fields are initialized by the implementation */
    int (*supports_selection)(struct device *self, int primary);
    int needs_popup_surface;
    void (*do_set_selection)(
        struct device *self,
        struct source *source,
        uint32_t serial,
        int primary
    );
};

int device_supports_selection(struct device *self, int primary);

void device_set_selection(
    struct device *self,
    struct source *source,
    uint32_t serial,
    int primary
);

/* Initializers */

void device_init_wl_data_device(struct device *self);

#ifdef HAVE_GTK_PRIMARY_SELECTION
void device_init_gtk_primary_selection_device(struct device *self);
#endif

#ifdef HAVE_WP_PRIMARY_SELECTION
void device_init_zwp_primary_selection_device_v1(struct device *self);
#endif

#ifdef HAVE_WLR_DATA_CONTROL
void device_init_zwlr_data_control_device_v1(struct device *self);
#endif

#endif /* TYPES_DEVICE_H */

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

#ifndef TYPES_OFFER_H
#define TYPES_OFFER_H

#include "includes/selection-protocols.h"

struct offer {
    /* These fields are initialized by the creator */
    void (*offer_callback)(struct offer *self, const char *mime_type);
    void *data;

    struct wl_proxy *proxy;

    /* These fields are initialized by the implementation */
    void (*do_receive)(struct wl_proxy *proxy, const char *mime_type, int fd);
    void (*do_destroy)(struct wl_proxy *proxy);
};

void offer_receive(struct offer *self, const char *mime_type, int fd);
void offer_destroy(struct offer *self);

/* Initializers */

void offer_init_wl_data_offer(struct offer *self);

#ifdef HAVE_GTK_PRIMARY_SELECTION
void offer_init_gtk_primary_selection_offer(struct offer *self);
#endif

#ifdef HAVE_WP_PRIMARY_SELECTION
void offer_init_zwp_primary_selection_offer_v1(struct offer *self);
#endif

#ifdef HAVE_WLR_DATA_CONTROL
void offer_init_zwlr_data_control_offer_v1(struct offer *self);
#endif

#endif /* TYPES_OFFER_H */

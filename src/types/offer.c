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

#include "types/offer.h"
#include "includes/selection-protocols.h"

#include <wayland-util.h>
#include <stdlib.h>
#include <string.h>

void offer_receive(struct offer *self, const char *mime_type, int fd) {
    self->do_receive(self->proxy, mime_type, fd);
}

void offer_destroy(struct offer *self) {
    self->do_destroy(self->proxy);
    wl_array_release(&self->offered_mime_types);
    free(self);
}


/* Macros to reduce implementation boilerplate */

#define OFFER_HANDLER(type) \
static void type ## _offer_handler( \
    void *data, \
    struct type *proxy, \
    const char *mime_type \
) { \
    struct offer *self = data; \
    char *ptr = wl_array_add( \
        &self->offered_mime_types, \
        strlen(mime_type) + 1 \
    ); \
    strcpy(ptr, mime_type); \
}

#define LISTENER(type) \
static const struct type ## _listener type ## _listener = { \
    .offer = type ## _offer_handler \
};

#define INIT(type) \
void offer_init_ ## type(struct offer *self) { \
    self->do_receive = \
        (void (*)(struct wl_proxy *, const char *, int)) type ## _receive; \
    self->do_destroy = (void (*)(struct wl_proxy *)) type ## _destroy; \
    wl_array_init(&self->offered_mime_types); \
    struct type *proxy = (struct type *) self->proxy; \
    type ## _add_listener(proxy, &type ## _listener, self); \
}


/* Core Wayland implementation */

OFFER_HANDLER(wl_data_offer)
LISTENER(wl_data_offer)
INIT(wl_data_offer)


/* gtk-primary-selection implementation */

#ifdef HAVE_GTK_PRIMARY_SELECTION

OFFER_HANDLER(gtk_primary_selection_offer)
LISTENER(gtk_primary_selection_offer)
INIT(gtk_primary_selection_offer)

#endif /* HAVE_GTK_PRIMARY_SELECTION */


/* wp-primary-selection implementation */

#ifdef HAVE_WP_PRIMARY_SELECTION

OFFER_HANDLER(zwp_primary_selection_offer_v1)
LISTENER(zwp_primary_selection_offer_v1)
INIT(zwp_primary_selection_offer_v1)

#endif /* HAVE_WP_PRIMARY_SELECTION */


/* wlr-data-control implementation */

#ifdef HAVE_WLR_DATA_CONTROL

OFFER_HANDLER(zwlr_data_control_offer_v1)
LISTENER(zwlr_data_control_offer_v1)
INIT(zwlr_data_control_offer_v1)

#endif /* HAVE_WLR_DATA_CONTROL */


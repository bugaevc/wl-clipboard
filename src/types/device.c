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

#include "config.h"
#include "types/device.h"
#include "types/offer.h"
#include "types/source.h"
#include "includes/selection-protocols.h"

#include <wayland-client.h>
#include <stdlib.h>

int device_supports_selection(struct device *self, int primary) {
    return self->supports_selection(self, primary);
}

void device_set_selection(
    struct device *self,
    struct source *source,
    uint32_t serial,
    int primary
) {
    self->do_set_selection(self, source, serial, primary);
}

/* Macros to reduce implementation boilerplate */

#define SUPPORTS_SELECTION(type, expr) \
static int device_supports_selection_on_ ## type(struct device *self, int primary) { \
    return expr; \
}

#define SET_SELECTION_IMPL(type, source_type, impl) \
static void device_set_selection_on_ ## type( \
    struct device *self, \
    struct source *source_wrapper, \
    uint32_t serial, \
    int primary \
) { \
    struct type *device = (struct type *) self->proxy; \
    struct source_type *source = NULL; \
    if (source_wrapper != NULL) { \
        source = (struct source_type *) source_wrapper->proxy; \
    } \
    impl \
}

#define DATA_OFFER_HANDLER(type, offer_type) \
static void type ## _data_offer_handler( \
    void *data, \
    struct type *device, \
    struct offer_type *offer_proxy \
) { \
    struct device *self = data; \
    struct offer *offer = calloc(1, sizeof(struct offer)); \
    offer->proxy = (struct wl_proxy *) offer_proxy; \
    offer_init_ ## offer_type(offer); \
    if (self->new_offer_callback) { \
        self->new_offer_callback(offer); \
    } \
}

#define SELECTION_HANDLER(type, offer_type, selection_event, primary) \
static void type ## _ ## selection_event ## _handler( \
    void *data, \
    struct type *device, \
    struct offer_type *offer_proxy \
) { \
    struct device *self = data; \
    struct offer *offer = NULL; \
    if (offer_proxy != NULL) { \
        offer = wl_proxy_get_user_data((struct wl_proxy *) offer_proxy); \
    } \
    if (self->selection_callback) { \
        self->selection_callback(offer, primary); \
    } \
}

#define INIT(type, needs_surface) \
void device_init_ ## type(struct device *self) { \
    struct type *device = (struct type *) self->proxy; \
    type ## _add_listener(device, &type ## _listener, self); \
    self->supports_selection = device_supports_selection_on_ ## type; \
    self->needs_popup_surface = needs_surface; \
    self->do_set_selection = device_set_selection_on_ ## type; \
}


/* Core Wayland implementation */

SUPPORTS_SELECTION(wl_data_device, !primary)

SET_SELECTION_IMPL(wl_data_device, wl_data_source, {
    wl_data_device_set_selection(device, source, serial);
})

DATA_OFFER_HANDLER(wl_data_device, wl_data_offer)

SELECTION_HANDLER(wl_data_device, wl_data_offer, selection, 0)

static const struct wl_data_device_listener wl_data_device_listener = {
    .data_offer = wl_data_device_data_offer_handler,
    .selection = wl_data_device_selection_handler
};

INIT(wl_data_device, 1)


/* gtk-primary-selection implementation */

#ifdef HAVE_GTK_PRIMARY_SELECTION

SUPPORTS_SELECTION(gtk_primary_selection_device, primary)

SET_SELECTION_IMPL(gtk_primary_selection_device, gtk_primary_selection_source, {
    gtk_primary_selection_device_set_selection(device, source, serial);
})

DATA_OFFER_HANDLER(gtk_primary_selection_device, gtk_primary_selection_offer)

SELECTION_HANDLER(
    gtk_primary_selection_device,
    gtk_primary_selection_offer,
    selection,
    1
)

static const struct gtk_primary_selection_device_listener
gtk_primary_selection_device_listener = {
    .data_offer = gtk_primary_selection_device_data_offer_handler,
    .selection = gtk_primary_selection_device_selection_handler
};

INIT(gtk_primary_selection_device, 1)

#endif /* HAVE_GTK_PRIMARY_SELECTION */


/* wp-primary-selection implementation */

#ifdef HAVE_WP_PRIMARY_SELECTION

SUPPORTS_SELECTION(zwp_primary_selection_device_v1, primary)

SET_SELECTION_IMPL(
    zwp_primary_selection_device_v1,
    zwp_primary_selection_source_v1,
    {
        zwp_primary_selection_device_v1_set_selection(device, source, serial);
    }
)

DATA_OFFER_HANDLER(
    zwp_primary_selection_device_v1,
    zwp_primary_selection_offer_v1
)

SELECTION_HANDLER(
    zwp_primary_selection_device_v1,
    zwp_primary_selection_offer_v1,
    selection,
    1
)

static const struct zwp_primary_selection_device_v1_listener
zwp_primary_selection_device_v1_listener = {
    .data_offer = zwp_primary_selection_device_v1_data_offer_handler,
    .selection = zwp_primary_selection_device_v1_selection_handler
};

INIT(zwp_primary_selection_device_v1, 1)

#endif /* HAVE_WP_PRIMARY_SELECTION */


/* wlr-data-control implementation */

#ifdef HAVE_WLR_DATA_CONTROL

enum TriState {
    Unknown,
    Yes,
    No
};

/* Whether wlr-data-control supports primary selection */
static enum TriState device_wlr_supports_primary_selection = Unknown;
static int device_get_wlr_supports_selection(struct device *self, int primary) {
    if (!primary) {
        return 1;
    }

    if (device_wlr_supports_primary_selection == Yes) {
        return 1;
    } else if (device_wlr_supports_primary_selection == No) {
        return 0;
    }

    wl_display_roundtrip(self->wl_display);

    if (device_wlr_supports_primary_selection == Yes) {
        return 1;
    } else {
        device_wlr_supports_primary_selection = No;
        return 0;
    }
}

SUPPORTS_SELECTION(
    zwlr_data_control_device_v1,
    device_get_wlr_supports_selection(self, primary)
)

SET_SELECTION_IMPL(zwlr_data_control_device_v1, zwlr_data_control_source_v1, {
    if (!primary) {
        zwlr_data_control_device_v1_set_selection(device, source);
    } else {
        zwlr_data_control_device_v1_set_primary_selection(device, source);
    }
})

DATA_OFFER_HANDLER(zwlr_data_control_device_v1, zwlr_data_control_offer_v1)

SELECTION_HANDLER(
    zwlr_data_control_device_v1,
    zwlr_data_control_offer_v1,
    selection,
    0
)

static void zwlr_data_control_device_v1_primary_selection_handler(
    void *data,
    struct zwlr_data_control_device_v1 *device,
    struct zwlr_data_control_offer_v1 *offer_proxy
) {
    device_wlr_supports_primary_selection = Yes;
    struct device *self = data;
    struct offer *offer = NULL;
    if (offer_proxy != NULL) {
        offer = wl_proxy_get_user_data((struct wl_proxy *) offer_proxy);
    }
    if (self->selection_callback != NULL) {
        self->selection_callback(offer, 1);
    }
}

static const struct zwlr_data_control_device_v1_listener
zwlr_data_control_device_v1_listener = {
    .data_offer = zwlr_data_control_device_v1_data_offer_handler,
    .selection = zwlr_data_control_device_v1_selection_handler,
    .primary_selection = zwlr_data_control_device_v1_primary_selection_handler
};

INIT(zwlr_data_control_device_v1, 0)

#endif /* HAVE_WLR_DATA_CONTROL */

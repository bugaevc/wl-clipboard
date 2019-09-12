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
#include "includes/selection-protocols.h"
#include "types/source.h"
#include "util/string.h"

#include <unistd.h>

void source_offer(struct source *self, char *mime_type) {
    self->do_offer(self->proxy, mime_type);
}


/* Macros to reduce implementation boilerplate */

#define SEND_HANDLER(type) \
static void type ## _send_handler( \
    void *data, \
    struct type *proxy, \
    const char *mime_type, \
    int fd \
) { \
    struct source *self = data; \
    if (self->send_callback != NULL) { \
        self->send_callback(self, mime_type, fd); \
    } else { \
        close(fd); \
    } \
}

#define CANCELLED_HANDLER(type) \
static void type ## _cancelled_handler( \
    void *data, \
    struct type *proxy \
) { \
    struct source *self = data; \
    if (self->cancelled_callback != NULL) { \
        self->cancelled_callback(self); \
    } \
}

#define LISTENER(type) \
static const struct type ## _listener type ## _listener = { \
    .send = type ## _send_handler, \
    .cancelled = type ## _cancelled_handler \
};

#define INIT(type) \
void source_init_ ## type(struct source *self) { \
    self->do_offer = \
        (void (*)(struct wl_proxy *, const char *)) type ## _offer; \
    struct type *proxy = (struct type *) self->proxy; \
    type ## _add_listener(proxy, &type ## _listener, self); \
}


/* Core Wayland implementation */

static void wl_data_source_target_handler
(
    void *data,
    struct wl_data_source *wl_data_source,
    const char *mime_type
) {}

SEND_HANDLER(wl_data_source)
CANCELLED_HANDLER(wl_data_source)

static const struct wl_data_source_listener wl_data_source_listener = {
    .target = wl_data_source_target_handler,
    .send = wl_data_source_send_handler,
    .cancelled = wl_data_source_cancelled_handler
};

INIT(wl_data_source)


/* gtk-primary-selection implementation */

#ifdef HAVE_GTK_PRIMARY_SELECTION

SEND_HANDLER(gtk_primary_selection_source)
CANCELLED_HANDLER(gtk_primary_selection_source)
LISTENER(gtk_primary_selection_source)
INIT(gtk_primary_selection_source)

#endif /* HAVE_GTK_PRIMARY_SELECTION */


/* wp-primary-selection implementation */

#ifdef HAVE_WP_PRIMARY_SELECTION

SEND_HANDLER(zwp_primary_selection_source_v1)
CANCELLED_HANDLER(zwp_primary_selection_source_v1)
LISTENER(zwp_primary_selection_source_v1)
INIT(zwp_primary_selection_source_v1)

#endif /* HAVE_WP_PRIMARY_SELECTION */


/* wlr-data-control implementation */

#ifdef HAVE_WLR_DATA_CONTROL

SEND_HANDLER(zwlr_data_control_source_v1)
CANCELLED_HANDLER(zwlr_data_control_source_v1)
LISTENER(zwlr_data_control_source_v1)
INIT(zwlr_data_control_source_v1)

#endif /* HAVE_WLR_DATA_CONTROL */


/* wl-clipboard
 *
 * Copyright © 2018-2026 Sergey Bugaev <bugaevc@gmail.com>
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

#ifndef TYPES_COPY_ACTION_H
#define TYPES_COPY_ACTION_H

#include "util/string.h"

#include <stddef.h>

struct device;
struct source;
struct popup_surface;

struct copy_action {
    /* These fields are initialized by the creator */
    struct device *device;
    struct source *source;
    struct popup_surface *popup_surface;
    int primary;
    int sensitive;

    void (*did_set_selection_callback)(struct copy_action *self);
    void (*pasted_callback)(struct copy_action *self);
    void (*cancelled_callback)(struct copy_action *self);

    /* May be -1 if clipboard is empty */
    int fd_to_copy_from;
};

void copy_action_init(struct copy_action *self);

#endif /* TYPES_COPY_ACTION_H */

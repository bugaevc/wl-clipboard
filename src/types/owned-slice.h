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

#ifndef TYPES_OWNED_SLICE_H
#define TYPES_OWNED_SLICE_H

#include <stddef.h>

struct owned_slice {
    void (*destroy)(struct owned_slice*);

    char*  ptr;
    size_t len;
};


void owned_slice_steal(struct owned_slice* dest, struct owned_slice* src);

#endif /* UTIL_FILES_H */

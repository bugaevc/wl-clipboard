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

#ifndef TYPES_COPY_SOURCE_H
#define TYPES_COPY_SOURCE_H

#include <stddef.h>

struct copy_source;
typedef void (*copy_source_copy_t)(int, struct copy_source*);

struct copy_source {
    void (*copy)(int, struct copy_source*);
    void (*destroy)(struct copy_source*);
};

#endif /* TYPES_COPY_ACTION_H */

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

struct buffer {
    void (*destroy)(struct buffer*);

    char*  ptr;
    size_t len;
};

/// @brief Steal buffer to dest from src, leaving the latter empty
/// @param[out] dest Uninitialised (or empty) buffer
/// @param[in]  src  Initialised and valid buffer
/// @note It is safe to call .destroy on src after stealing
/// @warning Does not check anything
void buffer_steal(struct buffer* dest, struct buffer* src);

#endif /* UTIL_FILES_H */

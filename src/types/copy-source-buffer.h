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

#ifndef TYPES_COPY_SOURCE_SLICE_H
#define TYPES_COPY_SOURCE_SLICE_H

#include <types/copy-source.h>
#include <types/buffer.h>

#include <stddef.h>

/// @brief Copy source, serving data from the provided buffer
struct copy_source_buffer {
    struct copy_source impl;

    struct buffer slice;
};

/// @brief Initialise copy source from buffer, stealing it from provided ptr
/// @param[out] self
/// @param[in]  src
/// @return 0 on success, negative value otherwise
int copy_source_buffer_init(struct copy_source_buffer* self, struct buffer* src);

#endif /* TYPES_COPY_ACTION_H */

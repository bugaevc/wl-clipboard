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

#ifndef TYPES_COPY_SOURCE_ARGV_H
#define TYPES_COPY_SOURCE_ARGV_H

#include "../util/string.h"
#include "copy-source.h"

#include <stddef.h>

struct copy_source_argv {
    struct copy_source impl;

    argv_t argv;
};

int copy_source_argv_init(struct copy_source_argv* self, argv_t argv);

#endif /* TYPES_COPY_ACTION_H */

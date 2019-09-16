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

#ifndef UTIL_MISC_H
#define UTIL_MISC_H

#include <stdio.h>
#include <stdlib.h>

#define bail(message) do { fprintf(stderr, message "\n"); exit(1); } while (0)

void print_version_info(void);

void complain_about_selection_support(int primary);
void complain_about_watch_mode_support(void);

#endif /* UTIL_MISC_H */

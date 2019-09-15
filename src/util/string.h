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

#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#define text_plain "text/plain"
#define text_plain_utf8 "text/plain;charset=utf-8"

typedef char * const *argv_t;

int mime_type_is_text(const char *mime_type);

int str_has_prefix(const char *string, const char *prefix);
int str_has_suffix(const char *string, const char *suffix);

const char *get_file_extension(const char *file_path);

#endif /* UTIL_STRING_H */

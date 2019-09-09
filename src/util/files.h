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

#ifndef UTIL_FILES_H
#define UTIL_FILES_H

int create_anonymous_file(void);

void trim_trailing_newline(const char *file_path);

/* These functions return owned strings, so make sure
 * to free() their return values when done with them.
 */

char *path_for_fd(int fd);
char *infer_mime_type_from_contents(const char *file_path);
char *infer_mime_type_from_name(const char *file_path);

/* Returns the name of a new file */
char *dump_stdin_into_a_temp_file(void);

#endif /* UTIL_FILES_H */

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

#include <stddef.h>
#include "../types/buffer.h"

struct anonfile {
    void (*destroy)(struct anonfile*);
    void* ctx;

    int fd;
};

/// @brief Create anonymous file
/// @param[out] file
/// @return 0 on success, negative value otherwise
int create_anonymous_file(struct anonfile* file);

void trim_trailing_newline(const char *file_path);

/* These functions return owned strings, so make sure
 * to free() their return values when done with them.
 */

char *path_for_fd(int fd);
char *infer_mime_type_from_contents(const char *file_path);
char *infer_mime_type_from_name(const char *file_path);

/// @brief Creates a temporary file and writes stdin contents into it
/// @param[out] fd   Temporary file descriptor
/// @param[out] path Path to created file
/// @return 0 on success, negative value otherwise
/// @note Use `free` to free `path`.
int dump_stdin_into_a_temp_file(int* fd, char** path);

/* Copies stdin to memory buffer */
int copy_stdin_to_mem(struct buffer* slice);

/// @brief Attempts to mmap file `fd`.
/// @param[out] self Uninitialised valid buffer
/// @param[in]  fd   Valid file descriptor
/// @return 0 on success, -1 on failure and 1 if the fd is not mmappable
int buffer_mmap_file(struct buffer* self, int fd);

#endif /* UTIL_FILES_H */

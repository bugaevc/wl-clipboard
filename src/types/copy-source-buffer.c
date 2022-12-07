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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "copy-source-buffer.h"

static void copy(int fd, struct copy_source* self) {
    struct copy_source_buffer* self2 = (struct copy_source_buffer*)self;

    const char* ptr = self2->slice.ptr;
    size_t len = self2->slice.len;

    for (;;) {
        ssize_t bytes = write(fd, ptr, len);
        if (bytes < 0) {
            perror("copy_source_slice/copy: write");
            close(fd);
            exit(1);
        }
        len -= bytes;
        if (!bytes || !len) {
            break;
        }
        ptr += bytes;
    }

    char str[] = "\n";
    write(fd, &str, sizeof(str));

    if (close(fd)) {
        perror("copy_source_slice/copy: close");
    }
}

static void destroy(struct copy_source* self) {
    struct copy_source_buffer* self2 = (struct copy_source_buffer*)self;
    self2->slice.destroy(&self2->slice);
}


int copy_source_buffer_init(struct copy_source_buffer* self, struct buffer* slice) {
    if (!slice || !slice->ptr || !slice->len) {
        return -1;
    }

    self->impl.copy = copy;
    self->impl.destroy = destroy;

    buffer_steal(&self->slice, slice);
    return 0;
}


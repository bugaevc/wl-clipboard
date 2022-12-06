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

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "copy-source-file.h"
#include "../util/files.h"


static ssize_t write_(int fd, char* buf, size_t n) {
    char* const begin = buf;
    for (;;) {
        ssize_t written = write(fd, buf, n);
        if (written < 0) {
            perror("copy_source_file/copy: write");
            return -1;
        }
        buf += written;
        assert(n >= written);
        n -= written;
        if (n == 0) {
            return buf - begin;;
        }
    }
}

static ssize_t read_(int fd, char* buf, size_t n) {
    char* const begin = buf;
    for (;;) {
        ssize_t bytes = read(fd, buf, n);
        if (bytes < 0) {
            perror("copy_source_file/copy: read");
            return -1;
        }
        buf += bytes;
        assert(n >= bytes);
        n -= bytes;
        if (n == 0) {
            return buf - begin;
        }
    }
}

static int mmap_and_copy(int dest, int src) {
    struct owned_slice file;
    int err = owned_slice_mmap_file(&file, src);
    if (err) {
        return err;
    }
    err = write_(dest, file.ptr, file.len);
    file.destroy(&file);
    return err;
}


/* TODO: page size */
enum {
    BUFFER_SIZE = 4096
};

static void copy(int fd, struct copy_source* self) {
    struct copy_source_file* self2 = (struct copy_source_file*)self;

    int file = open(self2->fname, O_EXCL | O_RDONLY);
    if (file < 0) {
        perror("copy_source_file/copy: open");
        close(fd);
        return;
    }

    int err = mmap_and_copy(fd, file);
    if (err <= 0) {
        close(file);
        close(fd);
        return;
    }

    exit(49);

    char* buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("copy_source_file/copy: malloc");
        goto fin;
    }

    for (;;) {
        ssize_t bytes = read_(file, buffer, BUFFER_SIZE);
        if (bytes <= 0) {
            goto fin;
        }
        ssize_t written = write_(fd, buffer, bytes);
        if (written < 0) {
            goto fin;
        }
        assert(bytes == written);
    }

fin:
    if (buffer) {
        free(buffer);
    }
    close(file);
    close(fd);
}

void destroy(struct copy_source* self) {
    struct copy_source_file* self2 = (struct copy_source_file*)self;

    if (unlink(self2->fname) < 0) {
        perror("copy_source_file/destroy: unlink");
    }

    char* copy = strdup(self2->fname);
    const char* dir = dirname(copy);

    printf("removing dir=%s", dir);

    int r = dir || rmdir(dir);
    if (!r) {
        perror("copy_source_file/destroy: rmdir");
    }

    free(copy);
}


int copy_source_file_init(struct copy_source_file* self, const char* fname) {
    if (!fname) {
        return -1;
    }

    self->impl.copy = copy;
    self->impl.destroy = destroy;

    self->fname = fname;
    return 0;
}

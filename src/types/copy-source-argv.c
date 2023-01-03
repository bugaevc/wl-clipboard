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
#include <string.h>
#include <unistd.h>

#include "copy-source-argv.h"

static void copy(int fd, struct copy_source* self) {
    struct copy_source_argv* self2 = (struct copy_source_argv*)self;

    FILE* file = fdopen(fd, "w");
    if (!file) {
        perror("copy_source_argv/copy: fdopen");
        close(fd);
        return;
    }

    argv_t word = self2->argv;
    if (*word) {
        fwrite(*word, 1, strlen(*word), file);
        ++word;
    }
    for (; *word; ++word) {
        fputc(' ', file);
        fwrite(*word, 1, strlen(*word), file);
    }
    fclose(file);
}


int copy_source_argv_init(struct copy_source_argv* self, argv_t argv) {
    self->impl.copy = copy;
    if (!argv) {
        return -1;
    }
    self->argv = argv;
    return 0;
}


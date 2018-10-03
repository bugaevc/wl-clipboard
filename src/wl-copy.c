/* wl-clipboard
 *
 * Copyright © 2018 Sergey Bugaev <bugaevc@gmail.com>
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

#include "boilerplate.h"

const char **data_to_copy = NULL;
char *temp_file_to_copy = NULL;

void data_source_target_handler
(
    void *data,
    struct wl_data_source *data_source,
    const char *mime_type
) {}

void data_source_send_handler
(
    void *data,
    struct wl_data_source *data_source,
    const char *mime_type,
    int fd
) {
    if (data_to_copy != NULL) {
        // copy the specified data, separated by spaces
        const char **dataptr = data_to_copy;
        for (int is_first = 1; *dataptr != NULL; dataptr++, is_first = 0) {
            if (!is_first) {
                write(fd, " ", 1);
            }
            write(fd, dataptr, strlen(*dataptr));
        }
    } else {
        // copy from the temp file; for that, we delegate to a
        // (hopefully) highly optimized implementation of copying
        if (fork() == 0) {
            dup2(fd, STDOUT_FILENO);
            execlp("cat", "cat", temp_file_to_copy, NULL);
            // failed to execl
            perror("exec cat");
            exit(1);
        }
        wait(NULL);
    }
    close(fd);
}

void data_source_cancelled_handler
(
    void *data,
    struct wl_data_source *data_source
) {
    // we're done!
    if (temp_file_to_copy != NULL) {
        execlp("rm", "rm", "-r", dirname(temp_file_to_copy), NULL);
        perror("exec rm");
        exit(1);
    } else {
        exit(0);
    }
}

const struct wl_data_source_listener data_source_listener = {
    .target = data_source_target_handler,
    .send = data_source_send_handler,
    .cancelled = data_source_cancelled_handler
};

void offer_plain_text(struct wl_data_source *data_source) {
    wl_data_source_offer(data_source, "text/plain");
    wl_data_source_offer(data_source, "text/plain;charset=utf-8");
    wl_data_source_offer(data_source, "TEXT");
    wl_data_source_offer(data_source, "STRING");
    wl_data_source_offer(data_source, "UTF8_STRING");
}

int main(int argc, const char *argv[]) {

    char *mime_type = NULL;
    if (argc > 1) {
        // copy our command-line args
        data_to_copy = argv + 1;
    } else {
        // copy stdin
        mime_type = infer_mime_type_of_file(STDIN_FILENO);
        temp_file_to_copy = dump_into_a_temp_file(STDIN_FILENO);
    }

    init_wayland_globals();

    if (fork() != 0) {
        // exit in the parent, but leave the
        // child running in the background
        exit(0);
    }

    struct wl_data_source *data_source = wl_data_device_manager_create_data_source(data_device_manager);
    wl_data_source_add_listener(data_source, &data_source_listener, NULL);

    if (mime_type != NULL) {
        if (strcmp(mime_type, "text/plain") == 0) {
            offer_plain_text(data_source);
        } else {
            wl_data_source_offer(data_source, mime_type);
        }
        free(mime_type);
    } else {
        offer_plain_text(data_source);
    }

    wl_data_device_set_selection(data_device, data_source, get_serial());

    while (1) {
        wl_display_dispatch(display);
    }

    // never reached
    return 1;
}

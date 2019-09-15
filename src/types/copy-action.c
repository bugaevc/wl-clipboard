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

#include "types/copy-action.h"
#include "types/device.h"
#include "types/source.h"
#include "types/popup-surface.h"

#include "util/misc.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

static struct popup_surface *current_popup_surface = NULL;
static struct wl_array pending_selections;

static void on_focus(
    struct popup_surface *popup_surface,
    uint32_t serial
) {
    /* Set the pending selections */
    struct wl_display *wl_display = NULL;
    struct copy_action **ptr;
    wl_array_for_each(ptr, &pending_selections) {
        struct copy_action *self = *ptr;
        wl_display = self->device->wl_display;
        device_set_selection(self->device, self->source, serial, self->primary);
    }

    /* Make sure they reach the display
     * before we do anything else, such
     * as destroying the surface or exiting.
     */
    wl_display_roundtrip(wl_display);

    /* Destroy the surface */
    popup_surface_destroy(current_popup_surface);
    current_popup_surface = NULL;

    /* Invoke the callbacks */
    wl_array_for_each(ptr, &pending_selections) {
        struct copy_action *self = *ptr;
        if (self->did_set_selection_callback != NULL) {
            self->did_set_selection_callback(self);
        }
    }

    /* Finally, destroy the array */
    wl_array_release(&pending_selections);
}

static void do_send(struct source *source, const char *mime_type, int fd) {
    struct copy_action *self = source->data;

     /* Unset O_NONBLOCK */
    fcntl(fd, F_SETFL, 0);

    if (self->copy_source.file_path != NULL) {
        /* Copy the file to the given file descriptor
         * by spawning an appropriate cat process.
         */
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(fd);
            return;
        }
        if (pid == 0) {
            dup2(fd, STDOUT_FILENO);
            close(fd);
            execlp("cat", "cat", self->copy_source.file_path, NULL);
            perror("exec cat");
            exit(1);
        }
        close(fd);
        /* Wait for the cat process to exit. This effectively
         * means waiting for the other side to read the whole
         * file. In theory, a malicious client could perform a
         * denial-of-serivice attack against us. Perhaps we
         * should switch to an asynchronous child waiting scheme
         * instead.
         */
        wait(NULL);
    } else {
        /* We'll perform the copy ourselves */
        FILE *f = fdopen(fd, "w");
        if (f == NULL) {
            perror("fdopen");
            close(fd);
            return;
        }

        if (self->copy_source.data.ptr != NULL) {
            /* Just copy the given chunk of data */
            fwrite(
                self->copy_source.data.ptr,
                1,
                self->copy_source.data.len,
                f
            );
        } else if (self->copy_source.argv != NULL) {
            /* Copy an argv-style string array,
             * inserting spaces between items.
             */
            int is_first = 1;
            for (argv_t word = self->copy_source.argv; *word != NULL; word++) {
                if (!is_first) {
                    fwrite(" ", 1, 1, f);
                }
                is_first = 0;
                fwrite(*word, 1, strlen(*word), f);
            }
        } else {
            bail("Unreachable: nothing to copy");
        }

        fclose(f);
    }


    if (self->pasted_callback != NULL) {
        self->pasted_callback(self);
    }
}

static void forward_cancel(struct source *source) {
    struct copy_action *self = source->data;
    if (self->cancelled_callback != NULL) {
        self->cancelled_callback(self);
    }
}

void copy_action_init(struct copy_action *self) {
    if (self->source != NULL) {
        self->source->send_callback = do_send;
        self->source->cancelled_callback = forward_cancel;
        self->source->data = self;
    }

    /* See if we can just set the selection directly */
    if (!self->device->needs_popup_surface) {
        /* The simple case.
         * In this case, setting the selection
         * doesn't actually require a serial,
         * so passing zero will do.
         */
        device_set_selection(self->device, self->source, 0, self->primary);
        /* Make sure it reaches the display
         * before we do anything else.
         */
        wl_display_roundtrip(self->device->wl_display);
        /* And invoke the callback */
        if (self->did_set_selection_callback != NULL) {
            self->did_set_selection_callback(self);
        }
        return;
    }

    /* The complicated case.
     * Add our action to the list of the pending
     * actions to be done when the popup surface
     * gets foces, and initialize both the surface
     * and the array if needed.
     */
    if (current_popup_surface == NULL) {
        wl_array_init(&pending_selections);
    }
    struct copy_action **ptr = (struct copy_action **) wl_array_add(
        &pending_selections,
        sizeof(struct copy_action *)
    );
    *ptr = self;

    /* Only initialize the surface after
     * we've initialized the array, because
     * it might get keyboard focus immediately.
     */
    if (current_popup_surface == NULL) {
        current_popup_surface = self->popup_surface;
        current_popup_surface->on_focus = on_focus;
        current_popup_surface->data = self;
        popup_surface_init(current_popup_surface);
    }
}

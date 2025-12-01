/* wl-clipboard
 *
 * Copyright © 2018-2025 Sergey Bugaev <bugaevc@gmail.com>
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

#include "types/popup-surface.h"
#include "types/registry.h"
#include "types/seat.h"
#include "types/keyboard.h"
#include "types/shell.h"
#include "types/shell-surface.h"
#include "util/files.h"
#include "util/misc.h"

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

static void forward_on_focus(
    struct keyboard *keyboard,
    uint32_t serial
) {
    struct popup_surface *self = (struct popup_surface *) keyboard->data;
    if (self->on_focus != NULL) {
        self->on_focus(self, serial);
    }
}

void popup_surface_init(struct popup_surface *self) {
    self->shell = registry_find_shell(self->registry);
    if (self->shell == NULL) {
        complain_about_missing_shell();
    }

    self->keyboard = seat_get_keyboard(self->seat);
    if (self->keyboard == NULL) {
        bail("This seat has no keyboard");
    }
    self->keyboard->on_focus = forward_on_focus;
    self->keyboard->data = self;

    /* Make sure that we get the keyboard
     * object before we create the surface,
     * so that we get the enter event.
     */
    wl_display_dispatch(self->registry->wl_display);


    struct wl_compositor *wl_compositor = self->registry->wl_compositor;
    if (wl_compositor == NULL) {
        complain_about_missing_global("wl_compositor");
    }
    self->wl_surface = wl_compositor_create_surface(wl_compositor);
    self->shell_surface = shell_create_shell_surface(
        self->shell,
        self->wl_surface
    );

#ifdef HAVE_GTK_SHELL
    if (self->registry->gtk_shell1 != NULL) {
        self->gtk_surface = gtk_shell1_get_gtk_surface(
            self->registry->gtk_shell1,
            self->wl_surface
        );
    }
#endif

    /* Signal that the surface is ready to be configured */
    wl_surface_commit(self->wl_surface);
    wl_display_roundtrip(self->registry->wl_display);

    if (self->wl_surface == NULL) {
        /* It's possible that we were given focus
         * (without ever committing a buffer) during
         * the above roundtrip, in which case we have
         * already fired the callback and have likely
         * already destroyed the surface. No need to
         * do anything further in that case.
         */
        free(self);
        return;
    }

    /* Remember that after this point, we should
     * free() self when it gets destroyed.
     */
    self->should_free_self = 1;

    int width = 1;
    int height = 1;
    int stride = width * 4;
    int size = stride * height;

    /* Open an anonymous file and write some zero bytes to it */
    int fd = create_anonymous_file();
    int rc = ftruncate(fd, size);
    if (rc < 0) {
        perror("ftruncate");
    }

    /* Create a shared memory pool */
    struct wl_shm *wl_shm = self->registry->wl_shm;
    if (wl_shm == NULL) {
        complain_about_missing_global("wl_shm");
    }
    struct wl_shm_pool *wl_shm_pool = wl_shm_create_pool(wl_shm, fd, size);

    /* Allocate the buffer in that pool */
    struct wl_buffer *wl_buffer = wl_shm_pool_create_buffer(
        wl_shm_pool,
        0,
        width,
        height,
        stride,
        WL_SHM_FORMAT_ARGB8888
    );
    /* We're using ARGB, so zero bytes mean
     * a fully transparent pixel, which happens
     * to be exactly what we want.
     */

    wl_surface_attach(self->wl_surface, wl_buffer, 0, 0);
    wl_surface_damage(self->wl_surface, 0, 0, width, height);

    /* Ask the compositor nicely to give us focus */
#ifdef HAVE_GTK_SHELL
    if (self->gtk_surface != NULL) {
        gtk_surface1_present(self->gtk_surface, 0);
    }
#endif
#ifdef HAVE_XDG_ACTIVATION
    if (self->registry->xdg_activation_v1 != NULL) {
        /* See if someone was kind enough to leave
         * some tokens for us in the environment.
         */
        const char *token = getenv("XDG_ACTIVATION_TOKEN");
        if (token == NULL) {
            token = getenv("DESKTOP_STARTUP_ID");
        }
        if (token != NULL) {
            xdg_activation_v1_activate(
                self->registry->xdg_activation_v1,
                token,
                self->wl_surface
            );
        }
    }
#endif

    wl_surface_commit(self->wl_surface);
}

void popup_surface_destroy(struct popup_surface *self) {
    /* We cannot destroy the keyboard
     * because it acts as a listener,
     * and there's no way to reset that.
     * So just unreference ourselves.
     */
    self->keyboard->on_focus = NULL;
    self->keyboard->data = NULL;

    shell_surface_destroy(self->shell_surface);
#ifdef HAVE_GTK_SHELL
    if (self->gtk_surface != NULL) {
        if (
            gtk_surface1_get_version(self->gtk_surface) >=
            GTK_SURFACE1_RELEASE_SINCE_VERSION
        ) {
            gtk_surface1_release(self->gtk_surface);
        } else {
            gtk_surface1_destroy(self->gtk_surface);
        }
    }
#endif
    wl_surface_destroy(self->wl_surface);
    /* Let popup_surface_init() know we're already done */
    self->wl_surface = NULL;
    free(self->shell);

    if (self->should_free_self) {
        free(self);
    }
}

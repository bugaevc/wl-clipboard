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

#ifndef TYPES_POPUP_SURFACE_H
#define TYPES_POPUP_SURFACE_H

#include <wayland-client.h>

struct shell;
struct shell_surface;

struct popup_surface {
    /* These fields are initialized by the creator */
    struct wl_display *wl_display;
    struct wl_compositor *wl_compositor;
    struct wl_shm *wl_shm;
    struct shell *shell;

    /* These fields are initialized by the implementation */
    struct shell_surface *shell_surface;
    struct wl_surface *wl_surface;
    int should_free_self;
};

void popup_surface_init(struct popup_surface *self);
void popup_surface_destroy(struct popup_surface *self);

#endif /* TYPES_POPUP_SURFACE_H */

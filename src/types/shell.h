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

#ifndef TYPES_SHELL_H
#define TYPES_SHELL_H

#include "includes/shell-protocols.h"

#include <wayland-client.h>

struct shell_surface;

struct shell {
    /* This field is initialized by the creator */
    struct wl_proxy *proxy;

    /* This field is initialized by the implementation */
    struct shell_surface *(*do_create_shell_surface)(
        struct shell *self,
        struct wl_surface *surface
    );
};

struct shell_surface *shell_create_shell_surface(
    struct shell *self,
    struct wl_surface *surface
);


/* Initializers */

void shell_init_wl_shell(struct shell *self);

#ifdef HAVE_XDG_SHELL
void shell_init_xdg_shell(struct shell *self);
#endif

#endif /* TYPES_SHELL_H */

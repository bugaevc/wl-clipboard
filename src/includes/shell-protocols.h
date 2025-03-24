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

#ifndef INCLUDES_SHELL_PROTOCOLS_H
#define INCLUDES_SHELL_PROTOCOLS_H

#include "config.h"

#include <wayland-client.h>

#ifdef HAVE_XDG_SHELL
#    include "xdg-shell.h"
#endif

#ifdef HAVE_GTK_SHELL
#    include "gtk-shell.h"
#endif

/* Not strictly speaking a shell */
#ifdef HAVE_XDG_ACTIVATION
#    include "xdg-activation.h"
#endif

#endif /* INCLUDES_SHELL_PROTOCOLS_H */

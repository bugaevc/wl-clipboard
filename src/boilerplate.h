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

#include "config.h"

#include "util/string.h"
#include "util/files.h"
#include "util/misc.h"

#include <wayland-client.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h> // isupper
#include <fcntl.h> // open
#include <sys/stat.h> // open
#include <sys/types.h> // open
#include <stdlib.h> // exit
#include <libgen.h> // basename
#include <sys/wait.h>
#include <limits.h> // PATH_MAX

#include "includes/shell-protocols.h"
#include "includes/selection-protocols.h"

struct wl_display *display;
struct wl_data_device_manager *data_device_manager;
struct wl_seat *seat;
struct wl_compositor *compositor;
struct wl_shm *shm;
struct wl_shell *shell;
struct wl_surface *surface;
struct wl_shell_surface *shell_surface;

#ifdef HAVE_XDG_SHELL
struct xdg_wm_base *xdg_wm_base;
struct xdg_surface *xdg_surface;
struct xdg_toplevel *xdg_toplevel;
#endif

struct wl_data_device *data_device;

#ifdef HAVE_GTK_PRIMARY_SELECTION
struct gtk_primary_selection_device_manager *gtk_primary_selection_device_manager;
struct gtk_primary_selection_device *gtk_primary_selection_device;
#endif

#ifdef HAVE_WP_PRIMARY_SELECTION
struct zwp_primary_selection_device_manager_v1 *primary_selection_device_manager;
struct zwp_primary_selection_device_v1 *primary_selection_device;
#endif

#ifdef HAVE_WLR_DATA_CONTROL
struct zwlr_data_control_manager_v1 *data_control_manager;
struct zwlr_data_control_device_v1 *data_control_device;
#endif

const char *requested_seat_name;

void init_wayland_globals(void);
int use_wlr_data_control;

void popup_tiny_invisible_surface(void);
void destroy_popup_surface(void);

void (*action_on_popup_surface_getting_focus)(uint32_t serial);
void (*action_on_no_keyboard)(void);

void ensure_has_primary_selection(void);

uint32_t get_serial(void);

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

#include "config.h"

#include <wayland-client.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h> // strcmp
#include <ctype.h> // isupper
#include <fcntl.h> // open
#include <sys/stat.h> // open
#include <sys/types.h> // open
#include <stdlib.h> // exit
#include <unistd.h> // execl, STDOUT_FILENO
#include <libgen.h> // basename
#include <sys/wait.h>
#include <sys/syscall.h> // syscall, SYS_memfd_create
#include <linux/limits.h> // PATH_MAX

#ifdef HAVE_XDG_SHELL
#    include "xdg-shell-client.h"
#endif

#ifdef HAVE_GTK_PRIMARY_SELECTION
#    include "gtk-primary-selection.h"
#endif

#define bail(message) do { fprintf(stderr, message "\n"); exit(1); } while (0)

#define text_plain "text/plain"

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
struct gtk_primary_selection_device_manager *primary_selection_device_manager;
struct gtk_primary_selection_device *primary_selection_device;
#endif

void init_wayland_globals(void);

void popup_tiny_invisible_surface(void);
void destroy_popup_surface(void);

void (*action_on_popup_surface_getting_focus)(uint32_t serial);
void (*action_on_no_keyboard)(void);

int get_serial(void);

int mime_type_is_text(const char *mime_type);
int str_has_prefix(const char *string, const char *prefix);

// functions below this line return owned strings,
// free() their return values when done with them

char *path_for_fd(int fd);
char *infer_mime_type_from_contents(const char *file_path);
char *infer_mime_type_from_name(const char *file_path);

// returns the name of a new file
char *dump_into_a_temp_file(int fd);

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

#ifdef HAVE_MEMFD
#    include <sys/syscall.h> // syscall, SYS_memfd_create
#endif
#ifdef HAVE_SHM_ANON
#    include <sys/mman.h> // shm_open, SHM_ANON
#endif


#ifdef HAVE_XDG_SHELL
#    include "xdg-shell.h"
#endif

#ifdef HAVE_WP_PRIMARY_SELECTION
#    include "wp-primary-selection.h"
#endif

#ifdef HAVE_WLR_LAYER_SHELL
#    include "wlr-layer-shell.h"
#endif

#ifdef HAVE_GTK_PRIMARY_SELECTION
#    include "gtk-primary-selection.h"
#endif

#ifdef HAVE_WLR_DATA_CONTROL
#    include "wlr-data-control.h"
#endif

#define bail(message) do { fprintf(stderr, message "\n"); exit(1); } while (0)

#define text_plain "text/plain"
#define text_plain_utf8 "text/plain;charset=utf-8"

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

#ifdef HAVE_WLR_LAYER_SHELL
struct zwlr_layer_shell_v1 *layer_shell;
struct zwlr_layer_surface_v1 *layer_surface;
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

void process_new_seat(struct wl_seat *new_seat);
const char *requested_seat_name;

void init_wayland_globals(void);
int use_wlr_data_control;

void popup_tiny_invisible_surface(void);
void destroy_popup_surface(void);

void (*action_on_popup_surface_getting_focus)(uint32_t serial);
void (*action_on_no_keyboard)(void);

void ensure_has_primary_selection(void);

int get_serial(void);

int mime_type_is_text(const char *mime_type);
int str_has_prefix(const char *string, const char *prefix);
int str_has_suffix(const char *string, const char *suffix);

void print_version_info(void);

void trim_trailing_newline(const char *file_path);

// functions below this line return owned strings,
// free() their return values when done with them

char *path_for_fd(int fd);
char *infer_mime_type_from_contents(const char *file_path);
char *infer_mime_type_from_name(const char *file_path);

// returns the name of a new file
char *dump_stdin_into_a_temp_file(void);

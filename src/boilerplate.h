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

#include <wayland-client.h>
#include <stdio.h>
#include <string.h> // strcmp
#include <fcntl.h> // open
#include <sys/stat.h> // open
#include <sys/types.h> // open
#include <stdlib.h> // exit
#include <unistd.h> // execl, STDOUT_FILENO
#include <sys/wait.h>
#include <sys/syscall.h> // syscall, SYS_memfd_create
#include <linux/limits.h> // PATH_MAX

#define bail(message) do { fprintf(stderr, message "\n"); exit(1); } while (0)

struct wl_display *display;
struct wl_data_device_manager *data_device_manager;
struct wl_seat *seat;
struct wl_compositor *compositor;
struct wl_shm *shm;
struct wl_shell *shell;

struct wl_data_device *data_device;

void init_wayland_globals(void);

struct wl_surface *popup_tiny_invisible_surface(void);

int get_serial();

// free when done
char *infer_mime_type_of_file(int fd);

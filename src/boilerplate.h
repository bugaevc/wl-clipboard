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

#include "types/registry.h"
#include "types/seat.h"

struct wl_display *display;
struct registry *registry;

struct seat *seat;
const char *requested_seat_name;

void init_wayland_globals(void);

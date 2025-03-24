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

#include "config.h"
#include "util/misc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void print_version_info() {
    printf(
        "wl-clipboard " PROJECT_VERSION "\n"
        "Copyright (C) 2018-2025 Sergey Bugaev\n"
        "License GPLv3+: GNU GPL version 3 or later"
        " <https://gnu.org/licenses/gpl.html>.\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n"
    );
}

void complain_about_selection_support(int primary) {
    if (!primary) {
        /* We always expect to find at least wl_data_device_manager */
        complain_about_missing_global("wl_data_device_manager");
    }

#if !defined(HAVE_WP_PRIMARY_SELECTION) && !defined(HAVE_GTK_PRIMARY_SELECTION)
    bail("wl-clipboard was built without primary selection support");
#endif

    bail("The compositor does not seem to support primary selection");
}

void complain_about_watch_mode_support() {
#ifdef HAVE_WLR_DATA_CONTROL
    bail(
        "Watch mode requires a compositor that supports"
        " the wlroots data-control protocol"
    );
#else
    bail(
        "wl-clipboard was built without support for"
        " the wlroots data-control protocol"
    );
#endif
}

void complain_about_wayland_connection() {
    int saved_errno = errno;
    fprintf(stderr, "Failed to connect to a Wayland server");
    if (saved_errno != 0) {
        fprintf(stderr, ": %s", strerror(saved_errno));
    }
    fputc('\n', stderr);

    const char *display = getenv("WAYLAND_DISPLAY");
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (display != NULL) {
        fprintf(stderr, "Note: WAYLAND_DISPLAY is set to %s\n", display);
    } else {
        fprintf(
            stderr,
            "Note: WAYLAND_DISPLAY is unset"
            " (falling back to wayland-0)\n"
        );
        display = "wayland-0";
    }
    if (runtime_dir != NULL) {
        fprintf(stderr, "Note: XDG_RUNTIME_DIR is set to %s\n", runtime_dir);
    } else {
        fprintf(stderr, "Note: XDG_RUNTIME_DIR is unset\n");
    }
    if (display[0] != '/' && runtime_dir != NULL) {
        fprintf(
            stderr,
            "Please check whether %s/%s socket exists and is accessible.\n",
            runtime_dir,
            display
        );
    }
    exit(1);
}

void complain_about_missing_seat(const char *seat_name) {
    if (seat_name != NULL) {
        fprintf(stderr, "No such seat: %s\n", seat_name);
    } else {
        complain_about_missing_global("seat");
    }
    exit(1);
}

void complain_about_missing_global(const char *global) {
    fprintf(
        stderr,
        "The compositor does not seem to implement %s,"
        " which is required for wl-clipboard to work\n",
        global
    );
    exit(1);
}

void complain_about_missing_shell() {
#ifndef HAVE_XDG_SHELL
    fprintf(
        stderr,
        "wl-clipboard was built without xdg-shell support, and the compositor"
        " does not seem to support any other Wayland shell.\nPlease rebuild"
        " wl-clipboard with xdg-shell support.\n"
    );
    exit(1);
#endif


    fprintf(
        stderr,
        "The compositor does not seem to implement a Wayland shell,"
        " which is required for wl-clipboard to work\n"
    );

    exit(1);
}

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
#include "util/misc.h"

#include <stdio.h>

void print_version_info() {
    printf(
        "wl-clipboard " PROJECT_VERSION "\n"
        "Copyright (C) 2019 Sergey Bugaev\n"
        "License GPLv3+: GNU GPL version 3 or later"
        " <https://gnu.org/licenses/gpl.html>.\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n"
    );
}

void complain_about_selection_support(int primary) {
    if (!primary) {
        /* We always expect to find at least wl_data_device_manager */
        bail("Missing a required global object");
    }

#if !defined(HAVE_WP_PRIMARY_SELECTION) && !defined(HAVE_GTK_PRIMARY_SELECTION)
    bail("wl-clipboard was built without primary selection support");
#endif

    bail("The compositor does not seem to support primary selection");
}

void complain_about_watch_mode_support() {
#ifdef HAVE_WLR_DATA_CONTROL
    bail(
        "Watch mode requires a compositor that supports "
        "wlroots data-control protocol"
    );
#else
    bail(
        "wl-clipboard was built without wlroots data-control protocol support"
    );
#endif
}

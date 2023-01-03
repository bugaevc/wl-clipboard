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

#include <types/sensitive-section.h>
#include <signal.h>
#include <stdio.h>

/* "Good enough" signal handler. It's not hard to break, but it works for the most common cases */

static struct sensitive* current_handler = NULL;

static const int sigs[] = { SIGINT };

static void handler_impl(int signal) {
    (void)signal;

    if (current_handler) {
        current_handler->cleanup(current_handler);
    }
}

int sensitive_begin(struct sensitive* handler) {
    // we don't use multithreading, so we don't care about races
    if (current_handler) {
        return -1;
    }
    current_handler = handler;

    const int* sig = sigs;
    const int* const end = sigs + sizeof( sigs ) / sizeof( *sigs );
    for (; sig != end; ++sig) {
        if (signal(*sig, handler_impl) == SIG_ERR) {
            perror("signal");
            goto restore;
        }
    }

    return 0;

restore:
    --sig;
    for (; sig != (sigs - 1); --sig) {
        signal(*sig, SIG_DFL);
    }
    return -1;
}

/// @brief End "sensitive" section
/// @param[in] handler Emergency handler
int sensitive_end(struct sensitive* handler) {
    (void)handler;

    signal(SIGKILL, SIG_DFL);
    signal(SIGINT, SIG_DFL);

    current_handler = NULL;
    return 0;
}

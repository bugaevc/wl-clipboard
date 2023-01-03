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

#ifndef TYPES_SENSITIVE_SECTION_H
#define TYPES_SENSITIVE_SECTION_H

struct sensitive {
    void (*cleanup)(struct sensitive*);
};

/// @brief Begin "sensitive" section
/// @param[in] handler Emergency handler, invoked on signal
/// @return 0 on success, negative value otherwise
int sensitive_begin(struct sensitive* handler);

/// @brief End "sensitive" section
/// @param[in] handler Emergency handler, invoked on signal
/// @return 0 on success, negative value otherwise
int sensitive_end(struct sensitive* handler);

#endif

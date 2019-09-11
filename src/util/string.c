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

#include "util/string.h"

#include <string.h>
#include <stdlib.h>

int mime_type_is_text(const char *mime_type) {
    /* A heuristic to detect plain text mime types */

    /* Types that explicitly declare they're textual */
    int basic
        = str_has_prefix(mime_type, "text/")
        || strcmp(mime_type, "TEXT") == 0
        || strcmp(mime_type, "STRING") == 0
        || strcmp(mime_type, "UTF8_STRING") == 0;

    /* Common script and markup types */
    int common
        = strstr(mime_type, "json") != NULL
        || str_has_suffix(mime_type, "script")
        || str_has_suffix(mime_type, "xml")
        || str_has_suffix(mime_type, "yaml");

    /* Special-case PGP and SSH keys.
     * A public SSH key is typically stored
     * in a file that has a name similar to
     * id_rsa.pub, which xdg-mime misidentifies
     * as being a Publisher file. Note that it
     * handles private keys, which do not have
     * a .pub extension, correctly.
     */
    int special
        = strstr(mime_type, "application/vnd.ms-publisher") != NULL
        || str_has_suffix(mime_type, "pgp-keys");

    return basic || common || special;
}

int str_has_prefix(const char *string, const char *prefix) {
    size_t prefix_length = strlen(prefix);
    return strncmp(string, prefix, prefix_length) == 0;
}

int str_has_suffix(const char *string, const char *suffix) {
    size_t string_length = strlen(string);
    size_t suffix_length = strlen(suffix);
    if (string_length < suffix_length) {
        return 0;
    }
    size_t offset = string_length - suffix_length;
    return strcmp(string + offset, suffix) == 0;
}

const char *get_file_extension(const char *file_path) {
    const char *name = strrchr(file_path, '/');
    if (name == NULL) {
        name = file_path;
    }
    const char *ext = strrchr(name, '.');
    if (ext == NULL) {
        return NULL;
    }
    return ext + 1;
}

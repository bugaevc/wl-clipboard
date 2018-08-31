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

#include "boilerplate.h"

void registry_global_handler
(
    void *data,
    struct wl_registry *registry,
    uint32_t name,
    const char *interface,
    uint32_t version
) {
    if (strcmp(interface, "wl_data_device_manager") == 0) {
        data_device_manager = wl_registry_bind(
            registry,
            name,
            &wl_data_device_manager_interface,
            1
        );
    } else if (strcmp(interface, "wl_seat") == 0) {
        seat = wl_registry_bind(
            registry,
            name,
            &wl_seat_interface,
            5
        );
    } else if (strcmp(interface, "wl_compositor") == 0) {
        compositor = wl_registry_bind(
            registry,
            name,
            &wl_compositor_interface,
            3
        );
    } else if (strcmp(interface, "wl_shm") == 0) {
        shm = wl_registry_bind(
            registry,
            name,
            &wl_shm_interface,
            1
        );
    } else if (strcmp(interface, "wl_shell") == 0) {
        shell = wl_registry_bind(
            registry,
            name,
            &wl_shell_interface,
            1
        );
    }
}

void registry_global_remove_handler
(
    void *data,
    struct wl_registry *registry,
    uint32_t name
) {}



const struct wl_registry_listener registry_listener = {
    .global = registry_global_handler,
    .global_remove = registry_global_remove_handler
};


void shell_surface_ping
(
    void *data,
    struct wl_shell_surface *shell_surface,
    uint32_t serial
) {
    wl_shell_surface_pong(shell_surface, serial);
}

void shell_surface_configure
(
    void *data,
    struct wl_shell_surface *shell_surface,
    uint32_t edges,
    int32_t width,
    int32_t height
) {}

void shell_surface_popup_done
(
    void *data,
    struct wl_shell_surface *shell_surface
) {}

const struct wl_shell_surface_listener shell_surface_listener = {
    .ping = shell_surface_ping,
    .configure = shell_surface_configure,
    .popup_done = shell_surface_popup_done
};

void init_wayland_globals() {
        display = wl_display_connect(NULL);
    if (display == NULL) {
        bail("Failed to connect to a Wayland server");
    }

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    // wait for the "initial" set of globals to appear
    wl_display_roundtrip(display);

    if (
        data_device_manager == NULL ||
        seat == NULL ||
        compositor == NULL ||
        shm == NULL ||
        shell == NULL
    ) {
        bail("Missing a required global object");
    }

    data_device = wl_data_device_manager_get_data_device(data_device_manager, seat);
}

struct wl_surface *popup_tiny_invisible_surface() {
    struct wl_surface *surface = wl_compositor_create_surface(compositor);
    struct wl_shell_surface *shell_surface = wl_shell_get_shell_surface(shell, surface);
    wl_shell_surface_set_toplevel(shell_surface);
    wl_shell_surface_set_title(shell_surface, "wl-paste");

    int width = 1;
    int height = 1;
    int stride = width * 4;
    int size = stride * height;  // bytes

    // open an anonymous file and write some zero bytes to it
    int fd = syscall(SYS_memfd_create, "buffer", 0);
    ftruncate(fd, size);

    // turn it into a shared memory pool
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);

    // allocate the buffer in that pool
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool,
        0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    // zeros in ARGB8888 mean fully transparent

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_commit(surface);
    return surface;
}

static int global_serial;

void callback_done
(
    void *data,
    struct wl_callback *callback,
    uint32_t serial
) {
    global_serial = serial;
}

const struct wl_callback_listener callback_listener = {
    .done = callback_done
};

int get_serial() {
    struct wl_callback *callback = wl_display_sync(display);
    wl_callback_add_listener(callback, &callback_listener, NULL);

    while (global_serial == 0) {
        wl_display_dispatch(display);
    }

    return global_serial;
}

char *infer_mime_type_of_file(int fd) {
    char fdpath[64];
    snprintf(fdpath, sizeof(fdpath), "/proc/self/fd/%d", fd);

    char *file_path = malloc(PATH_MAX + 1);
    ssize_t link_length = readlink(fdpath, file_path, PATH_MAX + 1);
    file_path[link_length] = 0;

    // filter pipes out
    if (file_path[0] != '/') {
        return NULL;
    }

    int pipefd[2];
    pipe(pipefd);
    if (fork() == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        int devnull = open("/dev/null", O_RDONLY);
        dup2(devnull, STDIN_FILENO);
        close(devnull);
        execlp("xdg-mime", "xdg-mime", "query", "filetype", file_path, NULL);
        exit(1);
    }

    close(pipefd[1]);
    free(file_path);
    int wstatus;
    wait(&wstatus);
    if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0) {
        char *res = malloc(PATH_MAX + 1);
        size_t len = read(pipefd[0], res, PATH_MAX + 1);
        len--; // trim the newline
        close(pipefd[0]);
        res[len] = 0;

        if (strncmp(res, "inode/", strlen("inode/")) == 0) {
            free(res);
            return NULL;
        }

        return res;
    }
    close(pipefd[0]);
    return NULL;
}

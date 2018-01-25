#include <wayland-client.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> // STDOUT_FILENO
#include <syscall.h> // syscall, SYS_memfd_create

struct wl_data_device_manager *data_device_manager;
struct wl_seat *seat;
struct wl_compositor *compositor;
struct wl_shm *shm;
struct wl_shell *shell;

int should_exit;

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


void data_device_data_offer
(
    void *data,
    struct wl_data_device *data_device,
    struct wl_data_offer *data_offer
) {}

void data_device_selection
(
    void *data,
    struct wl_data_device *data_device,
    struct wl_data_offer *data_offer
) {
    if (!should_exit) {
        // do not accept twice
        wl_data_offer_receive(data_offer, "text/plain", STDOUT_FILENO);
        should_exit = 1;
    }
}

const struct wl_data_device_listener data_device_listener = {
    .data_offer = data_device_data_offer,
    .selection = data_device_selection
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

int main(int argc, const char *argv[]) {

    struct wl_display *display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "Failed to connect to a Wayland server\n");
        return 1;
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
        fprintf(stderr, "Missing a required global object\n");
        return 1;
    }

    struct wl_data_device *data_device = wl_data_device_manager_get_data_device(data_device_manager, seat);
    wl_data_device_add_listener(data_device, &data_device_listener, NULL);

    // HACK:
    // pop up a tiny invisible surface to get the keyboard focus,
    // otherwise we won't be notified of the selection

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


    while (!should_exit) {
        wl_display_dispatch(display);
    }
    // one last time
    wl_display_roundtrip(display);

    // exit now; the process that send us data may continue writing it to
    // our stdout after this process exits

    return 0;
}

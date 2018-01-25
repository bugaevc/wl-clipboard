#include <wayland-client.h>
#include <stdio.h>
#include <string.h>

struct wl_data_device_manager *data_device_manager;
struct wl_seat *seat;

int global_serial;
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
            2
        );
    } else if (strcmp(interface, "wl_seat") == 0) {
        seat = wl_registry_bind(
            registry,
            name,
            &wl_seat_interface,
            5
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


void data_source_target_handler
(
    void *data,
    struct wl_data_source *data_source,
    const char *mime_type
) {}

void data_source_send_handler
(
    void *data,
    struct wl_data_source *data_source,
    const char *mime_type,
    int fd
) {
    FILE *f = fdopen(fd, "w");
    fprintf(f, "Hello copied losers");
    fclose(f);
    printf("Copied\n");
    should_exit = 1;
}

void data_source_cancelled_handler
(
    void *data,
    struct wl_data_source *data_source
) {
    fprintf(stderr, "Cancelled\n");
    should_exit = 1;
}

const struct wl_data_source_listener data_source_listener = {
    .target = data_source_target_handler,
    .send = data_source_send_handler,
    .cancelled = data_source_cancelled_handler
};

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

    if (data_device_manager == NULL || seat == NULL) {
        fprintf(stderr, "Missing a required global object\n");
        return 1;
    }

    struct wl_data_source *data_source = wl_data_device_manager_create_data_source(data_device_manager);
    wl_data_source_add_listener(data_source, &data_source_listener, NULL);

    wl_data_source_offer(data_source, "text/plain");
    wl_data_source_offer(data_source, "text/plain;charset=utf-8");
    wl_data_source_offer(data_source, "TEXT");
    wl_data_source_offer(data_source, "STRING");
    wl_data_source_offer(data_source, "UTF8_STRING");

    struct wl_callback *callback = wl_display_sync(display);
    wl_callback_add_listener(callback, &callback_listener, NULL);

    while (!should_exit && global_serial == 0) {
        wl_display_dispatch(display);
    }

    struct wl_data_device *data_device = wl_data_device_manager_get_data_device(data_device_manager, seat);
    wl_data_device_set_selection(data_device, data_source, global_serial);

    while (!should_exit) {
        wl_display_dispatch(display);
    }

    return 0;
}
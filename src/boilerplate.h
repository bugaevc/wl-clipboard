#include <wayland-client.h>
#include <stdio.h>
#include <string.h> // strcmp
#include <stdlib.h> // exit
#include <unistd.h> // execl, STDOUT_FILENO
#include <sys/wait.h>
#include <sys/syscall.h> // syscall, SYS_memfd_create

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

wayland = dependency('wayland-client')

cc = meson.get_compiler('c')
have_memfd = cc.has_header_symbol('sys/syscall.h', 'SYS_memfd_create')
have_shm_anon = cc.has_header_symbol('sys/mman.h', 'SHM_ANON')

conf_data = configuration_data()

conf_data.set('HAVE_MEMFD', have_memfd)
conf_data.set('HAVE_SHM_ANON', have_shm_anon)

subdir('protocol')

configure_file(output: 'config.h', configuration: conf_data)

lib = static_library(
    'wl-clipboard',
    [
        protocol_headers,

        'includes/shell-protocols.h',
        'includes/selection-protocols.h',

        'util/string.h',
        'util/string.c',
        'util/files.h',
        'util/files.c',
        'util/misc.h',
        'util/misc.c',

        'types/source.h',
        'types/source.c',
        'types/offer.h',
        'types/offer.c',
        'types/device.h',
        'types/device.c',
        'types/device-manager.h',
        'types/device-manager.c',
        'types/keyboard.h',
        'types/keyboard.c',
        'types/seat.h',
        'types/seat.c',
        'types/shell.h',
        'types/shell.c',
        'types/shell-surface.h',
        'types/shell-surface.c',
        'types/popup-surface.h',
        'types/popup-surface.c',
        'types/registry.h',
        'types/registry.c',
        'types/copy-action.h',
        'types/copy-action.c',
    ],
    dependencies: wayland,
    link_with: protocol_deps
)

executable(
    'wl-copy',
    ['wl-copy.c', protocol_headers],
    dependencies: wayland,
    link_with: lib,
    install: true
)
executable(
    'wl-paste',
    ['wl-paste.c', protocol_headers],
    dependencies: wayland,
    link_with: lib,
    install: true
)

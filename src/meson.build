wayland = dependency('wayland-client')

wayland_scanner = find_program('wayland-scanner', required: false, native: true)
wayland_protocols = dependency('wayland-protocols', version: '>= 1.12', required: false)

if wayland_scanner.found()
    have_xdg_shell = wayland_protocols.found()
    have_wp_primary_selection = wayland_protocols.found() and wayland_protocols.version().version_compare('>= 1.17')
    # these are bundled
    have_gtk_primary_selection = true
    have_wlr_data_control = true

    if wayland.version().version_compare('>= 1.15')
        scanner_code_command = 'private-code'
    else
        scanner_code_command = 'code'
    endif
else
    have_xdg_shell = false
    have_gtk_primary_selection = false
    have_wlr_data_control = false
endif

cc = meson.get_compiler('c')
have_memfd = cc.has_header_symbol('sys/syscall.h', 'SYS_memfd_create')
have_shm_anon = cc.has_header_symbol('sys/mman.h', 'SHM_ANON')

conf_data = configuration_data()

conf_data.set('PROJECT_VERSION', '"@0@"'.format(meson.project_version()))
conf_data.set('HAVE_XDG_SHELL', have_xdg_shell)
conf_data.set('HAVE_WP_PRIMARY_SELECTION', have_wp_primary_selection)
conf_data.set('HAVE_GTK_PRIMARY_SELECTION', have_gtk_primary_selection)
conf_data.set('HAVE_WLR_DATA_CONTROL', have_wlr_data_control)

conf_data.set('HAVE_MEMFD', have_memfd)
conf_data.set('HAVE_SHM_ANON', have_shm_anon)

configure_file(output: 'config.h', configuration: conf_data)

if wayland_protocols.found()
    protocols_path = wayland_protocols.get_pkgconfig_variable('pkgdatadir')
endif

protocols = []

if not have_xdg_shell
    warning('Building without xdg-shell support')
else
    xdg_shell_xml = join_paths(protocols_path, 'stable', 'xdg-shell', 'xdg-shell.xml')
    protocols += [['xdg-shell', xdg_shell_xml]]
endif

if have_wp_primary_selection
    wp_primary_selection_xml = join_paths(protocols_path, 'unstable', 'primary-selection', 'primary-selection-unstable-v1.xml')
    protocols += [['wp-primary-selection', wp_primary_selection_xml]]
endif

if not have_gtk_primary_selection
    warning('Building without primary selection support')
else
    gtk_primary_selection_xml = join_paths('protocol', 'gtk-primary-selection.xml')
    protocols += [['gtk-primary-selection', gtk_primary_selection_xml]]
endif

if have_wlr_data_control
    wlr_data_control_xml = join_paths('protocol', 'wlr-data-control-unstable-v1.xml')
    protocols += [['wlr-data-control', wlr_data_control_xml]]
endif

protocol_deps = []
protocol_headers = []

foreach protocol : protocols
    name = protocol[0]
    xml = protocol[1]
    header = custom_target(name + ' client header',
        input: xml, output: name + '.h',
        command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@']
    )
    impl = custom_target(name + ' code',
        input: xml, output: name + '.c',
        command: [wayland_scanner, scanner_code_command, '@INPUT@', '@OUTPUT@']
    )
    protocol_headers += header
    lib = static_library(name, impl, header, dependencies: wayland)
    protocol_deps += lib
endforeach

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

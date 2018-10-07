wayland = dependency('wayland-client')

wayland_scanner = find_program('wayland-scanner', required: false)
wayland_protocols = dependency('wayland-protocols', version: '>= 1.12', required: false)

have_xdg_shell = wayland_scanner.found() and wayland_protocols.found()
have_gtk_primary_selection = wayland_scanner.found()

conf_data = configuration_data()
conf_data.set('HAVE_XDG_SHELL', have_xdg_shell)
conf_data.set('HAVE_GTK_PRIMARY_SELECTION', have_gtk_primary_selection)
configure_file(output: 'config.h', configuration: conf_data)

if not have_xdg_shell
    warning('Building without xdg-shell support')
    xdg_shell = static_library('xdg-shell')
else
    protocols_path = wayland_protocols.get_pkgconfig_variable('pkgdatadir')
    xdg_shell_xml = join_paths(protocols_path, 'stable', 'xdg-shell', 'xdg-shell.xml')

    xdg_shell_client_h = custom_target('xdg-shell client header',
        input: xdg_shell_xml,
        output: 'xdg-shell-client.h',
        command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@']
    )

    xdg_shell_c = custom_target('xdg-shell code',
        input: xdg_shell_xml,
        output: 'xdg-shell.c',
        command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@']
    )

    xdg_shell = static_library('xdg-shell', xdg_shell_c, xdg_shell_client_h)
endif

if not have_gtk_primary_selection
    warning('Building without primary selection support')
    gtk_primary_selection = static_library('gtk-primary-selection')
else
    gtk_primary_selection_xml = join_paths('protocol', 'gtk-primary-selection.xml')

    gtk_primary_selection_client_h = custom_target(
        'gtk-primary-selection client header',
        input: gtk_primary_selection_xml,
        output: 'gtk-primary-selection.h',
        command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@']
    )

    gtk_primary_selection_c = custom_target(
        'gtk-primary-selection code',
        input: gtk_primary_selection_xml,
        output: 'gtk-primary-selection.c',
        command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@']
    )

    gtk_primary_selection = static_library(
        'gtk-primary-selection',
        gtk_primary_selection_c,
        gtk_primary_selection_client_h
    )
endif

boilerplate = static_library(
    'wl-clipboard-boilerplate',
    'boilerplate.c',
    dependencies: wayland,
    link_with: [xdg_shell, gtk_primary_selection]
)

executable('wl-copy', 'wl-copy.c', dependencies: wayland, link_with: boilerplate, install: true)
executable('wl-paste', 'wl-paste.c', dependencies: wayland, link_with: boilerplate, install: true)
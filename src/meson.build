wayland = dependency('wayland-client')

wayland_scanner = find_program('wayland-scanner', required: false)
wayland_protocols = dependency('wayland-protocols', version: '>= 1.12', required: false)

have_xdg_shell = wayland_scanner.found() and wayland_protocols.found()

conf_data = configuration_data()
conf_data.set('HAVE_XDG_SHELL', have_xdg_shell)
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

boilerplate = static_library('wl-clipboard-boilerplate', 'boilerplate.c', dependencies: wayland, link_with: xdg_shell)

executable('wl-copy', 'wl-copy.c', dependencies: wayland, link_with: boilerplate, install: true)
executable('wl-paste', 'wl-paste.c', dependencies: wayland, link_with: boilerplate, install: true)
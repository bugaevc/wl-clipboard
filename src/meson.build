wayland = dependency('wayland-client')

wayland_scanner = find_program('wayland-scanner', required: false)
wayland_protocols = dependency('wayland-protocols', version: '>= 1.12', required: false)

if wayland_scanner.found()
    have_xdg_shell = wayland_protocols.found()
    # these are bundled
    have_wlr_layer_shell = have_xdg_shell  # depends on xdg_popup
    have_gtk_primary_selection = true
    have_wlr_data_control = true

    if wayland.version().version_compare('>= 1.15')
        scanner_code_command = 'private-code'
    else
        scanner_code_command = 'code'
    endif
else
    have_xdg_shell = false
    have_wlr_layer_shell = false
    have_gtk_primary_selection = false
    have_wlr_data_control = false
endif

conf_data = configuration_data()
conf_data.set('PROJECT_VERSION', '"@0@"'.format(meson.project_version()))
conf_data.set('HAVE_XDG_SHELL', have_xdg_shell)
conf_data.set('HAVE_WLR_LAYER_SHELL', have_wlr_layer_shell)
conf_data.set('HAVE_GTK_PRIMARY_SELECTION', have_gtk_primary_selection)
conf_data.set('HAVE_WLR_DATA_CONTROL', have_wlr_data_control)
configure_file(output: 'config.h', configuration: conf_data)

protocols = []

if not have_xdg_shell
    warning('Building without xdg-shell support')
else
    protocols_path = wayland_protocols.get_pkgconfig_variable('pkgdatadir')
    xdg_shell_xml = join_paths(protocols_path, 'stable', 'xdg-shell', 'xdg-shell.xml')
    protocols += [['xdg-shell', xdg_shell_xml]]
endif

if have_wlr_layer_shell
    wlr_layer_shell_xml = join_paths('protocol', 'wlr-layer-shell-unstable-v1.xml')
    protocols += [['wlr-layer-shell', wlr_layer_shell_xml]]
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
    lib = static_library(name, impl, header)
    protocol_deps += lib
endforeach

boilerplate = static_library(
    'wl-clipboard-boilerplate',
    'boilerplate.c',
    dependencies: wayland,
    link_with: protocol_deps
)

executable('wl-copy', 'wl-copy.c', dependencies: wayland, link_with: boilerplate, install: true)
executable('wl-paste', 'wl-paste.c', dependencies: wayland, link_with: boilerplate, install: true)

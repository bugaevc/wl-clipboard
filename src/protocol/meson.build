protocols_opt = get_option('protocols')
wayland_scanner = find_program('wayland-scanner', required: protocols_opt, native: true)
wayland_protocols = dependency(
    'wayland-protocols',
    version: '>= 1.12',
    required: protocols_opt,
    default_options: ['tests=false']
)

if wayland_scanner.found()
    have_xdg_shell = wayland_protocols.found()
    have_wp_primary_selection = wayland_protocols.found() and wayland_protocols.version().version_compare('>= 1.17')
    have_xdg_activation = wayland_protocols.found() and wayland_protocols.version().version_compare('>= 1.21')
    # these are bundled
    have_gtk_primary_selection = true
    have_wlr_data_control = true
    have_ext_data_control = wayland_protocols.found() and wayland_protocols.version().version_compare('>= 1.39')
    have_gtk_shell = true

    if wayland.version().version_compare('>= 1.15')
        scanner_code_command = 'private-code'
    else
        scanner_code_command = 'code'
    endif
else
    have_xdg_shell = false
    have_wp_primary_selection = false
    have_xdg_activation = false
    have_gtk_primary_selection = false
    have_wlr_data_control = false
    have_ext_data_control = false
    have_gtk_shell = false
endif

conf_data.set('PROJECT_VERSION', '"@0@"'.format(meson.project_version()))
conf_data.set('HAVE_XDG_SHELL', have_xdg_shell)
conf_data.set('HAVE_WP_PRIMARY_SELECTION', have_wp_primary_selection)
conf_data.set('HAVE_GTK_PRIMARY_SELECTION', have_gtk_primary_selection)
conf_data.set('HAVE_WLR_DATA_CONTROL', have_wlr_data_control)
conf_data.set('HAVE_EXT_DATA_CONTROL', have_ext_data_control)
conf_data.set('HAVE_XDG_ACTIVATION', have_xdg_activation)
conf_data.set('HAVE_GTK_SHELL', have_gtk_shell)

if wayland_protocols.found()
    if meson.version().version_compare('>= 0.58')
        protocols_path = wayland_protocols.get_variable('pkgdatadir')
    else
        protocols_path = wayland_protocols.get_pkgconfig_variable('pkgdatadir')
    endif
endif

protocols = []

if not have_xdg_shell
    warning('Building without xdg-shell support, wl-clipboard will be unable to work on some compositors')
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
    gtk_primary_selection_xml = 'gtk-primary-selection.xml'
    protocols += [['gtk-primary-selection', gtk_primary_selection_xml]]
endif

if have_wlr_data_control
    wlr_data_control_xml = 'wlr-data-control-unstable-v1.xml'
    protocols += [['wlr-data-control', wlr_data_control_xml]]
endif

if have_ext_data_control
    ext_data_control_xml = join_paths(protocols_path, 'staging', 'ext-data-control', 'ext-data-control-v1.xml')
    protocols += [['ext-data-control', ext_data_control_xml]]
endif

if have_xdg_activation
    xdg_activation_xml = join_paths(protocols_path, 'staging', 'xdg-activation', 'xdg-activation-v1.xml')
    protocols += [['xdg-activation', xdg_activation_xml]]
endif

if have_gtk_shell
    gtk_shell_xml = 'gtk-shell.xml'
    protocols += [['gtk-shell', gtk_shell_xml]]
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

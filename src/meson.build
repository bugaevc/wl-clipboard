wayland = dependency('wayland-client')

boilerplate = static_library('wl-clipboard-boilerplate', 'boilerplate.c', dependencies: wayland)

executable('wl-copy', 'wl-copy.c', dependencies: wayland, link_with: boilerplate, install: true)
executable('wl-paste', 'wl-paste.c', dependencies: wayland, link_with: boilerplate, install: true)
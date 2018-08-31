# wl-clipboard: Wayland clipboard utilities

This project implements two little Wayland clipboard utilities, `wl-copy` and
`wl-paste`, that let you easily copy data between the clipboard and Unix pipes,
sockets, files and so on.

Usage is as simple as:

```bash
# copy a simple text message
$ wl-copy Hello world!

# copy the list of files in Downloads
$ ls ~/Downloads | wl-copy

# copy an image file
$ wl-copy < ~/Pictures/photo.png

# paste to a file
$ wl-paste > clipboard.txt

# grep each pasted word in file source.c
$ for word in $(wl-paste); do grep $word source.c; done
```

# Building

wl-clipboard is a simple Meson project, so building it is just:

```bash
$ git clone https://github.com/bugaevc/wl-clipboard.git
$ cd wl-clipboard
$ meson build
$ cd build
$ ninja
```

wl-clipboard only supports Linux (though patches to add BSD support are
welcome!). You'll need `wayland-client` library & headers installed (try package
named `wayland-devel` or `libwayland-dev`). wl-clipboard tries to use `xdg-mime`
to infer content mime type, but will fall back to plain text if `xdg-mime` is
unavailable.

If `wayland-scanner` and `wayland-protocols` (version 1.12 or later) are present
at build time, wl-clipboard will be built with additional xdg-shell support;
this may be helpful if your Wayland compositor does not support `wl_shell`.

# License

wl-clipboard is free sofrware, available under the GNU General Public License
version 3 or later.

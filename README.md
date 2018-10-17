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

# copy the previous command
$ wl-copy "!!"
```

# Options

For `wl-copy`:

* `-o`, `--paste-once` Only serve one paste request and then exit. Unless a clipboard manager specifically designed to prevent this is in use, this has the effect of clearing the clipboard after the first paste, which is useful for copying sensitive data such as passwords. Note that this may break pasting into some clients, in particular pasting into XWayland windows is known to break when this option is used.
* `-f`, `--foreground` By default, `wl-copy` forks and serves data requests in the background; this option overrides that behavior, causing `wl-copy` to run in the foreground.
* `-c`, `--clear` Instead of copying anything, clear the clipboard so that nothing is copied.

For `wl-paste`:

* `-n`, `-no-newline` Do not append a newline character after the pasted clipboard content.

For both:

* `-p`, `--primary` Use the "primary" clipboard instead of the regular clipboard. This uses the private GTK+ primary selection protocol. See [the GNOME Wiki page on primary selection under Wayland](https://wiki.gnome.org/Initiatives/Wayland/PrimarySelection) for more details.
* `-t mime/type`, `--mime-type mime/type` Override the inferred MIME type for the content.

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

If `wayland-scanner` is present at build time, wl-clipboard will be built with
primary selecttion support using the bundled
[gtk-primary-selection protocol](src/protocol/gtk-primary-selection.xml).
Additionally, if `wayland-protocols` (version 1.12 or later) is present at build
time, wl-clipboard will be built with xdg-shell support; this may be helpful if
your Wayland compositor does not support `wl_shell`.

# License

wl-clipboard is free sofrware, available under the GNU General Public License
version 3 or later.

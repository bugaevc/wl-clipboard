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

# replace the current selection with the list of types it's offered in
$ wl-paste --list-types | wl-copy
```

Although `wl-copy` and `wl-paste` are particularly optimized for plain text and
other textual content formats, they fully support content of arbitrary MIME
types. `wl-copy` automatically infers the type of the copied content by running
`xdg-mime(1)` on it. `wl-paste` tries its best to pick a type to paste based on
the list of offered MIME types and the extension of the file it's pasting into.
If you're not satisfied with the type they pick or don't want to rely on this
implicit type inference, you can explicitly specify the type to use with the
`--type` option.

# Options

For `wl-copy`:

* `-o`, `--paste-once` Only serve one paste request and then exit. Unless a clipboard manager specifically designed to prevent this is in use, this has the effect of clearing the clipboard after the first paste, which is useful for copying sensitive data such as passwords. Note that this may break pasting into some clients, in particular pasting into XWayland windows is known to break when this option is used.
* `-f`, `--foreground` By default, `wl-copy` forks and serves data requests in the background; this option overrides that behavior, causing `wl-copy` to run in the foreground.
* `-c`, `--clear` Instead of copying anything, clear the clipboard so that nothing is copied.

For `wl-paste`:

* `-n`, `-no-newline` Do not append a newline character after the pasted clipboard content. This option is automatically enabled for non-text content types.
* `-l`, `--list-types` Instead of pasting the selection, output the list of MIME types it is offered in.

For both:

* `-p`, `--primary` Use the "primary" clipboard instead of the regular clipboard. This uses the private GTK+ primary selection protocol. See [the GNOME Wiki page on primary selection under Wayland](https://wiki.gnome.org/Initiatives/Wayland/PrimarySelection) for more details.
* `-t mime/type`, `--type mime/type` Override the inferred MIME type for the content. For `wl-copy` this option controls which type `wl-copy` will offer the content as. For `wl-paste` it controls which of the offered types `wl-paste` will request the content in. In addition to specific MIME types such as _image/png_, `wl-paste` also accepts generic type names such as _text_ and _image_ which make it automatically pick some offered MIME type that matches the given generic name.

# Building

wl-clipboard is a simple Meson project, so building it is just:

```bash
# clone
$ git clone https://github.com/bugaevc/wl-clipboard.git
$ cd wl-clipboard

# build
$ meson build
$ cd build
$ ninja

# install
$ sudo ninja install
```

wl-clipboard only supports Linux (though patches to add BSD support are
welcome!). The only manadatory dependency is the `wayland-client` library (try
package named `wayland-devel` or `libwayland-dev`).

Optional dependencies for building:
* `wayland-scanner` for primary selection support using the bundled [gtk-primary-selection protocol](src/protocol/gtk-primary-selection.xml)
* `wayland-protocols` (version 1.12 or later) for xdg-shell support (otherwise it won't run under compositors lacking `wl_shell` support, see [the issue #2](https://github.com/bugaevc/wl-clipboard/issues/2))

Optional dependencies for running:
* `xdg-mime` for content type inference in `wl-copy` (try package named `xdg-utils`)
* `/etc/mime.types` file for type inference in `wl-paste` (try package named `mime-support` or `mailcap`)

# License

wl-clipboard is free software, available under the GNU General Public License
version 3 or later.

# wl-clipboard: Wayland clipboard utilities

This project implements two command-line Wayland clipboard utilities, `wl-copy`
and `wl-paste`, that let you easily copy data between the clipboard and Unix
pipes, sockets, files and so on.

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

* `-n`, `--trim-newline` Do not copy the trailing newline character if it is present in the input file.
* `-o`, `--paste-once` Only serve one paste request and then exit. Unless a clipboard manager specifically designed to prevent this is in use, this has the effect of clearing the clipboard after the first paste, which is useful for copying sensitive data such as passwords. Note that this may break pasting into some clients, in particular pasting into XWayland windows is known to break when this option is used.
* `-f`, `--foreground` By default, `wl-copy` forks and serves data requests in the background; this option overrides that behavior, causing `wl-copy` to run in the foreground.
* `-c`, `--clear` Instead of copying anything, clear the clipboard so that nothing is copied.

For `wl-paste`:

* `-n`, `--no-newline` Do not append a newline character after the pasted clipboard content. This option is automatically enabled for non-text content types and when using the `--watch` mode.
* `-l`, `--list-types` Instead of pasting the selection, output the list of MIME types it is offered in.
* `-w command...`, `--watch command...` Instead of pasting once and exiting, continuously watch the clipboard for changes, and run the specified command each time a new selection appears. The spawned process can read the clipboard contents from its standard input. This mode requires a compositor that supports the [wlroots data-control protocol](https://github.com/swaywm/wlr-protocols/blob/master/unstable/wlr-data-control-unstable-v1.xml).

For both:

* `-p`, `--primary` Use the "primary" clipboard instead of the regular clipboard.
* `-t mime/type`, `--type mime/type` Override the inferred MIME type for the content. For `wl-copy` this option controls which type `wl-copy` will offer the content as. For `wl-paste` it controls which of the offered types `wl-paste` will request the content in. In addition to specific MIME types such as _image/png_, `wl-paste` also accepts generic type names such as _text_ and _image_ which make it automatically pick some offered MIME type that matches the given generic name.
* `-s seat-name`, `--seat seat-name` Specify which seat `wl-copy` and `wl-paste` should work with. Wayland natively supports multi-seat configurations where each seat gets its own mouse pointer, keyboard focus, and among other things its own separate clipboard. The name of the default seat is likely _default_ or _seat0_, and additional seat names normally come form `udev(7)` property `ENV{WL_SEAT}`. You can view the list of the currently available seats as advertised by the compositor using the `weston-info(1)` tool. If you don't specify the seat name explicitly, `wl-copy` and `wl-paste` will pick a seat arbitrarily. If you are using a single-seat system, there is little reason to use this option.
* `-v`, `--version` Display the version of wl-clipboard and some short info about its license.
* `-h`, `--help` Display a short help message listing the available options.

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

wl-clipboard supports Linux and BSD systems, and is also known to work on
Mac OS X and GNU Hurd. The only mandatory dependency is the `wayland-client`
library (try package named `wayland-devel` or `libwayland-dev`).

Optional dependencies for building:
* `wayland-scanner`
* `wayland-protocols` (version 1.12 or later)

If these are found during configuration, wl-clipboard gets built with
additional protocols support, which enables features such as primary selection
support and `--watch` mode.

Optional dependencies for running:
* `xdg-mime` for content type inference in `wl-copy` (try package named `xdg-utils`)
* `/etc/mime.types` file for type inference in `wl-paste` (try package named `mime-support` or `mailcap`)

# License

wl-clipboard is free software, available under the GNU General Public License
version 3 or later.

# Related projects

* [wl-clipboard-x11](https://github.com/brunelli/wl-clipboard-x11): A wrapper to use wl-clipboard as a drop-in replacement to X11 clipboard tools.
* [wl-clipboard-rs](https://github.com/YaLTeR/wl-clipboard-rs): A Rust crate (library) for working with the Wayland clipboard which includes a reimplementation of `wl-copy` and `wl-paste`.

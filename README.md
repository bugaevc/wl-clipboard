# wl-clipboard: Wayland clipboard utilities

This project implements two command-line Wayland clipboard utilities, `wl-copy`
and `wl-paste`, that let you easily copy data between the clipboard and Unix
pipes, sockets, files and so on.

```bash
# Copy a simple text message:
$ wl-copy Hello world!

# Copy the list of files in ~/Downloads:
$ ls ~/Downloads | wl-copy

# Copy an image:
$ wl-copy < ~/Pictures/photo.png

# Copy the previous command:
$ wl-copy "!!"

# Paste to a file:
$ wl-paste > clipboard.txt

# Sort clipboard contents:
$ wl-paste | sort | wl-copy

# Upload clipboard contents to a pastebin on each change:
$ wl-paste --watch nc paste.example.org 5555
```

Please see the wl-clipboard(1) man page for more details.

# Installing

wl-clipboard is likely available in your favorite Linux or BSD distro. For
building from source, see [BUILDING.md](BUILDING.md).

# License

wl-clipboard is free software, available under the GNU General Public License
version 3 or later.

# Related projects

* [wl-clipboard-x11](https://github.com/brunelli/wl-clipboard-x11): A wrapper to
  use wl-clipboard as a drop-in replacement to X11 clipboard tools.
* [wl-clipboard-rs](https://github.com/YaLTeR/wl-clipboard-rs): A Rust crate
  (library) for working with the Wayland clipboard which includes a
  reimplementation of `wl-copy` and `wl-paste`.

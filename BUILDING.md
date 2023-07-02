# Building

wl-clipboard is a simple Meson project, so building it is just:

```bash
# Clone:
$ git clone https://github.com/bugaevc/wl-clipboard.git
$ cd wl-clipboard

# Build:
$ meson setup build
$ cd build
$ ninja

# Install
$ sudo meson install
```

wl-clipboard supports Linux and BSD systems, and is also known to work on
Mac OS X and GNU Hurd. The only mandatory dependency is the `wayland-client`
library (try package named `wayland-devel` or `libwayland-dev`).

Optional (but highly recommended) dependencies for building:
* `wayland-scanner`
* `wayland-protocols` (version 1.12 or later)

If these are found during configuration, wl-clipboard gets built with
additional protocols support, which enables features such as primary selection
support and `--watch` mode. If `wayland-protocols` is not found, Meson can
optionally build it on the spot as a subproject.

Note that many compositors have dropped support for the `wl_shell` interface,
which means wl-clipboard will not work under them unless built with both
`wayland-scanner` and `wayland-protocols`. For this reason, you have to
explicitly opt into allowing building without these dependencies by specifying
`-D protocols=auto` (or `-D protocols=disabled`) when configuring with Meson.

Optional dependencies for running:
* `xdg-mime` for content type inference in `wl-copy` (try package named
  `xdg-utils`)
* `/etc/mime.types` file for type inference in `wl-paste` (try package named
  `mime-support` or `mailcap`)

If you're packaging wl-clipboard for a distribution, please consider making
packages providing `xdg-mime` and `/etc/mime.types` *weak* dependencies of the
package providing wl-clipboard, meaning ones that get installed along with
wl-clipboard by default, but are not strictly required by it.

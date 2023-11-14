{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
    name = "wl-clipboard-shell";
    buildInputs = [
        pkgs.meson
        pkgs.ninja
    ];
}

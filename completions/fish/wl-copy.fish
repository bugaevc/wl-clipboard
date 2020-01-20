function __wayland_seats --description 'Print all wayland seats'
    if type -q weston-info
        weston-info 2>/dev/null | sed -n '/wl_seat/{n;s/\s*name: //;p}'
    else if type -q loginctl
        loginctl --no-legend --no-pager list-seats 2>/dev/null
    else
        # Fallback seat
        echo "seat0"
    end

end

complete -c wl-copy -x
complete -c wl-copy -s h -l help -d 'Display a help message'
complete -c wl-copy -s v -l version -d 'Display version info'
complete -c wl-copy -s o -l paste-once -d 'Only serve one paste request and then exit'
complete -c wl-copy -s c -l clear -d 'Instead of copying anything, clear the clipboard'
complete -c wl-copy -s p -l primary -d 'Use the "primary" clipboard'
complete -c wl-copy -s n -l trim-newline -d 'Do not copy the trailing newline character'
complete -c wl-copy -s t -l type -x -d 'Override the inferred MIME type for the content' -a "(__fish_print_xdg_mimetypes)"
complete -c wl-copy -s s -l seat -x -d 'Pick the seat to work with' -a "(__wayland_seats)"

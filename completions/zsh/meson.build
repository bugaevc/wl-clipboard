zsh_completion_dir = get_option('zshcompletiondir')
if zsh_completion_dir == ''
    zsh_completion_dir = join_paths(get_option('datadir'), 'zsh', 'site-functions')
endif

if zsh_completion_dir != 'no'
    install_data('_wl-copy', install_dir: zsh_completion_dir)
    install_data('_wl-paste', install_dir: zsh_completion_dir)
endif

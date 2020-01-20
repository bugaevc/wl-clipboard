fish_completion_dir = get_option('fishcompletiondir')
fish = dependency('fish', required : false)

if fish_completion_dir == '' and fish.found()
    fish_completion_dir = fish.get_pkgconfig_variable('completionsdir')
elif fish_completion_dir == ''
    # Fish does not look in /usr/local/, which is the default prefix,
    # so we cannot use get_option("datadir"). Instead, they recommend
    # custom completions to be installed into /usr/share/fish/vendor_completions.d,
    # so we use that. Note that this is a fallback in case you neither
    # have fish installed, nor have specified a path explicitly
    fish_completion_dir = '/usr/share/fish/vendor_completions.d'
endif

if fish_completion_dir != 'no'
    install_data('wl-copy.fish', install_dir: fish_completion_dir)
    install_data('wl-paste.fish', install_dir: fish_completion_dir)
endif

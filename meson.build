project('mpirs-notify', 'c',
  version: '0.1.0',
  default_options: [
    'warning_level=3',
    'werror=false',
    'c_std=c11',
    'optimization=2',
    'strip=true',
    'b_lto=true',
    'b_pie=true'
  ],
  license: 'GPL-3.0-or-later'
)

# Dependencies
dbus_dep = dependency('dbus-1', required: true)
notify_dep = dependency('libnotify', required: true)
glib_dep = dependency('glib-2.0', required: true)
gio_dep = dependency('gio-2.0', required: true)
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', required: true)

# Version define
add_global_arguments('-DVERSION="@0@"'.format(meson.project_version()), language: 'c')

# Include directory
inc_dir = include_directories('include')

# Sources
sources = [
  'src/main.c',
  'src/dbus.c',
  'src/metadata.c',
  'src/art.c',
  'src/player_icon.c',
  'src/notify.c',
  'src/util.c',
  'src/config.c'
]

# Executable
executable('mpirs-notify',
  sources,
  dependencies: [dbus_dep, notify_dep, glib_dep, gio_dep, gdk_pixbuf_dep],
  include_directories: [inc_dir],
  install: true
)

# Install config example
install_data('data/mpirs-notify.conf.example',
  install_dir: join_paths(get_option('sysconfdir'), 'xdg'),
  rename: 'mpirs-notify.conf'
)

# Install desktop file
install_data('data/mpirs-notify.desktop',
  install_dir: join_paths(get_option('datadir'), 'applications')
)

# Install man page (to be created)
# install_man('docs/mpirs-notify.1')
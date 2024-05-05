mps_sources = [
  'bus.c',
  'main.c',
  'settings.c'
]

mps_deps = [
  dependency('glib-2.0'),
  dependency('gio-2.0'),
  dependency('gio-unix-2.0'),
]

executable('mobile-power-saver', mps_sources,
  dependencies: mps_deps,
  install_dir: bin_dir,
  install: true,
)
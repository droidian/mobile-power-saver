mps_sources = [
  'bluetooth.c',
  'bus.c',
  'dozing.c',
  'main.c',
  'manager.c',
  'mpris.c',
  'modem.c',
  'network_manager.c',
  'network_manager_modem.c',
  'settings.c',
  '../common/services.c',
  '../common/utils.c'
]

mps_deps = [
  dependency('glib-2.0'),
  dependency('gio-2.0'),
  dependency('gio-unix-2.0')
]

if mm_enabled
  mps_deps += [ dependency('mm-glib') ]
  mps_sources += [ 'modem_mm.c' ]
else
  mps_sources += [ 'modem_ofono.c', 'modem_ofono_device.c' ]
endif

executable('mobile-power-saver', mps_sources,
  dependencies: mps_deps,
  install_dir: bin_dir,
  install: true,
)

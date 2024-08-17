mps_sources = [
  'bus.c',
  'cpufreq.c',
  'cpufreq_device.c',
  'devfreq.c',
  'devfreq_device.c',
  'freezer.c',
  'freq_device.c',
  'kernel_settings.c',
  'logind.c',
  'main.c',
  'manager.c',
  'modem.c',
  'network_manager.c',
  '../common/services.c',
  '../common/utils.c'
]

mps_deps = [
  dependency('glib-2.0'),
  dependency('gio-2.0'),
  dependency('gio-unix-2.0')
]

if wifi_enabled
  mps_deps += [ dependency('libnl-genl-3.0') ]
  mps_sources += [ 'wifi.c' ]
endif

if mm_enabled
  mps_deps += [ dependency('mm-glib') ]
  mps_sources += [ 'modem_mm.c' ]
else
  mps_sources += [ 'modem_ofono.c', 'modem_ofono_device.c' ]
endif

executable('mobile-power-saver', mps_sources,
  dependencies: mps_deps,
  install_dir: sbin_dir,
  install: true,
)

mps_sources = [
  'bus.c',
  'cpufreq.c',
  'cpufreq_device.c',
  'devfreq.c',
  'devfreq_device.c',
  'freezer.c',
  'freq_device.c',
  'binder.c',
  'kernel_settings.c',
  'logind.c',
  'main.c',
  'manager.c',
  '../common/services.c',
  '../common/utils.c'
]

mps_deps = [
  dependency('glib-2.0'),
  dependency('gio-2.0'),
  dependency('gio-unix-2.0'),
  dependency('libgbinder')
]

executable('mobile-power-saver', mps_sources,
  dependencies: mps_deps,
  install_dir: sbin_dir,
  install: true,
)

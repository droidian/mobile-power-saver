project('mps', 'c',
          version: '0.2.0',
    meson_version: '>= 0.60.0',
  default_options: [ 'warning_level=2', 'werror=false', 'c_std=gnu11', ],
)

cc = meson.get_compiler('c')

prefix = get_option('prefix')

data_dir = join_paths(prefix, get_option('datadir'))
localedir = join_paths(prefix, get_option('localedir'))
mps_data_dir = join_paths(data_dir, meson.project_name())
mps_resource = join_paths(mps_data_dir, meson.project_name() + '.gresource')
devices_json = join_paths(mps_data_dir, 'devices.json')
dbus_conf_dir = join_paths(data_dir, 'dbus-1/system.d')
dbus_service_dir = join_paths(data_dir, 'dbus-1/system-services')
systemd_system_dir = join_paths(get_option('prefix'), 'lib/systemd/system')
systemd_user_dir = join_paths(get_option('prefix'), 'lib/systemd/user')
bin_dir = join_paths(get_option('prefix'), get_option('bindir'))
sbin_dir = join_paths(get_option('prefix'), get_option('sbindir'))

config_h = configuration_data()
config_h.set('APP_ID', '"org.adishatz.Mps"')
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set('MPS_RESOURCES', '"' + mps_resource + '"')
config_h.set('DEVICES_JSON', '"' + devices_json + '"')
config_h.set('BIN_DIR', bin_dir)
config_h.set('SBIN_DIR', sbin_dir)
config_h.set_quoted('GETTEXT_PACKAGE', 'mps')
config_h.set_quoted('LOCALEDIR', localedir)

configure_file(output: 'config.h', configuration: config_h)
add_project_arguments(['-I' + meson.project_build_root()], language: 'c')

gnome = import('gnome')

project_c_args = []
test_c_args = [
  '-Wcast-align',
  '-Wdeclaration-after-statement',
  '-Werror=address',
  '-Werror=array-bounds',
  '-Werror=empty-body',
  '-Werror=implicit',
  '-Werror=implicit-function-declaration',
  '-Werror=incompatible-pointer-types',
  '-Werror=init-self',
  '-Werror=int-conversion',
  '-Werror=int-to-pointer-cast',
  '-Werror=main',
  '-Werror=misleading-indentation',
  '-Werror=missing-braces',
  '-Werror=missing-include-dirs',
  '-Werror=nonnull',
  '-Werror=overflow',
  '-Werror=parenthesis',
  '-Werror=pointer-arith',
  '-Werror=pointer-to-int-cast',
  '-Werror=redundant-decls',
  '-Werror=return-type',
  '-Werror=sequence-point',
  '-Werror=shadow',
  '-Werror=strict-prototypes',
  '-Werror=trigraphs',
  '-Werror=undef',
  '-Werror=write-strings',
  '-Wformat-nonliteral',
  '-Wignored-qualifiers',
  '-Wimplicit-function-declaration',
  '-Wlogical-op',
  '-Wmissing-declarations',
  '-Wmissing-format-attribute',
  '-Wmissing-include-dirs',
  '-Wmissing-noreturn',
  '-Wnested-externs',
  '-Wno-cast-function-type',
  '-Wno-dangling-pointer',
  '-Wno-missing-field-initializers',
  '-Wno-sign-compare',
  '-Wno-unused-parameter',
  '-Wold-style-definition',
  '-Wpointer-arith',
  '-Wredundant-decls',
  '-Wstrict-prototypes',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wundef',
  '-Wuninitialized',
  '-Wunused',
  '-fno-strict-aliasing',
  ['-Werror=format-security', '-Werror=format=2'],
]
if get_option('buildtype') != 'plain'
  test_c_args += '-fstack-protector-strong'
endif
foreach arg: test_c_args
  if cc.has_multi_arguments(arg)
    project_c_args += arg
  endif
endforeach
add_project_arguments(project_c_args, language: 'c')

subdir('system')
subdir('user')
subdir('data')

project('gst-turbojpeg', 'c',
  version : '1.0.0',
  license : 'LGPL',
  default_options : ['warning_level=1',
                     'buildtype=release',
                     'optimization=3'])

gst_version = '>=1.16.0'
gst_req = '>= 1.16.0'

gst_dep = dependency('gstreamer-1.0', version : gst_req)
gstbase_dep = dependency('gstreamer-base-1.0', version : gst_req)
gstvideo_dep = dependency('gstreamer-video-1.0', version : gst_req)
turbojpeg_dep = dependency('libturbojpeg')

plugin_c_args = ['-DHAVE_CONFIG_H', '-O3', '-ffast-math', '-funroll-loops', '-march=native']

cdata = configuration_data()
cdata.set_quoted('PACKAGE', 'gst-turbojpeg')
cdata.set_quoted('PACKAGE_VERSION', meson.project_version())
cdata.set_quoted('PACKAGE_BUGREPORT', 'https://github.com/example/gst-turbojpeg')
cdata.set_quoted('PACKAGE_NAME', 'GStreamer TurboJPEG Plugin')
cdata.set_quoted('GST_PACKAGE_NAME', 'GStreamer TurboJPEG Plugin')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://github.com/example/gst-turbojpeg')
cdata.set_quoted('VERSION', meson.project_version())

configure_file(output : 'config.h', configuration : cdata)

plugin_sources = [
  'src/gstturbojpegdec.c',
  'src/gstturbojpegenc.c',
  'src/plugin.c'
]

gstturbojpeg = library('gstturbojpeg',
  plugin_sources,
  c_args: plugin_c_args,
  dependencies : [gst_dep, gstbase_dep, gstvideo_dep, turbojpeg_dep],
  install : true,
  install_dir : join_paths(get_option('libdir'), 'gstreamer-1.0'),
)
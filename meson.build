project('gst-turbojpeg', ['c', 'cpp'],
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

# Force building from submodule source (ignore system package)
message('Building TurboJPEG from submodule source...')

# Build libjpeg-turbo using external command
libjpeg_build_dir = meson.current_build_dir() / 'libjpeg-turbo-build'

#for static and dynamic builds "turbojpeg turbojpeg-static"
# Custom target to build libjpeg-turbo
libjpeg_build = custom_target('libjpeg-turbo-build',
  command: [
    'sh', '-c', 
    'mkdir -p @0@ && cd @0@ && cmake @1@/subprojects/libjpeg-turbo -DCMAKE_BUILD_TYPE=Release -DWITH_MEM_SRCDST=ON -DWITH_TURBOJPEG=ON -DWITH_SIMD=ON -DENABLE_SHARED=ON -DENABLE_STATIC=ON && make -j$(nproc) turbojpeg turbojpeg-static && cp libturbojpeg.so* @2@/ && cp libturbojpeg.a @2@/'.format(libjpeg_build_dir, meson.current_source_dir(), meson.current_build_dir())
  ],
  output: ['stamp'],
  build_by_default: true,
  build_always_stale: true,
  console: true
)

# Create dependency using static library for better portability
turbojpeg_inc = include_directories('subprojects/libjpeg-turbo/src')
turbojpeg_static_lib_path = meson.current_build_dir() / 'libturbojpeg.a'
turbojpeg_dep = declare_dependency(
  link_args: [turbojpeg_static_lib_path],
  include_directories: turbojpeg_inc,
  sources: libjpeg_build
)

# Add version info to your code
plugin_c_args = ['-DHAVE_CONFIG_H', '-O3', '-ffast-math', '-funroll-loops', '-ftree-vectorize', '-march=native']

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

# Build benchmarks in subdirectory
subdir('benchmarks')
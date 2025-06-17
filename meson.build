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

turbojpeg_dep = dependency('libturbojpeg')
message('Using TurboJPEG: @0@ (version: @1@)'.format(
  turbojpeg_dep.name(), 
  turbojpeg_dep.version() != 'unknown' ? turbojpeg_dep.version() : 'unknown'
))

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

# Benchmark dependencies
benchmark_dep = dependency('benchmark', required: false)

if benchmark_dep.found()
  # Encoder benchmark
  encoder_benchmark = executable('encoder_benchmark',
    'benchmarks/encoder_benchmark.cpp',
    dependencies: [turbojpeg_dep, benchmark_dep],
    cpp_args: ['-std=c++11', '-O3', '-ffast-math', '-funroll-loops', '-ftree-vectorize', '-march=native'],
    install: false
  )
  
  # Decoder benchmark  
  decoder_benchmark = executable('decoder_benchmark',
    'benchmarks/decoder_benchmark.cpp',
    dependencies: [turbojpeg_dep, benchmark_dep],
    cpp_args: ['-std=c++11', '-O3', '-ffast-math', '-funroll-loops', '-ftree-vectorize', '-march=native'],
    install: false
  )
  
  # Register benchmarks
  benchmark('encoder_performance', encoder_benchmark)
  benchmark('decoder_performance', decoder_benchmark)
  
  message('Google Benchmark found - benchmark targets enabled')
else
  warning('Google Benchmark not found - benchmark targets disabled')
endif
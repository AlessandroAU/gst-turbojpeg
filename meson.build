project('gst-turbojpeg', ['c', 'cpp'],
  version : '1.0.0',
  license : 'LGPL',
  default_options : ['warning_level=1',
                     'buildtype=release',
                     'optimization=3',
                     'b_lto=true',
                     'b_ndebug=true'])

gst_version = '>=1.16.0'
gst_req = '>= 1.16.0'

gst_dep = dependency('gstreamer-1.0', version : gst_req)
gstbase_dep = dependency('gstreamer-base-1.0', version : gst_req)
gstvideo_dep = dependency('gstreamer-video-1.0', version : gst_req)

# Detect CPU features for optimal SIMD usage
cc = meson.get_compiler('c')
cpu_family = host_machine.cpu_family()

# Performance-oriented compilation flags
perf_c_args = [
  '-DHAVE_CONFIG_H',
  '-O3',                    # Maximum optimization
  '-ffast-math',           # Aggressive floating-point optimizations
  '-funroll-loops',        # Unroll loops for better performance
  '-ftree-vectorize',      # Enable auto-vectorization
  '-fomit-frame-pointer',  # Remove frame pointer for more registers
  '-finline-functions',    # Inline functions aggressively
  '-fno-signed-zeros',     # Optimize assuming no signed zeros
  '-fno-trapping-math',    # Assume no math exceptions
  '-fassociative-math',    # Allow reassociation of math operations
  '-freciprocal-math',     # Use reciprocal approximations
  '-DNDEBUG',              # Disable debug assertions
]

# Architecture-specific optimizations
if cpu_family == 'x86_64'
  # x86_64 specific optimizations
  if cc.has_argument('-march=native')
    perf_c_args += ['-march=native']  # Use all available CPU instructions
  else
    # Fallback to common x86_64 features
    perf_c_args += [
      '-march=x86-64-v3',   # AVX2, BMI1, BMI2, F16C, FMA, LZCNT, MOVBE, XSAVE
      '-mtune=generic'
    ]
  endif
  
  # Additional x86_64 optimizations
  if cc.has_argument('-mavx2')
    perf_c_args += ['-mavx2']
  endif
  if cc.has_argument('-mfma')
    perf_c_args += ['-mfma']
  endif
elif cpu_family == 'aarch64' or cpu_family == 'arm'
  # ARM specific optimizations with NEON
  if cc.has_argument('-march=native')
    perf_c_args += ['-march=native']
  else
    if cpu_family == 'aarch64'
      perf_c_args += ['-march=armv8-a+simd']
    else
      perf_c_args += ['-march=armv7-a']
    endif
  endif
  
  # Enable NEON for ARM platforms
  if cc.has_argument('-mfpu=neon')
    perf_c_args += ['-mfpu=neon']
  endif
  if cc.has_argument('-mfloat-abi=hard')
    perf_c_args += ['-mfloat-abi=hard']
  endif
endif

# Force building from submodule source with performance flags
message('Building TurboJPEG from submodule source with performance optimizations...')

libjpeg_build_dir = meson.current_build_dir() / 'libjpeg-turbo-build'

# Performance-optimized CMake flags for libjpeg-turbo
cmake_perf_flags = '-DCMAKE_C_FLAGS="-O3 -fPIC -march=native -ffast-math -funroll-loops -DNDEBUG"'
cmake_platform_flags = ''

if cpu_family == 'x86_64'
  cmake_platform_flags = '-DWITH_SIMD=ON -DREQUIRE_SIMD=ON'
elif cpu_family == 'aarch64' or cpu_family == 'arm'
  cmake_platform_flags = '-DWITH_SIMD=ON -DWITH_ARITH_ENC=ON -DWITH_ARITH_DEC=ON -DNEON_INTRINSICS=ON'
else
  cmake_platform_flags = '-DWITH_SIMD=OFF'
endif

# Custom target to build libjpeg-turbo with maximum performance
libjpeg_build = custom_target('libjpeg-turbo-build',
  command: [
    'sh', '-c', 
    'mkdir -p @0@ && cd @0@ && cmake @1@/subprojects/libjpeg-turbo -DCMAKE_BUILD_TYPE=Release -DWITH_TURBOJPEG=ON -DENABLE_SHARED=ON -DENABLE_STATIC=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON @2@ @3@ -DWITH_JAVA=OFF && make -j$(nproc) turbojpeg turbojpeg-static && cp libturbojpeg.so* @4@/ && cp libturbojpeg.a @4@/'.format(libjpeg_build_dir, meson.current_source_dir(), cmake_perf_flags, cmake_platform_flags, meson.current_build_dir())
  ],
  output: ['stamp'],
  build_by_default: true,
  build_always_stale: true,
  console: true
)

# Create dependency using static library
turbojpeg_inc = include_directories('subprojects/libjpeg-turbo/src')
turbojpeg_static_lib_path = meson.current_build_dir() / 'libturbojpeg.a'
turbojpeg_dep = declare_dependency(
  link_args: [turbojpeg_static_lib_path],
  include_directories: turbojpeg_inc,
  sources: libjpeg_build
)

# Version configuration
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

# Build the plugin with performance optimizations
gstturbojpeg = library('gstturbojpeg',
  plugin_sources,
  c_args: perf_c_args,
  link_args: ['-flto'],  # Link-time optimization
  dependencies : [gst_dep, gstbase_dep, gstvideo_dep, turbojpeg_dep],
  install : true,
  install_dir : join_paths(get_option('libdir'), 'gstreamer-1.0'),
)

# Build benchmarks in subdirectory
subdir('benchmarks')
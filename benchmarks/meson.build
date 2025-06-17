# Benchmarks for TurboJPEG encoder and decoder performance

# Check if benchmark dependency is available
benchmark_dep = dependency('benchmark', required: false)
gstreamer_dep = dependency('gstreamer-1.0', required: false)
gstreamer_app_dep = dependency('gstreamer-app-1.0', required: false)
gstreamer_video_dep = dependency('gstreamer-video-1.0', required: false)

if benchmark_dep.found()
  # Common benchmark arguments
  benchmark_cpp_args = [
    '-std=c++14', 
    '-O3', 
    '-ffast-math', 
    '-funroll-loops', 
    '-ftree-vectorize', 
    '-march=native'
  ]
  
  # Encoder benchmark (libturbojpeg)
  encoder_benchmark = executable('encoder_benchmark',
    ['encoder_benchmark.cpp', 'pattern_generator.cpp'],
    dependencies: [turbojpeg_dep, benchmark_dep],
    cpp_args: benchmark_cpp_args,
    install: false
  )
  
  # Decoder benchmark (libturbojpeg)
  decoder_benchmark = executable('decoder_benchmark',
    ['decoder_benchmark.cpp', 'pattern_generator.cpp'],
    dependencies: [turbojpeg_dep, benchmark_dep],
    cpp_args: benchmark_cpp_args,
    install: false
  )
  
  # GStreamer encoder benchmark
  if gstreamer_dep.found() and gstreamer_app_dep.found() and gstreamer_video_dep.found()
    gst_encoder_benchmark = executable('gst_encoder_benchmark',
      ['gst_encoder_benchmark.cpp', 'pattern_generator.cpp'],
      dependencies: [gstreamer_dep, gstreamer_app_dep, gstreamer_video_dep, benchmark_dep],
      cpp_args: benchmark_cpp_args,
      install: false
    )
    
    # GStreamer decoder benchmark
    gst_decoder_benchmark = executable('gst_decoder_benchmark',
      ['gst_decoder_benchmark.cpp', 'pattern_generator.cpp'],
      dependencies: [gstreamer_dep, gstreamer_app_dep, gstreamer_video_dep, benchmark_dep],
      cpp_args: benchmark_cpp_args,
      install: false
    )
    
    # Register GStreamer benchmarks
    benchmark('gst_encoder_performance', gst_encoder_benchmark)
    benchmark('gst_decoder_performance', gst_decoder_benchmark)
    
    message('GStreamer benchmarks enabled')
  else
    warning('GStreamer development libraries not found - GStreamer benchmarks disabled')
  endif
  
  # Pattern viewer utility
  pattern_viewer = executable('pattern_viewer',
    ['pattern_viewer.cpp', 'pattern_generator.cpp'],
    dependencies: [turbojpeg_dep],
    cpp_args: benchmark_cpp_args,
    install: false
  )
  
  # Register benchmarks
  benchmark('encoder_performance', encoder_benchmark)
  benchmark('decoder_performance', decoder_benchmark)
  
  message('Google Benchmark found - benchmark targets enabled')
  message('Pattern viewer utility enabled')
else
  warning('Google Benchmark not found - benchmark targets disabled')
endif
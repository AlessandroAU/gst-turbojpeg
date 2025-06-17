# Benchmarks for TurboJPEG encoder and decoder performance

# Check if benchmark dependency is available
benchmark_dep = dependency('benchmark', required: false)

if benchmark_dep.found()
  # Common benchmark arguments
  benchmark_cpp_args = [
    '-std=c++11', 
    '-O3', 
    '-ffast-math', 
    '-funroll-loops', 
    '-ftree-vectorize', 
    '-march=native'
  ]
  
  # Encoder benchmark
  encoder_benchmark = executable('encoder_benchmark',
    ['encoder_benchmark.cpp', 'pattern_generator.cpp'],
    dependencies: [turbojpeg_dep, benchmark_dep],
    cpp_args: benchmark_cpp_args,
    install: false
  )
  
  # Decoder benchmark  
  decoder_benchmark = executable('decoder_benchmark',
    ['decoder_benchmark.cpp', 'pattern_generator.cpp'],
    dependencies: [turbojpeg_dep, benchmark_dep],
    cpp_args: benchmark_cpp_args,
    install: false
  )
  
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
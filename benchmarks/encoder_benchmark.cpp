#include <benchmark/benchmark.h>
#include <turbojpeg.h>
#include <vector>
#include <memory>
#include <cstring>
#include <random>
#include <stdexcept>

class EncoderBenchmark {
public:
    EncoderBenchmark() {
        tjInstance = tjInitCompress();
        if (!tjInstance) {
            throw std::runtime_error("Failed to initialize TurboJPEG compressor");
        }
    }
    
    ~EncoderBenchmark() {
        if (tjInstance) {
            tjDestroy(tjInstance);
        }
    }
    
    void generateTestData(int width, int height, int pixelFormat) {
        int pixelSize = 3; // RGB
        if (pixelFormat == TJPF_GRAY) pixelSize = 1;
        else if (pixelFormat == TJPF_RGBA) pixelSize = 4;
        
        test_data.resize(width * height * pixelSize);
        
        // Generate some test pattern data
        std::mt19937 rng(42); // Fixed seed for reproducibility
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        
        for (size_t i = 0; i < test_data.size(); ++i) {
            test_data[i] = dist(rng);
        }
        
        // Pre-allocate JPEG buffer
        unsigned long maxJpegSize = tjBufSize(width, height, TJSAMP_420);
        jpeg_buffer.resize(maxJpegSize);
    }
    
    void benchmarkEncode(int width, int height, int quality, int subsampling, int pixelFormat) {
        unsigned long jpeg_size = jpeg_buffer.size();
        unsigned char* jpeg_ptr = jpeg_buffer.data();
        
        int flags = TJFLAG_FASTDCT | TJFLAG_NOREALLOC;
        
        if (tjCompress2(tjInstance, test_data.data(), width, 0, height,
                       pixelFormat, &jpeg_ptr, &jpeg_size, subsampling,
                       quality, flags) != 0) {
            throw std::runtime_error("TurboJPEG compression failed");
        }
    }
    
private:
    tjhandle tjInstance;
    std::vector<unsigned char> test_data;
    std::vector<unsigned char> jpeg_buffer;
};

static void BM_EncodeRGB_720p_Quality80(benchmark::State& state) {
    EncoderBenchmark encoder;
    encoder.generateTestData(1280, 720, TJPF_RGB);
    
    for (auto _ : state) {
        encoder.benchmarkEncode(1280, 720, 80, TJSAMP_420, TJPF_RGB);
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * 1280 * 720 * 3);
    
    // Calculate FPS: iterations per second
    state.counters["FPS"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
    state.SetLabel("720p RGB -> JPEG Q80 4:2:0");
}
BENCHMARK(BM_EncodeRGB_720p_Quality80)->Unit(benchmark::kMillisecond);

static void BM_EncodeRGB_1080p_Quality80(benchmark::State& state) {
    EncoderBenchmark encoder;
    encoder.generateTestData(1920, 1080, TJPF_RGB);
    
    for (auto _ : state) {
        encoder.benchmarkEncode(1920, 1080, 80, TJSAMP_420, TJPF_RGB);
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * 1920 * 1080 * 3);
    
    state.counters["FPS"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
    state.SetLabel("1080p RGB -> JPEG Q80 4:2:0");
}

static void BM_EncodeRGB_4K_Quality80(benchmark::State& state) {
    EncoderBenchmark encoder;
    encoder.generateTestData(3840, 2160, TJPF_RGB);
    
    for (auto _ : state) {
        encoder.benchmarkEncode(3840, 2160, 80, TJSAMP_420, TJPF_RGB);
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * 3840 * 2160 * 3);
    
    state.counters["FPS"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
    state.SetLabel("4K RGB -> JPEG Q80 4:2:0");
}

static void BM_EncodeRGB_Quality_Variations(benchmark::State& state) {
    EncoderBenchmark encoder;
    encoder.generateTestData(1920, 1080, TJPF_RGB);
    int quality = state.range(0);
    
    for (auto _ : state) {
        encoder.benchmarkEncode(1920, 1080, quality, TJSAMP_420, TJPF_RGB);
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * 1920 * 1080 * 3);
    
    state.counters["FPS"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
    state.SetLabel("1080p RGB -> JPEG Q" + std::to_string(quality) + " 4:2:0");
}

static void BM_EncodeRGB_Subsampling_Variations(benchmark::State& state) {
    EncoderBenchmark encoder;
    encoder.generateTestData(1920, 1080, TJPF_RGB);
    int subsampling = state.range(0);
    
    for (auto _ : state) {
        encoder.benchmarkEncode(1920, 1080, 80, subsampling, TJPF_RGB);
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * 1920 * 1080 * 3);
    
    state.counters["FPS"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
    
    std::string subsample_name;
    switch (subsampling) {
        case TJSAMP_444: subsample_name = "4:4:4"; break;
        case TJSAMP_422: subsample_name = "4:2:2"; break;
        case TJSAMP_420: subsample_name = "4:2:0"; break;
        case TJSAMP_GRAY: subsample_name = "GRAY"; break;
        default: subsample_name = "UNKNOWN"; break;
    }
    state.SetLabel("1080p RGB -> JPEG Q80 " + subsample_name);
}

// Register benchmarks
BENCHMARK(BM_EncodeRGB_1080p_Quality80)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_EncodeRGB_4K_Quality80)->Unit(benchmark::kMillisecond);

BENCHMARK(BM_EncodeRGB_Quality_Variations)->Arg(50)->Arg(75)->Arg(90)->Arg(95)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_EncodeRGB_Subsampling_Variations)->Arg(TJSAMP_444)->Arg(TJSAMP_422)->Arg(TJSAMP_420)->Arg(TJSAMP_GRAY)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
#include <benchmark/benchmark.h>
#include <turbojpeg.h>
#include <vector>
#include <memory>
#include <cstring>
#include <stdexcept>
#include "pattern_generator.h"

class DecoderBenchmark {
public:
    DecoderBenchmark() {
        compressor = tjInitCompress();
        decompressor = tjInitDecompress();
        if (!compressor || !decompressor) {
            throw std::runtime_error("Failed to initialize TurboJPEG instances");
        }
    }
    
    ~DecoderBenchmark() {
        if (compressor) tjDestroy(compressor);
        if (decompressor) tjDestroy(decompressor);
    }
    
    void generateJpegTestData(int width, int height, int quality, int subsampling) {
        // Generate RGB test data using SMPTE color bars - industry standard video test pattern
        std::vector<unsigned char> rgb_data = PatternGenerator::generateRGB(width, height, PatternGenerator::PatternType::SMPTE_COLOR_BARS);
        
        // Compress to JPEG
        unsigned long jpeg_size = tjBufSize(width, height, subsampling);
        jpeg_buffer.resize(jpeg_size);
        unsigned char* jpeg_ptr = jpeg_buffer.data();
        
        int flags = TJFLAG_FASTDCT;
        
        if (tjCompress2(compressor, rgb_data.data(), width, 0, height,
                       TJPF_RGB, &jpeg_ptr, &jpeg_size, subsampling,
                       quality, flags) != 0) {
            throw std::runtime_error("Failed to create test JPEG data");
        }
        
        jpeg_buffer.resize(jpeg_size); // Trim to actual size
        
        // Pre-allocate decode buffer for RGB output
        decode_buffer.resize(width * height * 3);
        
        // Pre-allocate YUV buffer and calculate layout (like GStreamer does)
        yuv_strides[0] = width;                    // Y plane stride
        yuv_strides[1] = (width + 1) / 2;         // U plane stride  
        yuv_strides[2] = (width + 1) / 2;         // V plane stride
        
        int y_plane_size = yuv_strides[0] * height;
        int u_plane_size = yuv_strides[1] * ((height + 1) / 2);
        int v_plane_size = yuv_strides[2] * ((height + 1) / 2);
        
        int total_yuv_size = y_plane_size + u_plane_size + v_plane_size;
        yuv_buffer.resize(total_yuv_size);
        
        // Pre-calculate plane pointers
        yuv_planes[0] = yuv_buffer.data();                    // Y plane
        yuv_planes[1] = yuv_planes[0] + y_plane_size;        // U plane
        yuv_planes[2] = yuv_planes[1] + u_plane_size;        // V plane
        
        this->width = width;
        this->height = height;
    }
    
    void benchmarkDecodeToRGB() {
        int decode_width, decode_height, decode_subsampling, decode_colorspace;
        
        if (tjDecompressHeader3(decompressor, jpeg_buffer.data(), jpeg_buffer.size(),
                               &decode_width, &decode_height, &decode_subsampling,
                               &decode_colorspace) != 0) {
            throw std::runtime_error("Failed to read JPEG header");
        }
        
        int flags = TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE;
        
        if (tjDecompress2(decompressor, jpeg_buffer.data(), jpeg_buffer.size(),
                         decode_buffer.data(), decode_width, 0, decode_height,
                         TJPF_RGB, flags) != 0) {
            throw std::runtime_error("TurboJPEG decompression failed");
        }
    }
    
    void benchmarkDecodeToYUV() {
        // Only the actual decompression is timed - buffers are pre-allocated
        int flags = TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE;
        
        if (tjDecompressToYUVPlanes(decompressor, jpeg_buffer.data(), jpeg_buffer.size(),
                                   yuv_planes, width, yuv_strides, height, flags) != 0) {
            throw std::runtime_error(std::string("TurboJPEG YUV decompression failed: ") + tjGetErrorStr());
        }
    }
    
private:
    tjhandle compressor;
    tjhandle decompressor;
    std::vector<unsigned char> jpeg_buffer;
    std::vector<unsigned char> decode_buffer;
    std::vector<unsigned char> yuv_buffer;
    int width, height;
    
    // Pre-allocated YUV decode structures
    int yuv_strides[3];
    unsigned char* yuv_planes[3];
};

static void BM_DecodeRGB_720p_Quality85(benchmark::State& state) {
    DecoderBenchmark decoder;
    decoder.generateJpegTestData(1280, 720, 85, TJSAMP_420);
    
    for (auto _ : state) {
        decoder.benchmarkDecodeToRGB();
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * 1280 * 720 * 3);
    
    state.counters["FPS"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
    state.SetLabel("720p SMPTE JPEG Q85 4:2:0 -> RGB");
}

static void BM_DecodeRGB_1080p_Quality85(benchmark::State& state) {
    DecoderBenchmark decoder;
    decoder.generateJpegTestData(1920, 1080, 85, TJSAMP_420);
    
    for (auto _ : state) {
        decoder.benchmarkDecodeToRGB();
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * 1920 * 1080 * 3);
    
    state.counters["FPS"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
    state.SetLabel("1080p SMPTE JPEG Q85 4:2:0 -> RGB");
}

static void BM_DecodeRGB_4K_Quality85(benchmark::State& state) {
    DecoderBenchmark decoder;
    decoder.generateJpegTestData(3840, 2160, 85, TJSAMP_420);
    
    for (auto _ : state) {
        decoder.benchmarkDecodeToRGB();
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * 3840 * 2160 * 3);
    
    state.counters["FPS"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
    state.SetLabel("4K SMPTE JPEG Q85 4:2:0 -> RGB");
}

static void BM_DecodeYUV_720p_Quality85(benchmark::State& state) {
    DecoderBenchmark decoder;
    decoder.generateJpegTestData(1280, 720, 85, TJSAMP_420);
    
    for (auto _ : state) {
        decoder.benchmarkDecodeToYUV();
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * 1280 * 720 * 3);
    
    state.counters["FPS"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
    state.SetLabel("720p SMPTE JPEG Q85 4:2:0 -> YUV");
}

static void BM_DecodeYUV_1080p_Quality85(benchmark::State& state) {
    DecoderBenchmark decoder;
    decoder.generateJpegTestData(1920, 1080, 85, TJSAMP_420);
    
    for (auto _ : state) {
        decoder.benchmarkDecodeToYUV();
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * 1920 * 1080 * 3);
    
    state.counters["FPS"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
    state.SetLabel("1080p SMPTE JPEG Q85 4:2:0 -> YUV");
}

static void BM_DecodeYUV_4K_Quality85(benchmark::State& state) {
    DecoderBenchmark decoder;
    decoder.generateJpegTestData(3840, 2160, 85, TJSAMP_420);
    
    for (auto _ : state) {
        decoder.benchmarkDecodeToYUV();
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * 3840 * 2160 * 3);
    
    state.counters["FPS"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
    state.SetLabel("4K SMPTE JPEG Q85 4:2:0 -> YUV");
}

static void BM_DecodeRGB_Quality_Variations(benchmark::State& state) {
    DecoderBenchmark decoder;
    int quality = state.range(0);
    decoder.generateJpegTestData(1920, 1080, quality, TJSAMP_420);
    
    for (auto _ : state) {
        decoder.benchmarkDecodeToRGB();
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * 1920 * 1080 * 3);
    
    state.counters["FPS"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
    state.SetLabel("1080p SMPTE JPEG Q" + std::to_string(quality) + " 4:2:0 -> RGB");
}

static void BM_DecodeRGB_Subsampling_Variations(benchmark::State& state) {
    DecoderBenchmark decoder;
    int subsampling = state.range(0);
    decoder.generateJpegTestData(1920, 1080, 85, subsampling);
    
    for (auto _ : state) {
        decoder.benchmarkDecodeToRGB();
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
    state.SetLabel("1080p SMPTE JPEG Q85 " + subsample_name + " -> RGB");
}

// Register benchmarks  
BENCHMARK(BM_DecodeRGB_720p_Quality85)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_DecodeRGB_1080p_Quality85)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_DecodeRGB_4K_Quality85)->Unit(benchmark::kMillisecond);

BENCHMARK(BM_DecodeYUV_720p_Quality85)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_DecodeYUV_1080p_Quality85)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_DecodeYUV_4K_Quality85)->Unit(benchmark::kMillisecond);

BENCHMARK(BM_DecodeRGB_Quality_Variations)->Arg(50)->Arg(75)->Arg(90)->Arg(95)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_DecodeRGB_Subsampling_Variations)->Arg(TJSAMP_444)->Arg(TJSAMP_422)->Arg(TJSAMP_420)->Arg(TJSAMP_GRAY)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
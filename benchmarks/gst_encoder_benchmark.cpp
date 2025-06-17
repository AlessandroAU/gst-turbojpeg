#include <benchmark/benchmark.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <vector>
#include <memory>
#include <cstring>
#include <stdexcept>
#include <chrono>
#include "pattern_generator.h"

class GstreamerEncoderBenchmark {
public:
    GstreamerEncoderBenchmark() : frames_processed(0) {
        gst_init(NULL, NULL);
        
        // Create pipeline: appsrc ! turbojpegenc ! appsink
        pipeline = gst_pipeline_new("encoder-benchmark");
        appsrc = gst_element_factory_make("appsrc", "source");
        encoder = gst_element_factory_make("turbojpegenc", "encoder");
        appsink = gst_element_factory_make("appsink", "sink");
        
        if (!pipeline || !appsrc || !encoder || !appsink) {
            throw std::runtime_error("Failed to create GStreamer elements");
        }
        
        // Add elements to pipeline
        gst_bin_add_many(GST_BIN(pipeline), appsrc, encoder, appsink, NULL);
        
        // Link elements
        if (!gst_element_link_many(appsrc, encoder, appsink, NULL)) {
            throw std::runtime_error("Failed to link GStreamer elements");
        }
        
        // Configure appsink
        g_object_set(appsink,
            "emit-signals", TRUE,
            "sync", FALSE,
            "async", FALSE,
            NULL);
    }
    
    ~GstreamerEncoderBenchmark() {
        if (pipeline) {
            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(pipeline);
        }
    }
    
    void setupPipeline(int width, int height, int quality, int subsampling, const std::string& format) {
        // Set encoder properties
        g_object_set(encoder,
            "quality", quality,
            "subsampling", subsampling,
            NULL);
        
        // Create caps for appsrc - fix sentinel warning
        GstCaps* caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, format.c_str(),
            "width", G_TYPE_INT, width,
            "height", G_TYPE_INT, height,
            "framerate", GST_TYPE_FRACTION, 30, 1,
            NULL);
        
        g_object_set(appsrc,
            "caps", caps,
            "format", GST_FORMAT_BYTES,
            "is-live", FALSE,
            NULL);
        
        gst_caps_unref(caps);
        
        // Set pipeline to playing state
        GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            throw std::runtime_error("Failed to start GStreamer pipeline");
        }
        
        // Wait for pipeline to reach playing state
        gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
    }
    
    void generateTestData(int width, int height, const std::string& format) {
        PatternGenerator::PatternType pattern = PatternGenerator::PatternType::SMPTE_COLOR_BARS;
        
        if (format == "RGB") {
            test_data = PatternGenerator::generateRGB(width, height, pattern);
            frame_size = width * height * 3;
        } else if (format == "I420") {
            // For I420, we'll generate RGB and convert to YUV420 manually
            // This is a simplified approach - in practice you'd want proper colorspace conversion
            auto rgb_data = PatternGenerator::generateRGB(width, height, pattern);
            test_data.resize(width * height * 3 / 2);
            
            // Simple RGB to YUV420 conversion (simplified)
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int rgb_idx = (y * width + x) * 3;
                    int y_idx = y * width + x;
                    
                    uint8_t r = rgb_data[rgb_idx];
                    uint8_t g = rgb_data[rgb_idx + 1];
                    uint8_t b = rgb_data[rgb_idx + 2];
                    
                    // Simple Y calculation
                    test_data[y_idx] = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
                }
            }
            
            // Fill U and V planes with neutral values for simplicity
            int uv_size = width * height / 4;
            std::fill(test_data.begin() + width * height, test_data.begin() + width * height + uv_size, 128);
            std::fill(test_data.begin() + width * height + uv_size, test_data.end(), 128);
            
            frame_size = width * height * 3 / 2;  // Y + U/2 + V/2
        } else {
            throw std::runtime_error("Unsupported format: " + format);
        }
    }
    
    void benchmarkEncode() {
        // Create buffer
        GstBuffer* buffer = gst_buffer_new_allocate(NULL, frame_size, NULL);
        GstMapInfo map;
        
        if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
            gst_buffer_unref(buffer);
            throw std::runtime_error("Failed to map buffer");
        }
        
        memcpy(map.data, test_data.data(), frame_size);
        gst_buffer_unmap(buffer, &map);
        
        // Push buffer to pipeline
        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
        if (ret != GST_FLOW_OK) {
            throw std::runtime_error("Failed to push buffer to pipeline");
        }
        
        // Wait for encoded frame - this ensures proper timing
        GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
        if (!sample) {
            throw std::runtime_error("Failed to get encoded sample");
        }
        gst_sample_unref(sample);
        frames_processed++;
    }
    
    void cleanup() {
        if (pipeline) {
            gst_element_set_state(pipeline, GST_STATE_NULL);
        }
    }
    
    size_t getFramesProcessed() const {
        return frames_processed;
    }
    
    void resetFrameCount() {
        frames_processed = 0;
    }

private:
    GstElement* pipeline;
    GstElement* appsrc;
    GstElement* encoder;
    GstElement* appsink;
    std::vector<unsigned char> test_data;
    size_t frame_size;
    size_t frames_processed;
};

// Global benchmark instance
static std::unique_ptr<GstreamerEncoderBenchmark> g_benchmark;

// Helper function to calculate FPS using manual timing
static void calculateAndSetFPS(benchmark::State& state, 
                              const std::chrono::high_resolution_clock::time_point& start_time,
                              const std::chrono::high_resolution_clock::time_point& end_time) {
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    double wall_time_seconds = duration.count() / 1e9;
    double fps = state.iterations() / wall_time_seconds;
    
    state.counters["FPS"] = benchmark::Counter(fps);
    //state.counters["AvgFrameTime_ms"] = benchmark::Counter(wall_time_seconds * 1000.0 / state.iterations());
}

// Benchmark functions
static void BM_GstEncodeRGB_720p_Quality80(benchmark::State& state) {
    const int width = 1280, height = 720, quality = 80, subsampling = 2; // 4:2:0
    
    g_benchmark.reset(new GstreamerEncoderBenchmark());
    g_benchmark->generateTestData(width, height, "RGB");
    g_benchmark->setupPipeline(width, height, quality, subsampling, "RGB");
    g_benchmark->resetFrameCount();
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        g_benchmark->benchmarkEncode();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    
    // Validate that we processed exactly one frame per iteration
    if (g_benchmark->getFramesProcessed() != static_cast<size_t>(state.iterations())) {
        state.SkipWithError(("Frame count mismatch: expected " + std::to_string(state.iterations()) + 
                           ", got " + std::to_string(g_benchmark->getFramesProcessed())).c_str());
        return;
    }
    
    // Calculate metrics
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * width * height * 3);
    calculateAndSetFPS(state, start_time, end_time);
    state.SetLabel("720p SMPTE RGB -> JPEG Q80 4:2:0");
    
    g_benchmark->cleanup();
    g_benchmark.reset();
}

static void BM_GstEncodeRGB_1080p_Quality80(benchmark::State& state) {
    const int width = 1920, height = 1080, quality = 80, subsampling = 2; // 4:2:0
    
    g_benchmark.reset(new GstreamerEncoderBenchmark());
    g_benchmark->generateTestData(width, height, "RGB");
    g_benchmark->setupPipeline(width, height, quality, subsampling, "RGB");
    g_benchmark->resetFrameCount();
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        g_benchmark->benchmarkEncode();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    
    // Validate that we processed exactly one frame per iteration
    if (g_benchmark->getFramesProcessed() != static_cast<size_t>(state.iterations())) {
        state.SkipWithError(("Frame count mismatch: expected " + std::to_string(state.iterations()) + 
                           ", got " + std::to_string(g_benchmark->getFramesProcessed())).c_str());
        return;
    }
    
    // Calculate metrics
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * width * height * 3);
    calculateAndSetFPS(state, start_time, end_time);
    state.SetLabel("1080p SMPTE RGB -> JPEG Q80 4:2:0");
    
    g_benchmark->cleanup();
    g_benchmark.reset();
}

static void BM_GstEncodeRGB_4K_Quality80(benchmark::State& state) {
    const int width = 3840, height = 2160, quality = 80, subsampling = 2; // 4:2:0
    
    g_benchmark.reset(new GstreamerEncoderBenchmark());
    g_benchmark->generateTestData(width, height, "RGB");
    g_benchmark->setupPipeline(width, height, quality, subsampling, "RGB");
    g_benchmark->resetFrameCount();
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        g_benchmark->benchmarkEncode();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    
    // Validate that we processed exactly one frame per iteration
    if (g_benchmark->getFramesProcessed() != static_cast<size_t>(state.iterations())) {
        state.SkipWithError(("Frame count mismatch: expected " + std::to_string(state.iterations()) + 
                           ", got " + std::to_string(g_benchmark->getFramesProcessed())).c_str());
        return;
    }
    
    // Calculate metrics
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * width * height * 3);
    calculateAndSetFPS(state, start_time, end_time);
    state.SetLabel("4K SMPTE RGB -> JPEG Q80 4:2:0");
    
    g_benchmark->cleanup();
    g_benchmark.reset();
}

static void BM_GstEncodeI420_1080p_Quality80(benchmark::State& state) {
    const int width = 1920, height = 1080, quality = 80, subsampling = 2; // 4:2:0
    
    g_benchmark.reset(new GstreamerEncoderBenchmark());
    g_benchmark->generateTestData(width, height, "I420");
    g_benchmark->setupPipeline(width, height, quality, subsampling, "I420");
    g_benchmark->resetFrameCount();
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        g_benchmark->benchmarkEncode();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    
    // Validate that we processed exactly one frame per iteration
    if (g_benchmark->getFramesProcessed() != static_cast<size_t>(state.iterations())) {
        state.SkipWithError(("Frame count mismatch: expected " + std::to_string(state.iterations()) + 
                           ", got " + std::to_string(g_benchmark->getFramesProcessed())).c_str());
        return;
    }
    
    // Calculate metrics
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * width * height * 1.5); // YUV420
    calculateAndSetFPS(state, start_time, end_time);
    state.SetLabel("1080p SMPTE I420 -> JPEG Q80 4:2:0");
    
    g_benchmark->cleanup();
    g_benchmark.reset();
}

static void BM_GstEncodeRGB_QualityVariations(benchmark::State& state) {
    const int width = 1920, height = 1080, subsampling = 2; // 4:2:0
    int quality = state.range(0);
    
    g_benchmark.reset(new GstreamerEncoderBenchmark());
    g_benchmark->generateTestData(width, height, "RGB");
    g_benchmark->setupPipeline(width, height, quality, subsampling, "RGB");
    g_benchmark->resetFrameCount();
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        g_benchmark->benchmarkEncode();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    
    // Validate that we processed exactly one frame per iteration
    if (g_benchmark->getFramesProcessed() != static_cast<size_t>(state.iterations())) {
        state.SkipWithError(("Frame count mismatch: expected " + std::to_string(state.iterations()) + 
                           ", got " + std::to_string(g_benchmark->getFramesProcessed())).c_str());
        return;
    }
    
    // Calculate metrics
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * width * height * 3);
    calculateAndSetFPS(state, start_time, end_time);
    state.SetLabel("1080p SMPTE RGB -> JPEG Q" + std::to_string(quality) + " 4:2:0");
    
    g_benchmark->cleanup();
    g_benchmark.reset();
}

static void BM_GstEncodeRGB_SubsamplingVariations(benchmark::State& state) {
    const int width = 1920, height = 1080, quality = 80;
    int subsampling = state.range(0);
    
    std::string subsample_name;
    switch (subsampling) {
        case 0: subsample_name = "4:4:4"; break;
        case 1: subsample_name = "4:2:2"; break;
        case 2: subsample_name = "4:2:0"; break;
        case 3: subsample_name = "GRAY"; break;
        default: subsample_name = "Unknown"; break;
    }
    
    g_benchmark.reset(new GstreamerEncoderBenchmark());
    g_benchmark->generateTestData(width, height, "RGB");
    g_benchmark->setupPipeline(width, height, quality, subsampling, "RGB");
    g_benchmark->resetFrameCount();
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        g_benchmark->benchmarkEncode();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    
    // Validate that we processed exactly one frame per iteration
    if (g_benchmark->getFramesProcessed() != static_cast<size_t>(state.iterations())) {
        state.SkipWithError(("Frame count mismatch: expected " + std::to_string(state.iterations()) + 
                           ", got " + std::to_string(g_benchmark->getFramesProcessed())).c_str());
        return;
    }
    
    // Calculate metrics
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * width * height * 3);
    calculateAndSetFPS(state, start_time, end_time);
    state.SetLabel("1080p SMPTE RGB -> JPEG Q80 " + subsample_name + "");
    
    g_benchmark->cleanup();
    g_benchmark.reset();
}

// Register benchmarks
BENCHMARK(BM_GstEncodeRGB_720p_Quality80)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_GstEncodeRGB_1080p_Quality80)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_GstEncodeRGB_4K_Quality80)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_GstEncodeI420_720p_Quality80)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_GstEncodeI420_1080p_Quality80)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_GstEncodeI420_4K_Quality80)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_GstEncodeRGB_QualityVariations)->Arg(50)->Arg(75)->Arg(90)->Arg(95)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_GstEncodeRGB_SubsamplingVariations)->Arg(0)->Arg(1)->Arg(2)->Arg(3)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
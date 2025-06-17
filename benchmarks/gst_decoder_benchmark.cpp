#include <benchmark/benchmark.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <vector>
#include <memory>
#include <cstring>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <chrono>

class GstreamerDecoderBenchmark {
public:
    GstreamerDecoderBenchmark() : frames_processed(0) {
        gst_init(NULL, NULL);
        
        // Create pipeline: appsrc ! turbojpegdec ! appsink
        pipeline = gst_pipeline_new("decoder-benchmark");
        appsrc = gst_element_factory_make("appsrc", "source");
        decoder = gst_element_factory_make("turbojpegdec", "decoder");
        appsink = gst_element_factory_make("appsink", "sink");
        
        if (!pipeline || !appsrc || !decoder || !appsink) {
            throw std::runtime_error("Failed to create GStreamer elements");
        }
        
        // Add elements to pipeline
        gst_bin_add_many(GST_BIN(pipeline), appsrc, decoder, appsink, NULL);
        
        // Link elements
        if (!gst_element_link_many(appsrc, decoder, appsink, NULL)) {
            throw std::runtime_error("Failed to link GStreamer elements");
        }
        
        // Configure appsink
        g_object_set(appsink,
            "emit-signals", TRUE,
            "sync", FALSE,
            "async", FALSE,
            NULL);
    }
    
    ~GstreamerDecoderBenchmark() {
        if (pipeline) {
            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(pipeline);
        }
    }
    
    void setupPipeline(const std::string& output_format) {
        // Create caps for appsrc (JPEG input) - fix sentinel warning
        GstCaps* input_caps = gst_caps_new_simple("image/jpeg", NULL, NULL);
        
        g_object_set(appsrc,
            "caps", input_caps,
            "format", GST_FORMAT_BYTES,
            "is-live", FALSE,
            NULL);
        
        gst_caps_unref(input_caps);
        
        // Create caps for appsink (decoded output)
        GstCaps* output_caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, output_format.c_str(),
            NULL);
        
        g_object_set(appsink,
            "caps", output_caps,
            NULL);
        
        gst_caps_unref(output_caps);
        
        // Set pipeline to playing state
        GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            throw std::runtime_error("Failed to start GStreamer pipeline");
        }
        
        // Wait for pipeline to reach playing state
        gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
    }
    
    bool loadJpegFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            return false;
        }
        
        // Get file size
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        // Read file data
        jpeg_data.resize(size);
        file.read(reinterpret_cast<char*>(jpeg_data.data()), size);
        
        return file.good();
    }
    
    void benchmarkDecode() {
        // Create buffer with JPEG data
        GstBuffer* buffer = gst_buffer_new_allocate(NULL, jpeg_data.size(), NULL);
        GstMapInfo map;
        
        if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
            gst_buffer_unref(buffer);
            throw std::runtime_error("Failed to map buffer");
        }
        
        memcpy(map.data, jpeg_data.data(), jpeg_data.size());
        gst_buffer_unmap(buffer, &map);
        
        // Push buffer to pipeline
        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
        if (ret != GST_FLOW_OK) {
            throw std::runtime_error("Failed to push buffer to pipeline");
        }
        
        // Wait for decoded frame - this ensures proper timing
        GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
        if (!sample) {
            throw std::runtime_error("Failed to get decoded sample");
        }
        gst_sample_unref(sample);
        frames_processed++;
    }
    
    void cleanup() {
        if (pipeline) {
            gst_element_set_state(pipeline, GST_STATE_NULL);
        }
    }
    
    size_t getJpegSize() const {
        return jpeg_data.size();
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
    GstElement* decoder;
    GstElement* appsink;
    std::vector<unsigned char> jpeg_data;
    size_t frames_processed;
};

// Global benchmark instance
static std::unique_ptr<GstreamerDecoderBenchmark> g_benchmark;

// Helper function to get frame size for different formats
static size_t getFrameSize(int width, int height, const std::string& format) {
    if (format == "RGB") {
        return width * height * 3;
    } else if (format == "I420") {
        return width * height * 3 / 2;
    }
    return 0;
}

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
static void BM_GstDecodeRGB_720p(benchmark::State& state) {
    const int width = 1280, height = 720;
    const std::string format = "RGB";
    const std::string jpeg_file = "test_patterns/smpte_color_bars_720p_smpte_color_bars_rgb.jpg";
    
    g_benchmark.reset(new GstreamerDecoderBenchmark());
    
    if (!g_benchmark->loadJpegFile(jpeg_file)) {
        state.SkipWithError(("Failed to load JPEG file: " + jpeg_file).c_str());
        return;
    }
    
    g_benchmark->setupPipeline(format);
    g_benchmark->resetFrameCount();
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        g_benchmark->benchmarkDecode();
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
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * getFrameSize(width, height, format));
    calculateAndSetFPS(state, start_time, end_time);
    state.SetLabel("720p SMPTE JPEG -> " + format + "");
    
    g_benchmark->cleanup();
    g_benchmark.reset();
}

static void BM_GstDecodeRGB_1080p(benchmark::State& state) {
    const int width = 1920, height = 1080;
    const std::string format = "RGB";
    const std::string jpeg_file = "test_patterns/smpte_color_bars_1080p_smpte_color_bars_rgb.jpg";
    
    g_benchmark.reset(new GstreamerDecoderBenchmark());
    
    if (!g_benchmark->loadJpegFile(jpeg_file)) {
        state.SkipWithError(("Failed to load JPEG file: " + jpeg_file).c_str());
        return;
    }
    
    g_benchmark->setupPipeline(format);
    g_benchmark->resetFrameCount();
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        g_benchmark->benchmarkDecode();
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
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * getFrameSize(width, height, format));
    calculateAndSetFPS(state, start_time, end_time);
    state.SetLabel("1080p SMPTE JPEG -> " + format + "");
    
    g_benchmark->cleanup();
    g_benchmark.reset();
}

static void BM_GstDecodeRGB_4K(benchmark::State& state) {
    const int width = 3840, height = 2160;
    const std::string format = "RGB";
    const std::string jpeg_file = "test_patterns/smpte_color_bars_4k_smpte_color_bars_rgb.jpg";
    
    g_benchmark.reset(new GstreamerDecoderBenchmark());
    
    if (!g_benchmark->loadJpegFile(jpeg_file)) {
        state.SkipWithError(("Failed to load JPEG file: " + jpeg_file).c_str());
        return;
    }
    
    g_benchmark->setupPipeline(format);
    g_benchmark->resetFrameCount();
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        g_benchmark->benchmarkDecode();
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
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * getFrameSize(width, height, format));
    calculateAndSetFPS(state, start_time, end_time);
    state.SetLabel("4K SMPTE JPEG -> " + format + "");
    
    g_benchmark->cleanup();
    g_benchmark.reset();
}

static void BM_GstDecodeI420_1080p(benchmark::State& state) {
    const int width = 1920, height = 1080;
    const std::string format = "I420";
    const std::string jpeg_file = "test_patterns/smpte_color_bars_1080p_smpte_color_bars_rgb.jpg";
    
    g_benchmark.reset(new GstreamerDecoderBenchmark());
    
    if (!g_benchmark->loadJpegFile(jpeg_file)) {
        state.SkipWithError(("Failed to load JPEG file: " + jpeg_file).c_str());
        return;
    }
    
    g_benchmark->setupPipeline(format);
    g_benchmark->resetFrameCount();
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        g_benchmark->benchmarkDecode();
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
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * getFrameSize(width, height, format));
    calculateAndSetFPS(state, start_time, end_time);
    state.SetLabel("1080p SMPTE JPEG -> " + format + "");
    
    g_benchmark->cleanup();
    g_benchmark.reset();
}

static void BM_GstDecodeRGB_PatternVariations(benchmark::State& state) {
    const int pattern_type = state.range(0);
    const int width = 1920, height = 1080;
    const std::string format = "RGB";
    
    // Map pattern type to corresponding test file
    std::string jpeg_file;
    std::string pattern_name;
    switch (pattern_type) {
        case 0:
            jpeg_file = "test_patterns/checkerboard_1080p_checkerboard_rgb.jpg";
            pattern_name = "Checkerboard";
            break;
        case 1:
            jpeg_file = "test_patterns/gradient_1080p_gradient_rgb.jpg";
            pattern_name = "Gradient";
            break;
        default:
            jpeg_file = "test_patterns/smpte_color_bars_1080p_smpte_color_bars_rgb.jpg";
            pattern_name = "SMPTE";
            break;
    }
    
    g_benchmark.reset(new GstreamerDecoderBenchmark());
    
    if (!g_benchmark->loadJpegFile(jpeg_file)) {
        state.SkipWithError(("Failed to load JPEG file: " + jpeg_file).c_str());
        return;
    }
    
    g_benchmark->setupPipeline(format);
    g_benchmark->resetFrameCount();
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        g_benchmark->benchmarkDecode();
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
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * getFrameSize(width, height, format));
    calculateAndSetFPS(state, start_time, end_time);
    state.SetLabel("1080p " + pattern_name + " JPEG -> " + format + "");
    
    g_benchmark->cleanup();
    g_benchmark.reset();
}

// Register benchmarks
BENCHMARK(BM_GstDecodeRGB_720p)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_GstDecodeRGB_1080p)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_GstDecodeRGB_4K)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_GstDecodeI420_1080p)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_GstDecodeRGB_PatternVariations)->Arg(0)->Arg(1)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
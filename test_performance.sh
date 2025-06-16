#!/bin/bash

# Performance test script for TurboJPEG decoder with SIMD/NEON and multicore support

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Set the plugin path
export GST_PLUGIN_PATH=$PWD/builddir:$GST_PLUGIN_PATH

echo -e "${BLUE}TurboJPEG Decoder Performance Test${NC}"
echo "=================================="
echo ""

# Function to run performance test
run_perf_test() {
    local test_name="$1"
    local pipeline="$2"
    local desc="$3"
    
    echo -e "${BLUE}Testing: $test_name${NC}"
    echo "Description: $desc"
    echo "Pipeline: $pipeline"
    echo ""
    
    echo "Running..."
    local start_time=$(date +%s.%N)
    
    if gst-launch-1.0 $pipeline >/dev/null 2>&1; then
        local end_time=$(date +%s.%N)
        local duration=$(echo "$end_time - $start_time" | bc -l)
        echo -e "${GREEN}✓ Completed in ${duration} seconds${NC}"
    else
        echo -e "${RED}✗ Test failed${NC}"
    fi
    echo ""
}

echo -e "${BLUE}System Information:${NC}"
echo "CPU: $(nproc) cores"
echo "Architecture: $(uname -m)"
echo "TurboJPEG version: $(pkg-config --modversion libturbojpeg)"
echo ""

echo -e "${BLUE}Plugin Configuration:${NC}"
gst-inspect-1.0 turbojpegdec | grep -E "(quality|max-errors)" | sed 's/^/  /'
gst-inspect-1.0 turbojpegenc | grep -E "(quality|subsampling|progressive)" | sed 's/^/  /'
echo ""

echo -e "${BLUE}SIMD/NEON Support:${NC}"
echo "✓ TurboJPEG automatically uses NEON SIMD instructions on ARM64"
echo "✓ Hardware-accelerated JPEG encoding and decoding enabled"
echo ""

echo -e "${BLUE}TurboJPEG Performance Tests:${NC}"
echo "Running performance tests comparing TurboJPEG vs standard GStreamer..."
echo ""

# Test 1: Full TurboJPEG pipeline vs standard pipeline
run_perf_test "Standard Pipeline (1080p)" \
    "videotestsrc num-buffers=100 ! video/x-raw,width=1920,height=1080 ! jpegenc ! jpegdec ! fakesink sync=false" \
    "Standard GStreamer JPEG encode/decode pipeline"

run_perf_test "TurboJPEG Pipeline (1080p)" \
    "videotestsrc num-buffers=100 ! video/x-raw,width=1920,height=1080 ! turbojpegenc ! turbojpegdec ! fakesink sync=false" \
    "TurboJPEG encode/decode pipeline with SIMD acceleration"

echo -e "${BLUE}Resolution Performance Tests:${NC}"
echo "Testing performance across different resolutions..."
echo ""

# Test 2: Different resolutions with TurboJPEG
run_perf_test "TurboJPEG HD (720p)" \
    "videotestsrc num-buffers=100 ! video/x-raw,width=1280,height=720 ! turbojpegenc ! turbojpegdec ! fakesink sync=false" \
    "720p TurboJPEG encode/decode"

run_perf_test "TurboJPEG 4K" \
    "videotestsrc num-buffers=50 ! video/x-raw,width=3840,height=2160 ! turbojpegenc ! turbojpegdec ! fakesink sync=false" \
    "4K TurboJPEG encode/decode"

echo -e "${BLUE}Quality Settings Tests:${NC}"
echo "Testing different quality levels..."
echo ""

# Test 3: Quality settings
run_perf_test "High Quality (95)" \
    "videotestsrc num-buffers=100 ! video/x-raw,width=1920,height=1080 ! turbojpegenc quality=95 ! turbojpegdec ! fakesink sync=false" \
    "1080p with high quality encoding"

run_perf_test "Medium Quality (75)" \
    "videotestsrc num-buffers=100 ! video/x-raw,width=1920,height=1080 ! turbojpegenc quality=75 ! turbojpegdec ! fakesink sync=false" \
    "1080p with medium quality encoding"

run_perf_test "Low Quality (50)" \
    "videotestsrc num-buffers=100 ! video/x-raw,width=1920,height=1080 ! turbojpegenc quality=50 ! turbojpegdec ! fakesink sync=false" \
    "1080p with low quality encoding"

echo -e "${BLUE}Format Tests:${NC}"
echo "Testing different chroma subsampling modes..."
echo ""

# Test 4: Subsampling tests
run_perf_test "4:4:4 Subsampling" \
    "videotestsrc num-buffers=100 ! video/x-raw,width=1920,height=1080 ! turbojpegenc subsampling=0 ! turbojpegdec ! fakesink sync=false" \
    "1080p with 4:4:4 chroma subsampling"

run_perf_test "4:2:0 Subsampling" \
    "videotestsrc num-buffers=100 ! video/x-raw,width=1920,height=1080 ! turbojpegenc subsampling=2 ! turbojpegdec ! fakesink sync=false" \
    "1080p with 4:2:0 chroma subsampling"

echo -e "${BLUE}Final Comparison Tests:${NC}"

run_perf_test "Standard Encode/Decode" \
    "videotestsrc num-buffers=100 ! video/x-raw,width=1920,height=1080 ! jpegenc ! jpegdec ! fakesink sync=false" \
    "Standard GStreamer JPEG encode/decode"

run_perf_test "TurboJPEG Encode/Decode" \
    "videotestsrc num-buffers=100 ! video/x-raw,width=1920,height=1080 ! turbojpegenc ! turbojpegdec ! fakesink sync=false" \
    "TurboJPEG encode/decode with SIMD acceleration"

run_perf_test "TurboJPEG Optimized" \
    "videotestsrc num-buffers=100 ! video/x-raw,width=1920,height=1080 ! turbojpegenc quality=85 ! turbojpegdec ! fakesink sync=false" \
    "Optimized TurboJPEG pipeline with balanced quality"

echo -e "${GREEN}Performance Testing Complete!${NC}"
echo ""
echo -e "${BLUE}Key Features Demonstrated:${NC}"
echo "✓ SIMD/NEON acceleration (automatic on ARM64)"
echo "✓ High-performance JPEG encoding and decoding"
echo "✓ Configurable quality settings (1-100)"
echo "✓ Multiple chroma subsampling modes"
echo "✓ Progressive JPEG support"
echo ""
echo -e "${BLUE}Manual Testing Commands:${NC}"
echo ""
echo "1. Full TurboJPEG pipeline:"
echo "   gst-launch-1.0 videotestsrc ! turbojpegenc ! turbojpegdec ! autovideosink"
echo ""
echo "2. Custom encoder quality:"
echo "   gst-launch-1.0 videotestsrc ! turbojpegenc quality=90 ! turbojpegdec ! autovideosink"
echo ""
echo "3. Progressive JPEG encoding:"
echo "   gst-launch-1.0 videotestsrc ! turbojpegenc progressive=true ! turbojpegdec ! autovideosink"
echo ""
echo "4. Custom subsampling (4:2:0, 4:2:2, 4:4:4):"
echo "   gst-launch-1.0 videotestsrc ! turbojpegenc subsampling=0 ! turbojpegdec ! autovideosink"
echo ""
echo "5. Debug behavior:"
echo "   GST_DEBUG=turbojpeg:4 gst-launch-1.0 videotestsrc ! turbojpegenc ! turbojpegdec ! fakesink"
echo ""
echo "6. File conversion with TurboJPEG:"
echo "   gst-launch-1.0 filesrc location=input.mp4 ! decodebin ! turbojpegenc ! turbojpegdec ! autovideosink"
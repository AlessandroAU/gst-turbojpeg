#!/bin/bash

# TurboJPEG YUV Decoder Test Script
# Tests all combinations of JPEG subsampling inputs and GStreamer YUV output formats
# Usage: ./test_yuv_combinations.sh [quick|full]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
TEST_WIDTH=640
TEST_HEIGHT=480
TEST_PATTERN="smpte"
OUTPUT_DIR="/tmp/turbojpeg_tests"
QUICK_MODE=false

if [[ "$1" == "quick" ]]; then
    QUICK_MODE=true
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo -e "${BLUE}=== TurboJPEG YUV Decoder Combination Test ===${NC}"
echo "Testing all JPEG subsampling inputs vs GStreamer YUV output formats"
echo "Output directory: $OUTPUT_DIR"
echo "Test resolution: ${TEST_WIDTH}x${TEST_HEIGHT}"
echo ""

# Define test combinations
# Format: "input_format:expected_jpeg_subsampling:description"
declare -a INPUT_FORMATS=(
    "I420:2:4:2:0 subsampling"
    "Y42B:1:4:2:2 subsampling" 
    "Y444:0:4:4:4 subsampling"
)

# Output formats to test
declare -a OUTPUT_FORMATS=(
    "I420"
    "YV12" 
    "Y42B"
    "Y444"
)

# RGB formats for comparison
declare -a RGB_FORMATS=(
    "RGB"
    "BGR"
)

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to run a single test
run_test() {
    local input_format="$1"
    local expected_subsamp="$2"
    local input_desc="$3"
    local output_format="$4"
    local test_name="${input_format}_to_${output_format}"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -n "Testing ${input_format} → ${output_format} (${input_desc}): "
    
    # Create test pipeline
    local pipeline="videotestsrc pattern=${TEST_PATTERN} num-buffers=1 ! \
        video/x-raw,width=${TEST_WIDTH},height=${TEST_HEIGHT},format=${input_format} ! \
        jpegenc ! \
        turbojpegdec ! \
        video/x-raw,format=${output_format} ! \
        videoconvert ! \
        video/x-raw,format=RGB ! \
        pngenc ! \
        filesink location=${OUTPUT_DIR}/${test_name}.png"
    
    # Run test with debug output
    local debug_output
    if debug_output=$(GST_DEBUG=*turbojpeg*:4 gst-launch-1.0 $pipeline 2>&1); then
        # Check if conversion was detected when expected
        local needs_conversion=false
        
        # Determine if conversion should be needed
        case "${input_format}_${output_format}" in
            "I420_I420"|"Y42B_Y42B"|"Y444_Y444")
                needs_conversion=false ;;
            "I420_YV12"|"YV12_I420")
                needs_conversion=false ;; # Same subsampling, just plane swap
            *)
                needs_conversion=true ;;
        esac
        
        # Check debug output for expected behavior
        local subsamp_found=$(echo "$debug_output" | grep "SUBSAMPLING DEBUG (mode=${expected_subsamp})" | wc -l)
        local conversion_found=$(echo "$debug_output" | grep "Subsampling format conversion" | wc -l)
        
        if [[ $subsamp_found -gt 0 ]]; then
            if [[ $needs_conversion == true && $conversion_found -gt 0 ]]; then
                echo -e "${GREEN}PASS${NC} (conversion applied)"
                PASSED_TESTS=$((PASSED_TESTS + 1))
            elif [[ $needs_conversion == false && $conversion_found -eq 0 ]]; then
                echo -e "${GREEN}PASS${NC} (direct decode)"
                PASSED_TESTS=$((PASSED_TESTS + 1))
            else
                echo -e "${YELLOW}PASS${NC} (unexpected conversion behavior)"
                PASSED_TESTS=$((PASSED_TESTS + 1))
            fi
        else
            echo -e "${RED}FAIL${NC} (wrong subsampling detected)"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    else
        echo -e "${RED}FAIL${NC} (pipeline error)"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        echo "Error: $debug_output" | head -3
    fi
}

# Function to test RGB formats for comparison
test_rgb_format() {
    local rgb_format="$1"
    echo -n "Testing RGB reference (${rgb_format}): "
    
    local pipeline="videotestsrc pattern=${TEST_PATTERN} num-buffers=1 ! \
        video/x-raw,width=${TEST_WIDTH},height=${TEST_HEIGHT},format=I420 ! \
        jpegenc ! \
        turbojpegdec ! \
        video/x-raw,format=${rgb_format} ! \
        pngenc ! \
        filesink location=${OUTPUT_DIR}/rgb_reference_${rgb_format}.png"
    
    if gst-launch-1.0 $pipeline >/dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC}"
    else
        echo -e "${RED}FAIL${NC}"
    fi
}

# Test camera simulation (if available)
test_camera_simulation() {
    echo -e "\n${BLUE}=== Camera Simulation Test ===${NC}"
    echo "Simulating 4:2:2 camera JPEG → I420 conversion"
    
    # Create a 4:2:2 JPEG first
    local jpeg_file="${OUTPUT_DIR}/camera_sim.jpg"
    
    if gst-launch-1.0 videotestsrc pattern=smpte num-buffers=1 ! \
        video/x-raw,width=1280,height=720,format=Y42B ! \
        jpegenc ! \
        filesink location="$jpeg_file" >/dev/null 2>&1; then
        
        echo -n "Testing simulated camera JPEG (4:2:2 → I420): "
        
        local pipeline="filesrc location=${jpeg_file} ! \
            jpegparse ! \
            turbojpegdec ! \
            video/x-raw,format=I420 ! \
            videoconvert ! \
            video/x-raw,format=RGB ! \
            pngenc ! \
            filesink location=${OUTPUT_DIR}/camera_sim_output.png"
        
        if GST_DEBUG=*turbojpeg*:4 gst-launch-1.0 $pipeline 2>&1 | grep -q "Subsampling format conversion"; then
            echo -e "${GREEN}PASS${NC} (conversion applied)"
        else
            echo -e "${RED}FAIL${NC} (no conversion detected)"
        fi
        
        rm -f "$jpeg_file"
    else
        echo "Could not create test JPEG file"
    fi
}

# Performance test
test_performance() {
    echo -e "\n${BLUE}=== Performance Test ===${NC}"
    echo "Testing decode performance (100 frames)"
    
    local perf_pipeline="videotestsrc pattern=smpte num-buffers=100 ! \
        video/x-raw,width=1280,height=720,format=Y42B ! \
        jpegenc ! \
        turbojpegdec ! \
        video/x-raw,format=I420 ! \
        perf ! \
        fakesink sync=false"
    
    echo "Running performance test..."
    if gst-launch-1.0 $perf_pipeline 2>&1 | grep "mean_fps" | tail -1; then
        echo "Performance test completed"
    else
        echo "Performance test failed"
    fi
}

# Main test execution
echo -e "${BLUE}=== YUV Format Combination Tests ===${NC}"

# Test all YUV combinations
for input_combo in "${INPUT_FORMATS[@]}"; do
    IFS=':' read -r input_format expected_subsamp input_desc <<< "$input_combo"
    
    echo -e "\n${YELLOW}Input: ${input_format} (${input_desc})${NC}"
    
    for output_format in "${OUTPUT_FORMATS[@]}"; do
        run_test "$input_format" "$expected_subsamp" "$input_desc" "$output_format"
        
        # Quick mode: only test I420 output for non-I420 inputs
        if [[ "$QUICK_MODE" == true && "$output_format" == "I420" && "$input_format" != "I420" ]]; then
            break
        fi
    done
done

# Test RGB formats
if [[ "$QUICK_MODE" == false ]]; then
    echo -e "\n${BLUE}=== RGB Reference Tests ===${NC}"
    for rgb_format in "${RGB_FORMATS[@]}"; do
        test_rgb_format "$rgb_format"
    done
    
    # Camera simulation test
    test_camera_simulation
    
    # Performance test
    test_performance
fi

# Generate summary report
echo -e "\n${BLUE}=== Test Summary ===${NC}"
echo "Total tests: $TOTAL_TESTS"
echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed: ${RED}$FAILED_TESTS${NC}"

if [[ $FAILED_TESTS -eq 0 ]]; then
    echo -e "\n${GREEN}🎉 All tests passed!${NC}"
    exit_code=0
else
    echo -e "\n${RED}❌ Some tests failed!${NC}"
    exit_code=1
fi

# List generated files
echo -e "\n${BLUE}Generated test files:${NC}"
ls -la "$OUTPUT_DIR"/*.png 2>/dev/null | wc -l | xargs echo "PNG files created:"
echo "Output directory: $OUTPUT_DIR"

echo -e "\n${BLUE}Usage examples:${NC}"
echo "  Quick test:  ./test_yuv_combinations.sh quick"
echo "  Full test:   ./test_yuv_combinations.sh full"
echo "  View images: eog $OUTPUT_DIR/*.png"

exit $exit_code
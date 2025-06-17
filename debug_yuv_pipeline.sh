#!/bin/bash

# TurboJPEG YUV Decoder Debug Script
# Provides detailed debugging for specific format combinations
# Usage: ./debug_yuv_pipeline.sh [input_format] [output_format] [width] [height]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default parameters
INPUT_FORMAT="${1:-Y42B}"
OUTPUT_FORMAT="${2:-I420}"
WIDTH="${3:-1280}"
HEIGHT="${4:-720}"
OUTPUT_DIR="turbojpeg_debug"

mkdir -p "$OUTPUT_DIR"

echo -e "${BLUE}=== TurboJPEG YUV Debug Pipeline ===${NC}"
echo "Input format: $INPUT_FORMAT"
echo "Output format: $OUTPUT_FORMAT" 
echo "Resolution: ${WIDTH}x${HEIGHT}"
echo "Output directory: $OUTPUT_DIR"
echo ""

# Function to extract debug info
extract_debug_info() {
    local debug_output="$1"
    
    echo -e "${CYAN}=== Debug Information ===${NC}"
    
    # Extract subsampling mode
    local subsamp_mode=$(echo "$debug_output" | grep "SUBSAMPLING DEBUG (mode=" | head -1 | sed 's/.*mode=\([0-9]\).*/\1/')
    case "$subsamp_mode" in
        0) echo "JPEG Subsampling: 4:4:4 (mode 0)" ;;
        1) echo "JPEG Subsampling: 4:2:2 (mode 1)" ;;
        2) echo "JPEG Subsampling: 4:2:0 (mode 2)" ;;
        *) echo "JPEG Subsampling: Unknown (mode $subsamp_mode)" ;;
    esac
    
    # Extract dimensions
    echo "$debug_output" | grep "JPEG dimensions:" | head -1
    echo "$debug_output" | grep "GStreamer.*plane:" | head -3
    echo "$debug_output" | grep "TurboJPEG.*plane:" | head -3
    
    # Check for conversions
    if echo "$debug_output" | grep -q "Subsampling format conversion needed"; then
        echo -e "${YELLOW}⚠ Subsampling format conversion required${NC}"
        echo "$debug_output" | grep "plane MISMATCH" | head -2
    else
        echo -e "${GREEN}✓ Direct decode (no conversion needed)${NC}"
    fi
    
    if echo "$debug_output" | grep -q "Subsampling format conversion completed"; then
        echo -e "${GREEN}✓ Subsampling conversion completed successfully${NC}"
    fi
    
    # Check for errors
    if echo "$debug_output" | grep -q "ERROR"; then
        echo -e "${RED}❌ Errors detected:${NC}"
        echo "$debug_output" | grep "ERROR" | head -3
    fi
}

# Function to create comparison images
create_comparison() {
    echo -e "\n${BLUE}=== Creating Comparison Images ===${NC}"
    
    # RGB reference
    echo "Creating RGB reference..."
    local rgb_pipeline="videotestsrc pattern=smpte num-buffers=1 ! \
        video/x-raw,width=${WIDTH},height=${HEIGHT},format=${INPUT_FORMAT} ! \
        jpegenc ! \
        turbojpegdec ! \
        video/x-raw,format=RGB ! \
        pngenc ! \
        filesink location=${OUTPUT_DIR}/rgb_reference.png"
    
    gst-launch-1.0 $rgb_pipeline >/dev/null 2>&1 || echo "RGB reference failed"
    
    # YUV output
    echo "Creating YUV output..."
    local yuv_pipeline="videotestsrc pattern=smpte num-buffers=1 ! \
        video/x-raw,width=${WIDTH},height=${HEIGHT},format=${INPUT_FORMAT} ! \
        jpegenc ! \
        turbojpegdec ! \
        video/x-raw,format=${OUTPUT_FORMAT} ! \
        videoconvert ! \
        video/x-raw,format=RGB ! \
        pngenc ! \
        filesink location=${OUTPUT_DIR}/yuv_output.png"
    
    gst-launch-1.0 $yuv_pipeline >/dev/null 2>&1 || echo "YUV output failed"
    
    echo "Comparison images created:"
    echo "  RGB reference: ${OUTPUT_DIR}/rgb_reference.png"
    echo "  YUV output:    ${OUTPUT_DIR}/yuv_output.png"
}

# Function to test camera scenario
test_camera_scenario() {
    if [[ "$INPUT_FORMAT" == "camera" ]]; then
        echo -e "\n${BLUE}=== Camera Test Mode ===${NC}"
        echo "Testing with camera device /dev/video0"
        
        local camera_pipeline="v4l2src device=/dev/video0 num-buffers=1 ! \
            image/jpeg,width=${WIDTH},height=${HEIGHT} ! \
            turbojpegdec ! \
            video/x-raw,format=${OUTPUT_FORMAT} ! \
            videoconvert ! \
            video/x-raw,format=RGB ! \
            pngenc ! \
            filesink location=${OUTPUT_DIR}/camera_output.png"
        
        echo "Running camera pipeline with full debug output..."
        GST_DEBUG=*turbojpeg*:7 gst-launch-1.0 $camera_pipeline 2>&1 | tee "${OUTPUT_DIR}/camera_debug.log"
        
        echo "Camera debug log saved to: ${OUTPUT_DIR}/camera_debug.log"
        return 0
    fi
}

# Function to run performance test
run_performance_test() {
    echo -e "\n${BLUE}=== Performance Test ===${NC}"
    echo "Testing performance with 50 frames..."
    
    local perf_pipeline="videotestsrc pattern=smpte num-buffers=50 ! \
        video/x-raw,width=${WIDTH},height=${HEIGHT},format=${INPUT_FORMAT} ! \
        jpegenc ! \
        turbojpegdec ! \
        video/x-raw,format=${OUTPUT_FORMAT} ! \
        perf ! \
        fakesink sync=false"
    
    gst-launch-1.0 $perf_pipeline 2>&1 | grep "perf:" | tail -1
}

# Main debug pipeline
main_debug_pipeline() {
    echo -e "${CYAN}=== Running Debug Pipeline ===${NC}"
    
    local main_pipeline="videotestsrc pattern=smpte num-buffers=1 ! \
        video/x-raw,width=${WIDTH},height=${HEIGHT},format=${INPUT_FORMAT} ! \
        jpegenc ! \
        turbojpegdec ! \
        video/x-raw,format=${OUTPUT_FORMAT} ! \
        fakesink sync=false"
    
    echo "Pipeline: $main_pipeline"
    echo ""
    
    local debug_output
    if debug_output=$(GST_DEBUG=*turbojpeg*:4 gst-launch-1.0 $main_pipeline 2>&1); then
        extract_debug_info "$debug_output"
        echo -e "\n${GREEN}✓ Pipeline executed successfully${NC}"
        return 0
    else
        echo -e "\n${RED}❌ Pipeline failed${NC}"
        echo "$debug_output" | tail -10
        return 1
    fi
}

# Function to show supported formats
show_supported_formats() {
    echo -e "\n${BLUE}=== Supported Formats ===${NC}"
    echo "Input formats:"
    echo "  I420  - 4:2:0 planar (creates 4:2:0 JPEG)"
    echo "  Y42B  - 4:2:2 planar (creates 4:2:2 JPEG)"  
    echo "  Y444  - 4:4:4 planar (creates 4:4:4 JPEG)"
    echo "  camera - Use real camera input"
    echo ""
    echo "Output formats:"
    echo "  I420  - 4:2:0 planar"
    echo "  YV12  - 4:2:0 planar (V/U swapped)"
    echo "  Y42B  - 4:2:2 planar"
    echo "  Y444  - 4:4:4 planar"
    echo ""
    echo "Expected conversions:"
    echo "  4:4:4 → 4:2:0: Downsample both horizontally and vertically"
    echo "  4:2:2 → 4:2:0: Downsample vertically only"
    echo "  4:2:0 → 4:2:2: Upsample vertically"
    echo "  4:2:0 → 4:4:4: Upsample both horizontally and vertically"
}

# Main execution
echo -e "${GREEN}Starting debug session...${NC}"

# Handle camera mode
if test_camera_scenario; then
    exit 0
fi

# Run main debug
if main_debug_pipeline; then
    create_comparison
    run_performance_test
    
    echo -e "\n${GREEN}=== Debug Session Complete ===${NC}"
    echo "All files saved to: $OUTPUT_DIR"
    
    # Show file listing
    echo -e "\n${BLUE}Generated files:${NC}"
    ls -la "$OUTPUT_DIR"
else
    echo -e "\n${RED}Debug session failed${NC}"
    exit 1
fi

show_supported_formats

echo -e "\n${BLUE}Usage examples:${NC}"
echo "  ./debug_yuv_pipeline.sh Y42B I420 1280 720"
echo "  ./debug_yuv_pipeline.sh camera I420 1280 720"  
echo "  ./debug_yuv_pipeline.sh Y444 Y42B 640 480"
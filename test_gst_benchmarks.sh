#!/bin/bash

# Test script for GStreamer benchmarks
# This validates that our FPS calculation improvements work correctly

set -e

echo "Testing GStreamer Encoder and Decoder Benchmarks"
echo "=================================================="

# Set up environment
export GST_PLUGIN_PATH=$PWD/builddir:$GST_PLUGIN_PATH
export GST_DEBUG=1

echo ""
echo "1. Testing GStreamer Encoder Benchmark"
echo "======================================="

echo "Running quick encoder test..."
builddir/benchmarks/gst_encoder_benchmark \
    --benchmark_min_time=0.5 \
    --benchmark_display_aggregates_only=false

echo ""
echo "2. Testing GStreamer Decoder Benchmark"  
echo "======================================="

echo "Running quick decoder test..."
builddir/benchmarks/gst_decoder_benchmark \
    --benchmark_min_time=0.5 \
    --benchmark_display_aggregates_only=false

echo ""
echo "3. Comparing with libturbojpeg direct benchmarks"
echo "================================================"

echo "Running equivalent libturbojpeg encoder test..."
builddir/benchmarks/encoder_benchmark \
    --benchmark_min_time=0.5

echo ""
echo "Running equivalent libturbojpeg decoder test..."
builddir/benchmarks/decoder_benchmark \
    --benchmark_min_time=0.5

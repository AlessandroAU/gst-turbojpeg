#!/bin/bash

# Build and install script for GStreamer TurboJPEG decoder plugin

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Configuration
BUILD_DIR="builddir"
PREFIX="/usr"
INSTALL_DIR="${PREFIX}/lib/$(uname -m)-linux-gnu/gstreamer-1.0"

echo -e "${BLUE}GStreamer TurboJPEG Decoder Plugin Build Script${NC}"
echo "=============================================="
echo ""

# Function to print status messages
print_status() {
    local status="$1"
    local message="$2"
    
    case $status in
        "info")
            echo -e "${BLUE}[INFO]${NC} $message"
            ;;
        "success")
            echo -e "${GREEN}[SUCCESS]${NC} $message"
            ;;
        "warning")
            echo -e "${YELLOW}[WARNING]${NC} $message"
            ;;
        "error")
            echo -e "${RED}[ERROR]${NC} $message"
            ;;
    esac
}

# Function to check if running as root for installation
check_install_permissions() {
    if [[ $EUID -ne 0 && "$PREFIX" == "/usr" ]]; then
        print_status "warning" "Installation to /usr requires root privileges"
        echo "Options:"
        echo "1. Run this script with sudo: sudo ./build.sh"
        echo "2. Install to user directory: ./build.sh --prefix=\$HOME/.local"
        echo "3. Continue with build only (no install)"
        echo ""
        read -p "Continue with build only? (y/N): " continue_build
        if [[ ! "$continue_build" =~ ^[Yy]$ ]]; then
            exit 1
        fi
        SKIP_INSTALL=1
    fi
}

# Function to check dependencies
check_dependencies() {
    print_status "info" "Checking build dependencies..."
    
    local missing_deps=()
    
    # Check for required tools
    command -v meson >/dev/null 2>&1 || missing_deps+=("meson")
    command -v ninja >/dev/null 2>&1 || missing_deps+=("ninja-build")
    command -v pkg-config >/dev/null 2>&1 || missing_deps+=("pkg-config")
    command -v gcc >/dev/null 2>&1 || missing_deps+=("gcc")
    
    # Check for required libraries
    pkg-config --exists gstreamer-1.0 || missing_deps+=("libgstreamer1.0-dev")
    pkg-config --exists gstreamer-base-1.0 || missing_deps+=("libgstreamer-plugins-base1.0-dev")
    pkg-config --exists gstreamer-video-1.0 || missing_deps+=("libgstreamer-plugins-base1.0-dev")
    pkg-config --exists libturbojpeg || missing_deps+=("libturbojpeg0-dev")
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        print_status "error" "Missing dependencies:"
        for dep in "${missing_deps[@]}"; do
            echo "  - $dep"
        done
        echo ""
        echo "On Ubuntu/Debian, install with:"
        echo "  sudo apt update"
        echo "  sudo apt install meson ninja-build pkg-config gcc"
        echo "  sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev"
        echo "  sudo apt install libturbojpeg0-dev"
        echo ""
        exit 1
    fi
    
    print_status "success" "All dependencies found"
}

# Function to clean build directory
clean_build() {
    if [[ -d "$BUILD_DIR" ]]; then
        print_status "info" "Cleaning existing build directory..."
        rm -rf "$BUILD_DIR"
    fi
}

# Function to setup build
setup_build() {
    print_status "info" "Setting up build with prefix: $PREFIX"
    
    if ! meson setup "$BUILD_DIR" --prefix="$PREFIX" --libdir=lib/$(uname -m)-linux-gnu; then
        print_status "error" "Failed to setup build"
        exit 1
    fi
    
    print_status "success" "Build setup completed"
}

# Function to compile
compile() {
    print_status "info" "Compiling plugin..."
    
    if ! meson compile -C "$BUILD_DIR"; then
        print_status "error" "Compilation failed"
        exit 1
    fi
    
    print_status "success" "Compilation completed"
}

# Function to run tests
run_tests() {
    print_status "info" "Running basic plugin test..."
    
    # Test if plugin loads
    export GST_PLUGIN_PATH="$PWD/$BUILD_DIR:$GST_PLUGIN_PATH"
    
    if gst-inspect-1.0 turbojpegdec >/dev/null 2>&1; then
        print_status "success" "Plugin loads successfully"
    else
        print_status "error" "Plugin failed to load"
        return 1
    fi
}

# Function to install
install_plugin() {
    if [[ "$SKIP_INSTALL" == "1" ]]; then
        print_status "warning" "Skipping installation (insufficient permissions)"
        return 0
    fi
    
    print_status "info" "Installing plugin to $INSTALL_DIR"
    
    if ! meson install -C "$BUILD_DIR"; then
        print_status "error" "Installation failed"
        exit 1
    fi
    
    print_status "success" "Installation completed"
    
    # Update GStreamer plugin cache
    print_status "info" "Updating GStreamer plugin cache..."
    if command -v gst-inspect-1.0 >/dev/null 2>&1; then
        gst-inspect-1.0 >/dev/null 2>&1 || true
    fi
}

# Function to show post-install information
show_post_install() {
    echo ""
    echo -e "${GREEN}Build completed successfully!${NC}"
    echo ""
    
    if [[ "$SKIP_INSTALL" != "1" ]]; then
        echo "Plugin installed to: $INSTALL_DIR"
        echo ""
        echo "The plugin should now be available system-wide."
        echo "Test with: gst-inspect-1.0 turbojpegdec"
    else
        echo "Plugin built but not installed."
        echo "To test locally, run:"
        echo "  export GST_PLUGIN_PATH=$PWD/$BUILD_DIR:\$GST_PLUGIN_PATH"
        echo "  gst-inspect-1.0 turbojpegdec"
        echo ""
        echo "To install manually:"
        echo "  sudo meson install -C $BUILD_DIR"
    fi

    echo "Example usage:"
    echo "  gst-launch-1.0 videotestsrc ! video/x-raw,width=1920,height=1080 ! imagefreeze ! turbojpegenc ! queue ! turbojpegdec ! perf ! fakesink sync=false"
}

# Parse command line arguments
CLEAN=0
SKIP_INSTALL=0

while [[ $# -gt 0 ]]; do
    case $1 in
        --prefix=*)
            PREFIX="${1#*=}"
            INSTALL_DIR="${PREFIX}/lib/$(uname -m)-linux-gnu/gstreamer-1.0"
            shift
            ;;
        --clean)
            CLEAN=1
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --prefix=PATH    Installation prefix (default: /usr)"
            echo "  --clean          Clean build directory before building"
            echo "  --help, -h       Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                           # Build and install to /usr (requires sudo)"
            echo "  $0 --prefix=\$HOME/.local     # Install to user directory"
            echo "  sudo $0                      # Install system-wide with sudo"
            echo "  $0 --clean                   # Clean build and rebuild"
            exit 0
            ;;
        *)
            print_status "error" "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Main build process
echo "Build configuration:"
echo "  Source directory: $PWD"
echo "  Build directory:  $BUILD_DIR"
echo "  Install prefix:   $PREFIX"
echo "  Plugin directory: $INSTALL_DIR"
echo ""

# Check permissions for installation
check_install_permissions

# Execute build steps
check_dependencies

if [[ "$CLEAN" == "1" ]]; then
    clean_build
fi

setup_build
compile
run_tests
install_plugin
show_post_install

print_status "success" "All done!"
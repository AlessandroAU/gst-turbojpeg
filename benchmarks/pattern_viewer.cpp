#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <turbojpeg.h>
#include "pattern_generator.h"

// JPEG writer using TurboJPEG for RGB images
void writeJPEG(const std::string& filename, const std::vector<uint8_t>& rgb_data, int width, int height, int quality = 85) {
    tjhandle tjInstance = tjInitCompress();
    if (!tjInstance) {
        std::cerr << "Error: Failed to initialize TurboJPEG compressor" << std::endl;
        return;
    }
    
    unsigned char* jpeg_buffer = nullptr;
    unsigned long jpeg_size = 0;
    
    int result = tjCompress2(tjInstance, rgb_data.data(), width, 0, height,
                            TJPF_RGB, &jpeg_buffer, &jpeg_size, TJSAMP_420,
                            quality, TJFLAG_FASTDCT);
    
    if (result != 0) {
        std::cerr << "Error: TurboJPEG compression failed: " << tjGetErrorStr() << std::endl;
        tjDestroy(tjInstance);
        return;
    }
    
    // Write JPEG data to file
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot create file " << filename << std::endl;
        tjFree(jpeg_buffer);
        tjDestroy(tjInstance);
        return;
    }
    
    file.write(reinterpret_cast<const char*>(jpeg_buffer), jpeg_size);
    
    std::cout << "Saved: " << filename << " (" << width << "x" << height << ", JPEG Q" << quality << ", " << jpeg_size << " bytes)" << std::endl;
    
    tjFree(jpeg_buffer);
    tjDestroy(tjInstance);
}

// JPEG writer for grayscale images
void writeGrayscaleJPEG(const std::string& filename, const std::vector<uint8_t>& gray_data, int width, int height, int quality = 85) {
    tjhandle tjInstance = tjInitCompress();
    if (!tjInstance) {
        std::cerr << "Error: Failed to initialize TurboJPEG compressor" << std::endl;
        return;
    }
    
    unsigned char* jpeg_buffer = nullptr;
    unsigned long jpeg_size = 0;
    
    int result = tjCompress2(tjInstance, gray_data.data(), width, 0, height,
                            TJPF_GRAY, &jpeg_buffer, &jpeg_size, TJSAMP_GRAY,
                            quality, TJFLAG_FASTDCT);
    
    if (result != 0) {
        std::cerr << "Error: TurboJPEG grayscale compression failed: " << tjGetErrorStr() << std::endl;
        tjDestroy(tjInstance);
        return;
    }
    
    // Write JPEG data to file
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot create file " << filename << std::endl;
        tjFree(jpeg_buffer);
        tjDestroy(tjInstance);
        return;
    }
    
    file.write(reinterpret_cast<const char*>(jpeg_buffer), jpeg_size);
    
    std::cout << "Saved: " << filename << " (" << width << "x" << height << ", Grayscale JPEG Q" << quality << ", " << jpeg_size << " bytes)" << std::endl;
    
    tjFree(jpeg_buffer);
    tjDestroy(tjInstance);
}

void generatePatternSet(const std::string& base_name, int width, int height, PatternGenerator::PatternType pattern, int quality = 85) {
    std::string pattern_name;
    switch (pattern) {
        case PatternGenerator::PatternType::GRADIENT:
            pattern_name = "gradient";
            break;
        case PatternGenerator::PatternType::CHECKERBOARD:
            pattern_name = "checkerboard";
            break;
        case PatternGenerator::PatternType::SINE_WAVE:
            pattern_name = "sine_wave";
            break;
        case PatternGenerator::PatternType::MIXED_FREQUENCY:
            pattern_name = "mixed_frequency";
            break;
        case PatternGenerator::PatternType::PHOTO_REALISTIC:
            pattern_name = "photo_realistic";
            break;
        case PatternGenerator::PatternType::SMPTE_COLOR_BARS:
            pattern_name = "smpte_color_bars";
            break;
    }
    
    std::cout << "\nGenerating " << pattern_name << " patterns (" << width << "x" << height << ")..." << std::endl;
    
    // Generate RGB pattern and save as JPEG
    auto rgb_data = PatternGenerator::generateRGB(width, height, pattern);
    std::string rgb_filename = base_name + "_" + pattern_name + "_rgb.jpg";
    writeJPEG(rgb_filename, rgb_data, width, height, quality);
    
    // Generate grayscale pattern and save as JPEG
    auto gray_data = PatternGenerator::generateGrayscale(width, height, pattern);
    std::string gray_filename = base_name + "_" + pattern_name + "_gray.jpg";
    writeGrayscaleJPEG(gray_filename, gray_data, width, height, quality);
    
    // Generate RGBA pattern and save as RGB JPEG (ignoring alpha for simplicity)
    auto rgba_data = PatternGenerator::generateRGBA(width, height, pattern);
    std::vector<uint8_t> rgba_as_rgb;
    rgba_as_rgb.reserve(width * height * 3);
    
    for (size_t i = 0; i < rgba_data.size(); i += 4) {
        rgba_as_rgb.push_back(rgba_data[i]);     // R
        rgba_as_rgb.push_back(rgba_data[i + 1]); // G
        rgba_as_rgb.push_back(rgba_data[i + 2]); // B
        // Skip alpha channel
    }
    
    std::string rgba_filename = base_name + "_" + pattern_name + "_rgba_as_rgb.jpg";
    writeJPEG(rgba_filename, rgba_as_rgb, width, height, quality);
}

void printUsage(const char* program_name) {
    std::cout << "Pattern Generator Viewer - Saves generated patterns to JPEG files using TurboJPEG\n\n";
    std::cout << "Usage: " << program_name << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -w, --width WIDTH       Image width (default: 512)\n";
    std::cout << "  -h, --height HEIGHT     Image height (default: 512)\n";
    std::cout << "  -q, --quality QUALITY   JPEG quality 1-100 (default: 85)\n";
    std::cout << "  -o, --output PREFIX     Output filename prefix (default: 'pattern')\n";
    std::cout << "  -p, --pattern TYPE      Generate specific pattern only:\n";
    std::cout << "                          gradient, checkerboard, sine_wave,\n";
    std::cout << "                          mixed_frequency, photo_realistic,\n";
    std::cout << "                          smpte_color_bars\n";
    std::cout << "                          (default: generate all patterns)\n";
    std::cout << "  --help                  Show this help message\n\n";
    std::cout << "Output formats:\n";
    std::cout << "  - RGB patterns saved as .jpg files using TurboJPEG\n";
    std::cout << "  - Grayscale patterns saved as .jpg files using TurboJPEG\n";
    std::cout << "  - RGBA patterns saved as .jpg files (alpha channel ignored)\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << "                           # Generate all patterns, 512x512, Q85\n";
    std::cout << "  " << program_name << " -w 1920 -h 1080 -q 90    # Generate all patterns, 1920x1080, Q90\n";
    std::cout << "  " << program_name << " -p mixed_frequency        # Generate only mixed_frequency pattern\n";
    std::cout << "  " << program_name << " -o test -w 256 -h 256 -q 70  # Custom prefix, size, and quality\n";
}

PatternGenerator::PatternType parsePatternType(const std::string& pattern_str) {
    if (pattern_str == "gradient") return PatternGenerator::PatternType::GRADIENT;
    if (pattern_str == "checkerboard") return PatternGenerator::PatternType::CHECKERBOARD;
    if (pattern_str == "sine_wave") return PatternGenerator::PatternType::SINE_WAVE;
    if (pattern_str == "mixed_frequency") return PatternGenerator::PatternType::MIXED_FREQUENCY;
    if (pattern_str == "photo_realistic") return PatternGenerator::PatternType::PHOTO_REALISTIC;
    if (pattern_str == "smpte_color_bars") return PatternGenerator::PatternType::SMPTE_COLOR_BARS;
    
    std::cerr << "Error: Unknown pattern type '" << pattern_str << "'" << std::endl;
    std::cerr << "Valid types: gradient, checkerboard, sine_wave, mixed_frequency, photo_realistic, smpte_color_bars" << std::endl;
    exit(1);
}

int main(int argc, char* argv[]) {
    int width = 512;
    int height = 512;
    int quality = 85;
    std::string output_prefix = "pattern";
    std::string specific_pattern = "";
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if ((arg == "-w" || arg == "--width") && i + 1 < argc) {
            width = std::atoi(argv[++i]);
            if (width <= 0) {
                std::cerr << "Error: Width must be positive" << std::endl;
                return 1;
            }
        } else if ((arg == "-h" || arg == "--height") && i + 1 < argc) {
            height = std::atoi(argv[++i]);
            if (height <= 0) {
                std::cerr << "Error: Height must be positive" << std::endl;
                return 1;
            }
        } else if ((arg == "-q" || arg == "--quality") && i + 1 < argc) {
            quality = std::atoi(argv[++i]);
            if (quality < 1 || quality > 100) {
                std::cerr << "Error: Quality must be between 1 and 100" << std::endl;
                return 1;
            }
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            output_prefix = argv[++i];
        } else if ((arg == "-p" || arg == "--pattern") && i + 1 < argc) {
            specific_pattern = argv[++i];
        } else {
            std::cerr << "Error: Unknown argument '" << arg << "'" << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    std::cout << "Pattern Generator Viewer" << std::endl;
    std::cout << "========================" << std::endl;
    
    if (specific_pattern.empty()) {
        // Generate all pattern types
        std::cout << "Generating all pattern types..." << std::endl;
        
        generatePatternSet(output_prefix, width, height, PatternGenerator::PatternType::GRADIENT, quality);
        generatePatternSet(output_prefix, width, height, PatternGenerator::PatternType::CHECKERBOARD, quality);
        generatePatternSet(output_prefix, width, height, PatternGenerator::PatternType::SINE_WAVE, quality);
        generatePatternSet(output_prefix, width, height, PatternGenerator::PatternType::MIXED_FREQUENCY, quality);
        generatePatternSet(output_prefix, width, height, PatternGenerator::PatternType::PHOTO_REALISTIC, quality);
        generatePatternSet(output_prefix, width, height, PatternGenerator::PatternType::SMPTE_COLOR_BARS, quality);
        
        std::cout << "\nGenerated " << 6 * 3 << " JPEG files total." << std::endl;
    } else {
        // Generate specific pattern type
        PatternGenerator::PatternType pattern_type = parsePatternType(specific_pattern);
        generatePatternSet(output_prefix, width, height, pattern_type, quality);
        
        std::cout << "\nGenerated 3 JPEG files." << std::endl;
    }
    
    std::cout << "\nTo view the generated JPEG images:" << std::endl;
    std::cout << "  - Use any image viewer that supports JPEG format" << std::endl;
    std::cout << "  - View with: display file.jpg (ImageMagick)" << std::endl;
    std::cout << "  - View with: feh file.jpg" << std::endl;
    std::cout << "  - View with: eog file.jpg (GNOME)" << std::endl;
    
    return 0;
}
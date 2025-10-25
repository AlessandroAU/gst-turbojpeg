#include "pattern_generator.h"
#include <algorithm>

// Helper function for clamping values (C++11 compatible)
template<typename T>
T clamp(T value, T min_val, T max_val) {
    return std::max(min_val, std::min(value, max_val));
}

std::vector<uint8_t> PatternGenerator::generateRGB(int width, int height, PatternType pattern) {
    std::vector<uint8_t> data(width * height * 3);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 3;
            
            switch (pattern) {
                case PatternType::GRADIENT:
                    data[idx] = gradientPattern(x, y, width, height, 0);     // R
                    data[idx + 1] = gradientPattern(x, y, width, height, 1); // G
                    data[idx + 2] = gradientPattern(x, y, width, height, 2); // B
                    break;
                case PatternType::CHECKERBOARD:
                    data[idx] = checkerboardPattern(x, y, 0);     // R
                    data[idx + 1] = checkerboardPattern(x, y, 1); // G
                    data[idx + 2] = checkerboardPattern(x, y, 2); // B
                    break;
                case PatternType::SINE_WAVE:
                    data[idx] = sineWavePattern(x, y, width, height, 0);     // R
                    data[idx + 1] = sineWavePattern(x, y, width, height, 1); // G
                    data[idx + 2] = sineWavePattern(x, y, width, height, 2); // B
                    break;
                case PatternType::MIXED_FREQUENCY:
                    data[idx] = mixedFrequencyPattern(x, y, width, height, 0);     // R
                    data[idx + 1] = mixedFrequencyPattern(x, y, width, height, 1); // G
                    data[idx + 2] = mixedFrequencyPattern(x, y, width, height, 2); // B
                    break;
                case PatternType::PHOTO_REALISTIC:
                    data[idx] = photoRealisticPattern(x, y, width, height, 0);     // R
                    data[idx + 1] = photoRealisticPattern(x, y, width, height, 1); // G
                    data[idx + 2] = photoRealisticPattern(x, y, width, height, 2); // B
                    break;
                case PatternType::SMPTE_COLOR_BARS:
                    data[idx] = smpteColorBarsPattern(x, y, width, height, 0);     // R
                    data[idx + 1] = smpteColorBarsPattern(x, y, width, height, 1); // G
                    data[idx + 2] = smpteColorBarsPattern(x, y, width, height, 2); // B
                    break;
            }
        }
    }
    
    return data;
}

std::vector<uint8_t> PatternGenerator::generateGrayscale(int width, int height, PatternType pattern) {
    std::vector<uint8_t> data(width * height);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            
            switch (pattern) {
                case PatternType::GRADIENT:
                    data[idx] = gradientPattern(x, y, width, height, 0);
                    break;
                case PatternType::CHECKERBOARD:
                    data[idx] = checkerboardPattern(x, y, 0);
                    break;
                case PatternType::SINE_WAVE:
                    data[idx] = sineWavePattern(x, y, width, height, 0);
                    break;
                case PatternType::MIXED_FREQUENCY:
                    data[idx] = mixedFrequencyPattern(x, y, width, height, 0);
                    break;
                case PatternType::PHOTO_REALISTIC:
                    data[idx] = photoRealisticPattern(x, y, width, height, 0);
                    break;
                case PatternType::SMPTE_COLOR_BARS:
                    data[idx] = smpteColorBarsPattern(x, y, width, height, 0);
                    break;
            }
        }
    }
    
    return data;
}

std::vector<uint8_t> PatternGenerator::generateRGBA(int width, int height, PatternType pattern) {
    std::vector<uint8_t> data(width * height * 4);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 4;
            
            switch (pattern) {
                case PatternType::GRADIENT:
                    data[idx] = gradientPattern(x, y, width, height, 0);     // R
                    data[idx + 1] = gradientPattern(x, y, width, height, 1); // G
                    data[idx + 2] = gradientPattern(x, y, width, height, 2); // B
                    data[idx + 3] = 255; // A (full opacity)
                    break;
                case PatternType::CHECKERBOARD:
                    data[idx] = checkerboardPattern(x, y, 0);     // R
                    data[idx + 1] = checkerboardPattern(x, y, 1); // G
                    data[idx + 2] = checkerboardPattern(x, y, 2); // B
                    data[idx + 3] = 255; // A
                    break;
                case PatternType::SINE_WAVE:
                    data[idx] = sineWavePattern(x, y, width, height, 0);     // R
                    data[idx + 1] = sineWavePattern(x, y, width, height, 1); // G
                    data[idx + 2] = sineWavePattern(x, y, width, height, 2); // B
                    data[idx + 3] = 255; // A
                    break;
                case PatternType::MIXED_FREQUENCY:
                    data[idx] = mixedFrequencyPattern(x, y, width, height, 0);     // R
                    data[idx + 1] = mixedFrequencyPattern(x, y, width, height, 1); // G
                    data[idx + 2] = mixedFrequencyPattern(x, y, width, height, 2); // B
                    data[idx + 3] = 255; // A
                    break;
                case PatternType::PHOTO_REALISTIC:
                    data[idx] = photoRealisticPattern(x, y, width, height, 0);     // R
                    data[idx + 1] = photoRealisticPattern(x, y, width, height, 1); // G
                    data[idx + 2] = photoRealisticPattern(x, y, width, height, 2); // B
                    data[idx + 3] = 255; // A
                    break;
                case PatternType::SMPTE_COLOR_BARS:
                    data[idx] = smpteColorBarsPattern(x, y, width, height, 0);     // R
                    data[idx + 1] = smpteColorBarsPattern(x, y, width, height, 1); // G
                    data[idx + 2] = smpteColorBarsPattern(x, y, width, height, 2); // B
                    data[idx + 3] = 255; // A
                    break;
            }
        }
    }
    
    return data;
}

uint8_t PatternGenerator::gradientPattern(int x, int y, int width, int height, int channel) {
    switch (channel) {
        case 0: // R - horizontal gradient
            return static_cast<uint8_t>((x * 255) / width);
        case 1: // G - vertical gradient
            return static_cast<uint8_t>((y * 255) / height);
        case 2: // B - diagonal gradient
            return static_cast<uint8_t>(((x + y) * 255) / (width + height));
        default:
            return 128;
    }
}

uint8_t PatternGenerator::checkerboardPattern(int x, int y, int channel) {
    const int checkerSize = 32;
    bool isWhite = ((x / checkerSize) + (y / checkerSize)) % 2 == 0;
    
    switch (channel) {
        case 0: // R
            return isWhite ? 255 : 64;
        case 1: // G
            return isWhite ? 255 : 128;
        case 2: // B
            return isWhite ? 255 : 192;
        default:
            return isWhite ? 255 : 128;
    }
}

uint8_t PatternGenerator::sineWavePattern(int x, int y, int width, int height, int channel) {
    double freq_x = 2.0 * M_PI * 8.0 / width;
    double freq_y = 2.0 * M_PI * 6.0 / height;
    
    switch (channel) {
        case 0: // R - horizontal sine wave
            return static_cast<uint8_t>(128 + 127 * std::sin(freq_x * x));
        case 1: // G - vertical sine wave
            return static_cast<uint8_t>(128 + 127 * std::sin(freq_y * y));
        case 2: // B - diagonal sine wave
            return static_cast<uint8_t>(128 + 127 * std::sin(freq_x * x + freq_y * y));
        default:
            return 128;
    }
}

uint8_t PatternGenerator::mixedFrequencyPattern(int x, int y, int width, int height, int channel) {
    // Combine multiple frequencies to create a more complex, realistic pattern
    double norm_x = static_cast<double>(x) / width;
    double norm_y = static_cast<double>(y) / height;
    
    // Base pattern with multiple harmonics
    double pattern = 0.0;
    
    // Low frequency component (gradual changes)
    pattern += 0.4 * std::sin(2.0 * M_PI * norm_x * 2.0) * std::cos(2.0 * M_PI * norm_y * 1.5);
    
    // Medium frequency component (texture details)
    pattern += 0.3 * std::sin(2.0 * M_PI * norm_x * 16.0) * std::sin(2.0 * M_PI * norm_y * 12.0);
    
    // High frequency component (fine details)
    pattern += 0.2 * std::sin(2.0 * M_PI * norm_x * 64.0) * std::cos(2.0 * M_PI * norm_y * 48.0);
    
    // Add some channel-specific variations
    switch (channel) {
        case 0: // R - emphasize horizontal patterns
            pattern += 0.1 * std::sin(2.0 * M_PI * norm_x * 32.0);
            break;
        case 1: // G - emphasize vertical patterns  
            pattern += 0.1 * std::sin(2.0 * M_PI * norm_y * 32.0);
            break;
        case 2: // B - emphasize diagonal patterns
            pattern += 0.1 * std::sin(2.0 * M_PI * (norm_x + norm_y) * 32.0);
            break;
    }
    
    // Normalize to 0-255 range
    return static_cast<uint8_t>(clamp(128 + pattern * 127, 0.0, 255.0));
}

uint8_t PatternGenerator::photoRealisticPattern(int x, int y, int width, int height, int channel) {
    // Create a pattern that mimics natural image characteristics
    double norm_x = static_cast<double>(x) / width;
    double norm_y = static_cast<double>(y) / height;
    
    // Base luminance with smooth gradients
    double base = 0.6 + 0.2 * std::sin(2.0 * M_PI * norm_x * 0.5) 
                     + 0.1 * std::cos(2.0 * M_PI * norm_y * 0.3);
    
    // Add texture with multiple scales (similar to natural images)
    double texture = 0.0;
    texture += 0.15 * std::sin(2.0 * M_PI * norm_x * 8.0) * std::cos(2.0 * M_PI * norm_y * 6.0);
    texture += 0.10 * std::sin(2.0 * M_PI * norm_x * 24.0) * std::sin(2.0 * M_PI * norm_y * 18.0);
    texture += 0.05 * std::sin(2.0 * M_PI * norm_x * 72.0) * std::cos(2.0 * M_PI * norm_y * 54.0);
    
    // Add some noise-like variations
    double noise = 0.03 * std::sin(2.0 * M_PI * norm_x * 200.0 + norm_y * 150.0);
    
    double final_value = base + texture + noise;
    
    // Channel-specific adjustments to simulate color balance
    switch (channel) {
        case 0: // R - slightly warmer
            final_value *= 1.05;
            break;
        case 1: // G - neutral
            final_value *= 1.0;
            break;
        case 2: // B - slightly cooler
            final_value *= 0.95;
            break;
    }
    
    return static_cast<uint8_t>(clamp(final_value * 255, 0.0, 255.0));
}

uint8_t PatternGenerator::smpteColorBarsPattern(int x, int y, int width, int height, int channel) {
    // SMPTE color bars test pattern - industry standard video test pattern
    // Based on SMPTE RP 219-2002 and EBU Tech 3213
    
    // Standard SMPTE color bar values in RGB (0-255)
    // Top section (75% color bars): White, Yellow, Cyan, Green, Magenta, Red, Blue, Black
    static const uint8_t top_colors[8][3] = {
        {191, 191, 191},  // 75% White
        {191, 191,   0},  // 75% Yellow
        {  0, 191, 191},  // 75% Cyan
        {  0, 191,   0},  // 75% Green
        {191,   0, 191},  // 75% Magenta
        {191,   0,   0},  // 75% Red
        {  0,   0, 191},  // 75% Blue
        {  0,   0,   0}   // Black
    };
    
    // Middle section (reverse blue bars): Blue, Black, Magenta, Black, Cyan, Black, White
    static const uint8_t middle_colors[7][3] = {
        {  0,   0, 191},  // Blue
        {  0,   0,   0},  // Black
        {191,   0, 191},  // Magenta
        {  0,   0,   0},  // Black
        {  0, 191, 191},  // Cyan
        {  0,   0,   0},  // Black
        {191, 191, 191}   // White
    };
    
    // Bottom section: Black, White, Black, -2%, +2%, Black
    static const uint8_t bottom_colors[6][3] = {
        {  0,   0,   0},  // Black
        {255, 255, 255},  // 100% White
        {  0,   0,   0},  // Black
        { 13,  13,  13},  // -2% (super black)
        { 38,  38,  38},  // +2% (sub white)
        {  0,   0,   0}   // Black
    };
    
    // Calculate vertical sections
    int top_height = height * 2 / 3;          // Top 2/3
    int middle_height = height * 1 / 12;      // 1/12 height
    // int bottom_height = height - top_height - middle_height;  // Remaining (unused)
    
    if (y < top_height) {
        // Top section - 8 color bars
        int bar_width = width / 8;
        int bar_index = std::min(x / bar_width, 7);
        return top_colors[bar_index][channel];
        
    } else if (y < top_height + middle_height) {
        // Middle section - 7 reverse blue bars
        int bar_width = width / 7;
        int bar_index = std::min(x / bar_width, 6);
        return middle_colors[bar_index][channel];
        
    } else {
        // Bottom section - 6 bars with special levels
        int bar_width = width / 6;
        int bar_index = std::min(x / bar_width, 5);
        return bottom_colors[bar_index][channel];
    }
}
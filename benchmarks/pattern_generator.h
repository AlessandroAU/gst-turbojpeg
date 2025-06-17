#pragma once

#include <vector>
#include <cstdint>
#include <cmath>

class PatternGenerator {
public:
    enum class PatternType {
        GRADIENT,
        CHECKERBOARD,
        SINE_WAVE,
        MIXED_FREQUENCY,
        PHOTO_REALISTIC,
        SMPTE_COLOR_BARS
    };

    // Generate RGB data with specified pattern
    static std::vector<uint8_t> generateRGB(int width, int height, PatternType pattern = PatternType::MIXED_FREQUENCY);
    
    // Generate grayscale data with specified pattern
    static std::vector<uint8_t> generateGrayscale(int width, int height, PatternType pattern = PatternType::MIXED_FREQUENCY);
    
    // Generate RGBA data with specified pattern
    static std::vector<uint8_t> generateRGBA(int width, int height, PatternType pattern = PatternType::MIXED_FREQUENCY);

private:
    static uint8_t gradientPattern(int x, int y, int width, int height, int channel);
    static uint8_t checkerboardPattern(int x, int y, int channel);
    static uint8_t sineWavePattern(int x, int y, int width, int height, int channel);  
    static uint8_t mixedFrequencyPattern(int x, int y, int width, int height, int channel);
    static uint8_t photoRealisticPattern(int x, int y, int width, int height, int channel);
    static uint8_t smpteColorBarsPattern(int x, int y, int width, int height, int channel);
};
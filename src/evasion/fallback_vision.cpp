#include "evasion/fallback_vision.hpp"
#include <cstring>
#include <algorithm>

namespace turbopalmtree {

FallbackVisionModel::FallbackVisionModel() {}

FallbackVisionModel::~FallbackVisionModel() {}

std::optional<MeterReading> FallbackVisionModel::quick_detect(const FrameBuffer& frame) {
    return track_green_cluster(frame);
}

bool FallbackVisionModel::should_use_fallback(float confidence, uint32_t green_pixel_count) const {
    // Switch to fallback if:
    // - Primary detector lost tracking (confidence < threshold)
    // - Not enough green pixels found
    // - Emergency mode is active
    return emergency_mode_ || 
           confidence < fallback_confidence_threshold_ || 
           green_pixel_count < fallback_min_pixels_;
}

void FallbackVisionModel::set_emergency_mode(bool enabled) {
    emergency_mode_ = enabled;
}

std::optional<MeterReading> FallbackVisionModel::track_green_cluster(const FrameBuffer& frame) {
    // Ultra-fast green detection: minimal processing
    // Trades accuracy for speed (~2-3ms vs 8ms)
    
    uint32_t green_pixel_count = 0;
    uint64_t sum_x = 0, sum_y = 0;
    
    // Skip pixels for faster processing: check every 2x2 block
    const uint32_t STRIDE = 2;
    
    for (uint32_t y = 0; y < frame.height; y += STRIDE) {
        for (uint32_t x = 0; x < frame.width; x += STRIDE) {
            uint32_t pixel_idx = (y * frame.width + x) * 3;  // BGR format
            uint8_t b = frame.data[pixel_idx];
            uint8_t g = frame.data[pixel_idx + 1];
            uint8_t r = frame.data[pixel_idx + 2];
            
            // Aggressive green detection: just check if g > b and g > r
            if (g > b && g > r && g > 120) {
                green_pixel_count++;
                sum_x += x;
                sum_y += y;
            }
        }
    }
    
    // Check minimum pixel threshold
    if (green_pixel_count < fallback_min_pixels_) {
        return std::nullopt;
    }
    
    // Calculate center of mass
    uint32_t center_x = sum_x / green_pixel_count;
    uint32_t center_y = sum_y / green_pixel_count;
    
    // Calculate confidence based on pixel count
    float confidence = static_cast<float>(green_pixel_count * STRIDE * STRIDE) / 
                       (frame.width * frame.height);
    
    if (confidence < fallback_confidence_threshold_ && !emergency_mode_) {
        return std::nullopt;
    }
    
    return MeterReading{
        center_x,
        center_y,
        confidence,
        green_pixel_count
    };
}

} // namespace turbopalmtree

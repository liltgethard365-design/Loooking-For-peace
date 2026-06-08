#pragma once

#include "engine/types.hpp"
#include <optional>
#include <vector>

namespace turbopalmtree {

// Emergency fallback: if primary vision fails, use lightweight backup
// This ensures the system NEVER crashes - avoiding detection via crash patterns
class FallbackVisionModel {
public:
    FallbackVisionModel();
    ~FallbackVisionModel();
    
    // Simple fallback: just track green pixel cluster without fancy heuristics
    // Much faster (2-3ms vs 8ms for full detector)
    std::optional<MeterReading> quick_detect(const FrameBuffer& frame);
    
    // Check if primary detector should hand off to fallback
    bool should_use_fallback(float confidence, uint32_t green_pixel_count) const;
    
    // Adaptive thresholding: if we're in fallback mode, be more lenient
    void set_emergency_mode(bool enabled);
    
private:
    float fallback_confidence_threshold_ = 0.4f;  // More lenient than primary
    uint32_t fallback_min_pixels_ = 30;           // Much lower threshold
    bool emergency_mode_ = false;
    
    // Simple green tracking without complex heuristics
    std::optional<MeterReading> track_green_cluster(const FrameBuffer& frame);
};

} // namespace turbopalmtree

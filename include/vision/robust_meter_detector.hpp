#pragma once

#include "engine/types.hpp"
#include <vector>
#include <memory>
#include <optional>

namespace turbopalmtree {

class RobustMeterDetector {
public:
    RobustMeterDetector();
    ~RobustMeterDetector();
    
    std::optional<MeterReading> detect(const FrameBuffer& frame);
    void calibrate(const std::vector<FrameBuffer>& sample_frames);
    
    void set_confidence_threshold(float threshold) { confidence_threshold_ = threshold; }
    float get_confidence_threshold() const { return confidence_threshold_; }
    bool is_calibrated() const { return is_calibrated_; }
    
private:
    bool is_green_pixel(uint8_t b, uint8_t g, uint8_t r) const;
    std::optional<MeterReading> find_meter_center(const FrameBuffer& frame);
    
    float confidence_threshold_ = 0.7f;
    uint32_t min_green_pixels_ = 100;
    bool is_calibrated_ = false;
    
    struct ColorProfile {
        uint8_t mean_b, mean_g, mean_r;
        uint8_t stddev_b, stddev_g, stddev_r;
    } green_profile_;
};

} // namespace turbopalmtree

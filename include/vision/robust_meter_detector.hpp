#pragma once

#include "engine/types.hpp"
#include <vector>
#include <memory>
#include <optional>
#include <array>
#include <cstdint>

namespace turbopalmtree {

// NBA 2K26 specific green meter profile
class RobustMeterDetector {
public:
    RobustMeterDetector();
    ~RobustMeterDetector();
    
    // Detection: find meter in frame
    std::optional<MeterReading> detect(const FrameBuffer& frame);
    
    // Auto-calibration: learn actual green from sample frames
    void auto_calibrate(const std::vector<FrameBuffer>& sample_frames);
    
    // Manual calibration with specific RGB values
    void set_green_profile(uint8_t r, uint8_t g, uint8_t b, 
                          uint8_t r_tolerance, uint8_t g_tolerance, uint8_t b_tolerance);
    
    // Configuration
    void set_confidence_threshold(float threshold) { confidence_threshold_ = threshold; }
    float get_confidence_threshold() const { return confidence_threshold_; }
    
    void set_min_green_pixels(uint32_t min_pixels) { min_green_pixels_ = min_pixels; }
    uint32_t get_min_green_pixels() const { return min_green_pixels_; }
    
    bool is_calibrated() const { return is_calibrated_; }
    
    // Get current green profile for diagnostics
    struct GreenProfile {
        uint8_t mean_r, mean_g, mean_b;
        uint8_t tol_r, tol_g, tol_b;
    };
    GreenProfile get_green_profile() const;
    
private:
    // NBA 2K26 green detection with learned tolerances
    bool is_green_pixel(uint8_t b, uint8_t g, uint8_t r) const;
    
    // Optimized detection with early exit
    std::optional<MeterReading> find_meter_center(const FrameBuffer& frame);
    
    // Learn green color distribution from samples
    void learn_green_distribution(const std::vector<FrameBuffer>& samples);
    
    // Profile learning helper
    struct PixelStats {
        uint32_t r_sum, g_sum, b_sum;
        uint32_t r_min, g_min, b_min;
        uint32_t r_max, g_max, b_max;
        uint32_t count;
    };
    
    // Configuration
    float confidence_threshold_ = 0.6f;  // Lower for more sensitivity
    uint32_t min_green_pixels_ = 50;    // Minimum green pixels for valid detection
    bool is_calibrated_ = false;
    
    // NBA 2K26 green meter profile (with tolerances)
    struct ColorProfile {
        uint8_t mean_r = 20;
        uint8_t mean_g = 210;
        uint8_t mean_b = 30;
        
        // Tolerance bands for matching
        uint8_t tol_r = 40;
        uint8_t tol_g = 35;
        uint8_t tol_b = 40;
    } green_profile_;
    
    // Statistics for calibration accuracy
    struct CalibrationStats {
        uint32_t samples_processed = 0;
        uint32_t frames_with_green = 0;
        double avg_confidence = 0.0;
    } cal_stats_;
};

} // namespace turbopalmtree

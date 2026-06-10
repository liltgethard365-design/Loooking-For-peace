#pragma once

#include "engine/types.hpp"
#include <vector>
#include <optional>
#include <cstdint>
#include <deque>

namespace turbopalmtree {

// Meter state machine for tracking shot readiness
enum class MeterState {
    IDLE = 0,           // No meter visible
    ACTIVE = 1,         // Meter visible, moving
    STABLE = 2,         // Meter stopped, ready for input
    RELEASE_WINDOW = 3  // Green zone is optimal for release
};

// Motion profile for tracking meter movement
struct MotionFrame {
    uint16_t x, y;
    uint32_t green_count;
    uint64_t timestamp_us;
};

// Shot window timing prediction
struct ShotWindow {
    double window_start_ms;  // When green zone enters release window
    double window_end_ms;    // When green zone leaves release window
    double apex_ms;          // Estimated apex of meter motion
    bool is_valid;
};

class RobustMeterDetector {
public:
    RobustMeterDetector();
    ~RobustMeterDetector();
    
    // Detection: find meter in frame and update motion tracking
    std::optional<MeterReading> detect(const FrameBuffer& frame);
    
    // Manual calibration
    void set_green_profile(uint8_t r, uint8_t g, uint8_t b, 
                          uint8_t r_tol, uint8_t g_tol, uint8_t b_tol);
    
    // Auto-calibration from samples
    void auto_calibrate(const std::vector<FrameBuffer>& sample_frames);
    
    // Motion tracking API
    MeterState get_meter_state() const { return meter_state_; }
    double get_motion_velocity() const;  // Pixels per millisecond
    bool is_meter_moving() const;
    bool is_meter_stable() const;
    
    // Shot window prediction
    std::optional<ShotWindow> predict_shot_window() const;
    double get_time_to_green_ms() const;  // Time until next green zone
    
    // Configuration
    void set_confidence_threshold(float threshold) { confidence_threshold_ = threshold; }
    void set_min_green_pixels(uint32_t min_pixels) { min_green_pixels_ = min_pixels; }
    void set_motion_history_size(uint32_t size) { motion_history_size_ = size; }
    void set_motion_stability_threshold(double threshold) { motion_stability_threshold_ = threshold; }
    
    bool is_calibrated() const { return is_calibrated_; }
    
    // Diagnostics
    struct GreenProfile {
        uint8_t mean_r, mean_g, mean_b;
        uint8_t tol_r, tol_g, tol_b;
    };
    GreenProfile get_green_profile() const;
    
private:
    // Core detection
    bool is_green_pixel(uint8_t b, uint8_t g, uint8_t r) const;
    std::optional<MeterReading> find_meter_center(const FrameBuffer& frame);
    
    // Calibration
    void learn_green_distribution(const std::vector<FrameBuffer>& samples);
    
    // Motion analysis
    void update_motion_history(const MeterReading& reading);
    void update_meter_state();
    
    // Configuration
    float confidence_threshold_ = 0.6f;
    uint32_t min_green_pixels_ = 50;
    uint32_t motion_history_size_ = 10;  // How many frames to track
    double motion_stability_threshold_ = 5.0;  // Pixels/ms threshold for "stable"
    
    // Green meter profile
    struct ColorProfile {
        uint8_t mean_r = 20;
        uint8_t mean_g = 210;
        uint8_t mean_b = 30;
        uint8_t tol_r = 40;
        uint8_t tol_g = 35;
        uint8_t tol_b = 40;
    } green_profile_;
    
    // Calibration stats
    struct CalibrationStats {
        uint32_t samples_processed = 0;
        uint32_t frames_with_green = 0;
        double avg_confidence = 0.0;
    } cal_stats_;
    
    // Motion tracking state
    std::deque<MotionFrame> motion_history_;
    MeterState meter_state_ = MeterState::IDLE;
    bool is_calibrated_ = false;
    
    // Helper for motion computation
    struct PixelStats {
        uint32_t r_sum, g_sum, b_sum;
        uint32_t r_min, g_min, b_min;
        uint32_t r_max, g_max, b_max;
        uint32_t count;
    };
};

} // namespace turbopalmtree

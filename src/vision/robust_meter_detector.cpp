#include "vision/robust_meter_detector.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <iomanip>

namespace turbopalmtree {

RobustMeterDetector::RobustMeterDetector() {
    // NBA 2K26 default: bright pure green
    green_profile_ = {
        .mean_r = 20,
        .mean_g = 210,
        .mean_b = 30,
        .tol_r = 40,
        .tol_g = 35,
        .tol_b = 40
    };
    cal_stats_ = {};
}

RobustMeterDetector::~RobustMeterDetector() {}

std::optional<MeterReading> RobustMeterDetector::detect(const FrameBuffer& frame) {
    if (!frame.data || frame.size == 0) {
        return std::nullopt;
    }
    
    auto reading_opt = find_meter_center(frame);
    if (reading_opt) {
        update_motion_history(*reading_opt);
        update_meter_state();
    } else {
        // No meter detected
        meter_state_ = MeterState::IDLE;
    }
    
    return reading_opt;
}

void RobustMeterDetector::set_green_profile(uint8_t r, uint8_t g, uint8_t b,
                                           uint8_t r_tol, uint8_t g_tol, uint8_t b_tol) {
    green_profile_ = {
        .mean_r = r,
        .mean_g = g,
        .mean_b = b,
        .tol_r = r_tol,
        .tol_g = g_tol,
        .tol_b = b_tol
    };
    is_calibrated_ = true;
    
    std::cout << "[MeterDetector] Manual profile set:" << std::endl;
    std::cout << "  RGB: (" << (int)r << ", " << (int)g << ", " << (int)b << ")" << std::endl;
    std::cout << "  Tolerances: (±" << (int)r_tol << ", ±" << (int)g_tol << ", ±" << (int)b_tol << ")" << std::endl;
}

void RobustMeterDetector::auto_calibrate(const std::vector<FrameBuffer>& sample_frames) {
    if (sample_frames.empty()) {
        std::cerr << "[MeterDetector] Calibration failed: no sample frames provided" << std::endl;
        return;
    }
    
    std::cout << "[MeterDetector] Starting auto-calibration from " << sample_frames.size() 
              << " sample frames..." << std::endl;
    
    learn_green_distribution(sample_frames);
    is_calibrated_ = true;
    
    std::cout << "[MeterDetector] ✓ Calibration complete!" << std::endl;
    std::cout << "  Learned profile:" << std::endl;
    std::cout << "    RGB Mean: (" << (int)green_profile_.mean_r << ", " 
              << (int)green_profile_.mean_g << ", " << (int)green_profile_.mean_b << ")" << std::endl;
    std::cout << "    Tolerances: (±" << (int)green_profile_.tol_r << ", ±" 
              << (int)green_profile_.tol_g << ", ±" << (int)green_profile_.tol_b << ")" << std::endl;
    std::cout << "    Frames with green: " << cal_stats_.frames_with_green 
              << " / " << cal_stats_.samples_processed << std::endl;
    std::cout << "    Avg confidence: " << std::fixed << std::setprecision(3) 
              << cal_stats_.avg_confidence << std::endl;
}

RobustMeterDetector::GreenProfile RobustMeterDetector::get_green_profile() const {
    return {
        .mean_r = green_profile_.mean_r,
        .mean_g = green_profile_.mean_g,
        .mean_b = green_profile_.mean_b,
        .tol_r = green_profile_.tol_r,
        .tol_g = green_profile_.tol_g,
        .tol_b = green_profile_.tol_b
    };
}

bool RobustMeterDetector::is_green_pixel(uint8_t b, uint8_t g, uint8_t r) const {
    int r_diff = (int)r - (int)green_profile_.mean_r;
    int g_diff = (int)g - (int)green_profile_.mean_g;
    int b_diff = (int)b - (int)green_profile_.mean_b;
    
    if (std::abs(r_diff) > green_profile_.tol_r) return false;
    if (std::abs(g_diff) > green_profile_.tol_g) return false;
    if (std::abs(b_diff) > green_profile_.tol_b) return false;
    
    // Green must be primary and bright
    if (g <= r || g <= b) return false;
    if (g < 150) return false;
    
    return true;
}

std::optional<MeterReading> RobustMeterDetector::find_meter_center(const FrameBuffer& frame) {
    uint64_t center_x_sum = 0, center_y_sum = 0;
    uint32_t green_count = 0;
    uint16_t min_x = frame.width, max_x = 0;
    uint16_t min_y = frame.height, max_y = 0;
    
    const size_t frame_area = (size_t)frame.width * frame.height;
    
    for (uint32_t y = 0; y < frame.height; ++y) {
        for (uint32_t x = 0; x < frame.width; ++x) {
            size_t pixel_idx = (y * frame.width + x) * frame.bytes_per_pixel;
            
            if (pixel_idx + 2 >= frame.size) continue;
            
            uint8_t b = frame.data[pixel_idx];
            uint8_t g = frame.data[pixel_idx + 1];
            uint8_t r = frame.data[pixel_idx + 2];
            
            if (is_green_pixel(b, g, r)) {
                center_x_sum += x;
                center_y_sum += y;
                min_x = std::min(min_x, (uint16_t)x);
                max_x = std::max(max_x, (uint16_t)x);
                min_y = std::min(min_y, (uint16_t)y);
                max_y = std::max(max_y, (uint16_t)y);
                green_count++;
            }
        }
    }
    
    if (green_count < min_green_pixels_) {
        return std::nullopt;
    }
    
    uint16_t center_x = (uint16_t)(center_x_sum / green_count);
    uint16_t center_y = (uint16_t)(center_y_sum / green_count);
    float confidence = (float)green_count / frame_area;
    
    if (min_x >= max_x) max_x = min_x + 1;
    if (min_y >= max_y) max_y = min_y + 1;
    
    return MeterReading{
        .x_coordinate = center_x,
        .y_coordinate = center_y,
        .x_min = min_x,
        .x_max = max_x,
        .y_min = min_y,
        .y_max = max_y,
        .confidence = confidence,
        .timestamp = frame.metadata.capture_timestamp,
        .is_valid = (confidence >= confidence_threshold_),
        .green_pixel_count = green_count
    };
}

void RobustMeterDetector::learn_green_distribution(const std::vector<FrameBuffer>& samples) {
    PixelStats stats = {0, 0, 0, 255, 255, 255, 0, 0, 0, 0};
    
    std::cout << "  [Calibration] Analyzing " << samples.size() << " frames..." << std::endl;
    
    for (const auto& frame : samples) {
        uint32_t frame_green_count = 0;
        
        for (size_t idx = 0; idx + 2 < frame.size; idx += frame.bytes_per_pixel) {
            uint8_t b = frame.data[idx];
            uint8_t g = frame.data[idx + 1];
            uint8_t r = frame.data[idx + 2];
            
            if (g > r && g > b && g > 100) {
                stats.r_sum += r;
                stats.g_sum += g;
                stats.b_sum += b;
                
                stats.r_min = std::min(stats.r_min, (uint32_t)r);
                stats.g_min = std::min(stats.g_min, (uint32_t)g);
                stats.b_min = std::min(stats.b_min, (uint32_t)b);
                
                stats.r_max = std::max(stats.r_max, (uint32_t)r);
                stats.g_max = std::max(stats.g_max, (uint32_t)g);
                stats.b_max = std::max(stats.b_max, (uint32_t)b);
                
                stats.count++;
                frame_green_count++;
            }
        }
        
        if (frame_green_count > 0) {
            cal_stats_.frames_with_green++;
        }
        cal_stats_.samples_processed++;
    }
    
    if (stats.count == 0) {
        std::cerr << "[MeterDetector] No green pixels found in samples! Using default profile." << std::endl;
        return;
    }
    
    uint8_t mean_r = (uint8_t)(stats.r_sum / stats.count);
    uint8_t mean_g = (uint8_t)(stats.g_sum / stats.count);
    uint8_t mean_b = (uint8_t)(stats.b_sum / stats.count);
    
    uint8_t tol_r = std::max(25u, (uint8_t)((stats.r_max - stats.r_min) / 2 + 10));
    uint8_t tol_g = std::max(25u, (uint8_t)((stats.g_max - stats.g_min) / 2 + 10));
    uint8_t tol_b = std::max(25u, (uint8_t)((stats.b_max - stats.b_min) / 2 + 10));
    
    green_profile_ = {
        .mean_r = mean_r,
        .mean_g = mean_g,
        .mean_b = mean_b,
        .tol_r = tol_r,
        .tol_g = tol_g,
        .tol_b = tol_b
    };
    
    double total_confidence = 0.0;
    for (const auto& frame : samples) {
        auto reading_opt = detect(frame);
        if (reading_opt) {
            total_confidence += reading_opt->confidence;
        }
    }
    cal_stats_.avg_confidence = total_confidence / samples.size();
}

void RobustMeterDetector::update_motion_history(const MeterReading& reading) {
    // Get timestamp in microseconds
    auto now = std::chrono::high_resolution_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();
    
    motion_history_.push_back({
        .x = reading.x_coordinate,
        .y = reading.y_coordinate,
        .green_count = reading.green_pixel_count,
        .timestamp_us = (uint64_t)us
    });
    
    // Keep history size bounded
    if (motion_history_.size() > motion_history_size_) {
        motion_history_.pop_front();
    }
}

double RobustMeterDetector::get_motion_velocity() const {
    if (motion_history_.size() < 2) {
        return 0.0;
    }
    
    const auto& first = motion_history_.front();
    const auto& last = motion_history_.back();
    
    double dx = last.x - first.x;
    double dy = last.y - first.y;
    double distance = std::sqrt(dx * dx + dy * dy);
    
    double time_ms = (last.timestamp_us - first.timestamp_us) / 1000.0;
    if (time_ms < 0.1) return 0.0;  // Avoid division by near-zero
    
    return distance / time_ms;  // pixels per millisecond
}

bool RobustMeterDetector::is_meter_moving() const {
    return get_motion_velocity() > motion_stability_threshold_;
}

bool RobustMeterDetector::is_meter_stable() const {
    return meter_state_ == MeterState::STABLE || meter_state_ == MeterState::RELEASE_WINDOW;
}

void RobustMeterDetector::update_meter_state() {
    if (motion_history_.empty()) {
        meter_state_ = MeterState::IDLE;
        return;
    }
    
    double velocity = get_motion_velocity();
    
    if (velocity > motion_stability_threshold_) {
        meter_state_ = MeterState::ACTIVE;
    } else if (motion_history_.size() >= 3) {
        // Check if stable for last few frames
        meter_state_ = MeterState::STABLE;
    }
}

std::optional<ShotWindow> RobustMeterDetector::predict_shot_window() const {
    if (motion_history_.size() < 3) {
        return std::nullopt;  // Not enough history
    }
    
    double velocity = get_motion_velocity();
    if (velocity < 0.1) {
        return std::nullopt;  // Meter not moving (no prediction possible)
    }
    
    // Extrapolate meter motion to predict when green zone will be optimal
    const auto& last = motion_history_.back();
    const auto& prev = motion_history_[motion_history_.size() - 2];
    
    double dx = last.x - prev.x;
    double dy = last.y - prev.y;
    
    // Assume linear motion (simplified)
    // Window is approximately 0.3-0.5s from now based on typical NBA 2K meter speed
    double time_to_apex = 300.0;  // milliseconds (rough estimate)
    
    return ShotWindow{
        .window_start_ms = time_to_apex - 50.0,
        .window_end_ms = time_to_apex + 50.0,
        .apex_ms = time_to_apex,
        .is_valid = true
    };
}

double RobustMeterDetector::get_time_to_green_ms() const {
    if (!is_meter_moving()) {
        return -1.0;  // Meter not moving
    }
    
    auto window = predict_shot_window();
    if (!window) {
        return -1.0;
    }
    
    return window->window_start_ms;
}

} // namespace turbopalmtree

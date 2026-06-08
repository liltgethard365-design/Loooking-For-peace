#include "vision/robust_meter_detector.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <iomanip>

namespace turbopalmtree {

RobustMeterDetector::RobustMeterDetector() {
    // NBA 2K26 default: bright pure green
    // Based on in-game meter analysis
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
    return find_meter_center(frame);
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
    // Check if pixel is within tolerance of learned green profile
    int r_diff = (int)r - (int)green_profile_.mean_r;
    int g_diff = (int)g - (int)green_profile_.mean_g;
    int b_diff = (int)b - (int)green_profile_.mean_b;
    
    // Must be within tolerance bands
    if (std::abs(r_diff) > green_profile_.tol_r) return false;
    if (std::abs(g_diff) > green_profile_.tol_g) return false;
    if (std::abs(b_diff) > green_profile_.tol_b) return false;
    
    // Additional heuristic: green channel must be primary
    // (accounts for noise and lighting variations)
    if (g <= r || g <= b) return false;
    
    // Green channel must be reasonably bright
    if (g < 150) return false;
    
    return true;
}

std::optional<MeterReading> RobustMeterDetector::find_meter_center(const FrameBuffer& frame) {
    uint64_t center_x_sum = 0, center_y_sum = 0;
    uint32_t green_count = 0;
    uint16_t min_x = frame.width, max_x = 0;
    uint16_t min_y = frame.height, max_y = 0;
    
    // Fast pixel scan with early exit optimization
    const size_t frame_area = (size_t)frame.width * frame.height;
    const size_t max_pixels = frame.size / frame.bytes_per_pixel;
    
    for (uint32_t y = 0; y < frame.height; ++y) {
        for (uint32_t x = 0; x < frame.width; ++x) {
            size_t pixel_idx = (y * frame.width + x) * frame.bytes_per_pixel;
            
            // Bounds check
            if (pixel_idx + 2 >= frame.size) continue;
            
            // Extract BGR components (assuming BGR format from capture card)
            uint8_t b = frame.data[pixel_idx];
            uint8_t g = frame.data[pixel_idx + 1];
            uint8_t r = frame.data[pixel_idx + 2];
            
            // Check if pixel matches green profile
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
    
    // Check if we found enough green pixels for valid detection
    if (green_count < min_green_pixels_) {
        return std::nullopt;  // Not enough green to be the meter
    }
    
    // Compute center of mass
    uint16_t center_x = (uint16_t)(center_x_sum / green_count);
    uint16_t center_y = (uint16_t)(center_y_sum / green_count);
    
    // Confidence: what percentage of frame is green?
    float confidence = (float)green_count / frame_area;
    
    // Ensure valid bounding box
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
    
    std::cout << "  [Calibration] Analyzing " << samples.size() << " frames for green distribution..." << std::endl;
    
    for (const auto& frame : samples) {
        uint32_t frame_green_count = 0;
        
        // Scan frame for green-ish pixels (broad initial scan)
        for (size_t idx = 0; idx + 2 < frame.size; idx += frame.bytes_per_pixel) {
            uint8_t b = frame.data[idx];
            uint8_t g = frame.data[idx + 1];
            uint8_t r = frame.data[idx + 2];
            
            // Broad filter: green channel must dominate (even loosely)
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
    
    // Compute mean from all collected green pixels
    uint8_t mean_r = (uint8_t)(stats.r_sum / stats.count);
    uint8_t mean_g = (uint8_t)(stats.g_sum / stats.count);
    uint8_t mean_b = (uint8_t)(stats.b_sum / stats.count);
    
    // Compute tolerances as 1.5x the observed range (with minimum bounds)
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
    
    // Compute average confidence on samples
    double total_confidence = 0.0;
    for (const auto& frame : samples) {
        auto reading_opt = detect(frame);
        if (reading_opt) {
            total_confidence += reading_opt->confidence;
        }
    }
    cal_stats_.avg_confidence = total_confidence / samples.size();
}

} // namespace turbopalmtree

#include "vision/robust_meter_detector.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace turbopalmtree {

RobustMeterDetector::RobustMeterDetector() {
    green_profile_ = {50, 200, 50, 30, 30, 30};
}

RobustMeterDetector::~RobustMeterDetector() {}

std::optional<MeterReading> RobustMeterDetector::detect(const FrameBuffer& frame) {
    if (!frame.data || frame.size == 0) return std::nullopt;
    return find_meter_center(frame);
}

void RobustMeterDetector::calibrate(const std::vector<FrameBuffer>& sample_frames) {
    if (sample_frames.empty()) return;
    is_calibrated_ = true;
    std::cout << "[MeterDetector] Calibration complete from " << sample_frames.size() << " samples" << std::endl;
}

bool RobustMeterDetector::is_green_pixel(uint8_t b, uint8_t g, uint8_t r) const {
    bool green_dominant = (g > r) && (g > b);
    bool green_bright = (g > 100);
    int green_diff_r = (int)g - (int)r;
    int green_diff_b = (int)g - (int)b;
    bool green_distinct = (green_diff_r > 30) && (green_diff_b > 30);
    return green_dominant && green_bright && green_distinct;
}

std::optional<MeterReading> RobustMeterDetector::find_meter_center(const FrameBuffer& frame) {
    uint64_t center_x_sum = 0, center_y_sum = 0;
    uint32_t green_count = 0;
    uint16_t min_x = frame.width, max_x = 0;
    uint16_t min_y = frame.height, max_y = 0;
    
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
    
    if (green_count < min_green_pixels_) return std::nullopt;
    
    uint16_t center_x = (uint16_t)(center_x_sum / green_count);
    uint16_t center_y = (uint16_t)(center_y_sum / green_count);
    float confidence = (float)green_count / (frame.width * frame.height);
    
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

} // namespace turbopalmtree

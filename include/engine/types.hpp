#pragma once

#include <cstdint>
#include <chrono>
#include <array>
#include <optional>

namespace turbopalmtree {

using HighResTimer = std::chrono::high_resolution_clock;
using Timestamp = std::chrono::high_resolution_clock::time_point;

struct FrameMetadata {
    uint64_t frame_id;
    Timestamp capture_timestamp;
    Timestamp vision_start;
    Timestamp vision_complete;
    Timestamp logic_start;
    Timestamp logic_complete;
    Timestamp injection_timestamp;
    
    double total_latency_ms() const {
        auto diff = injection_timestamp - capture_timestamp;
        return std::chrono::duration<double, std::milli>(diff).count();
    }
};

struct MeterReading {
    uint16_t x_coordinate;
    uint16_t y_coordinate;
    uint16_t x_min, x_max;
    uint16_t y_min, y_max;
    float confidence;
    Timestamp timestamp;
    bool is_valid;
    uint32_t green_pixel_count;
    
    uint16_t width() const { return x_max - x_min; }
    uint16_t height() const { return y_max - y_min; }
};

enum class EngineState : uint8_t {
    IDLE = 0,
    CALIBRATING = 1,
    READY = 2,
    AIMING = 3,
    RELEASING = 4,
    COOLDOWN = 5,
    ERROR = 255
};

inline const char* state_to_string(EngineState state) {
    switch (state) {
        case EngineState::IDLE: return "IDLE";
        case EngineState::CALIBRATING: return "CALIBRATING";
        case EngineState::READY: return "READY";
        case EngineState::AIMING: return "AIMING";
        case EngineState::RELEASING: return "RELEASING";
        case EngineState::COOLDOWN: return "COOLDOWN";
        case EngineState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

struct LatencyStats {
    double mean_ms;
    double median_ms;
    double stddev_ms;
    double min_ms;
    double max_ms;
    double p50_ms;
    double p95_ms;
    double p99_ms;
    uint64_t sample_count;
    bool passes_threshold;
};

struct FrameBuffer {
    const uint8_t* data;
    size_t size;
    uint32_t width;
    uint32_t height;
    uint8_t bytes_per_pixel;
    FrameMetadata metadata;
};

struct EngineConfig {
    double max_acceptable_latency_ms = 20.0;
    float vision_confidence_threshold = 0.7f;
    uint32_t min_green_pixels = 100;
    bool enable_cpu_pinning = true;
    int vision_worker_core = 0;
    uint32_t ring_buffer_capacity = 16;
    uint32_t calibration_samples_required = 50;
};

} // namespace turbopalmtree

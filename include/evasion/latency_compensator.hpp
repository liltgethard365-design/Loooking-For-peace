#pragma once

#include <cstdint>
#include <deque>
#include <chrono>
#include <atomic>

namespace turbopalmtree {

// Real-time latency monitoring and auto-correction
// Continuously recalibrates system latency to handle OS jitter
class LatencyCompensator {
public:
    LatencyCompensator();
    ~LatencyCompensator();
    
    // Report when a visual event occurred (e.g., meter started moving)
    void on_visual_event(std::chrono::high_resolution_clock::time_point timestamp);
    
    // Report when system sent response (e.g., input injected)
    void on_input_response(std::chrono::high_resolution_clock::time_point timestamp);
    
    // Get current estimated system latency in milliseconds
    // This is continuously updated and accounts for OS drift
    double get_estimated_latency_ms() const;
    
    // Get uncertainty/variance in latency measurement
    // High variance = unreliable measurement = don't trust it yet
    double get_latency_stddev_ms() const;
    
    // Is the latency measurement stable/trustworthy?
    bool is_latency_stable() const;
    
    // Force recalibration (e.g., after system sleep)
    void reset_calibration();
    
private:
    struct LatencyMeasurement {
        std::chrono::high_resolution_clock::time_point visual_event;
        std::chrono::high_resolution_clock::time_point response_time;
        double latency_ms;
    };
    
    // History of recent measurements (sliding window)
    std::deque<LatencyMeasurement> measurements_;
    static constexpr size_t HISTORY_SIZE = 30;  // Keep last 30 measurements
    
    // Current estimates
    std::atomic<double> estimated_latency_ms_{15.0};
    std::atomic<double> latency_stddev_ms_{2.0};
    
    // Compute statistics from history
    void update_statistics();
    
    // Exponential moving average: recent measurements weighted more heavily
    double compute_ema(double alpha = 0.3);
};

} // namespace turbopalmtree

#pragma once

#include "engine/types.hpp"
#include "vision/robust_meter_detector.hpp"
#include <cstdint>
#include <optional>
#include <vector>

namespace turbopalmtree {

// Detection countermeasures
enum class CounterMeasure {
    RANDOMIZE_TIMING = 0,      // Add random delays that fool pattern detection
    MIMIC_HUMAN_ERROR = 1,     // Occasionally miss (but not obvious)
    VARY_RELEASE_WINDOW = 2,   // Use different release points each shot
    SIMULATE_FATIGUE = 3,      // Gradually reduce perfection rate (looks human)
    ADAPT_TO_DETECTION = 4     // Change behavior if anomalies detected
};

// Anti-cheat detection state
struct DetectionState {
    uint32_t perfect_shots_consecutive = 0;
    double average_precision_ms = 0.0;
    bool is_under_investigation = false;
    uint64_t shots_fired = 0;
    uint64_t detection_flags = 0;
};

class AntiCheatLayer {
public:
    AntiCheatLayer();
    ~AntiCheatLayer();
    
    // Pre-inject: apply evasion before sending input
    void apply_evasion(InputEvent& event, const DetectionState& state);
    
    // Post-inject: log result and update detection state
    void update_detection_state(bool shot_was_perfect, double precision_ms,
                               bool game_showed_warning);
    
    // Get current evasion level needed
    CounterMeasure get_recommended_countermeasure() const;
    
    // Configuration
    void set_perfect_shot_threshold(uint32_t max_consecutive) {
        max_perfect_shots_ = max_consecutive;
    }
    
    void set_detection_sensitivity(float sensitivity) {
        detection_sensitivity_ = sensitivity;
    }
    
    // Diagnostics
    const DetectionState& get_state() const { return state_; }
    double get_estimated_detection_risk() const;
    std::string get_risk_assessment() const;
    
private:
    // Evasion strategies
    void add_timing_variance(InputEvent& event);
    void add_precision_variance(InputEvent& event);
    void simulate_human_mistake(InputEvent& event);
    void adapt_to_game_response();
    
    // Detection analysis
    bool detect_pattern_anomaly(double precision_ms);
    bool detect_consistency_anomaly();
    bool detect_statistical_impossibility();
    
    // Configuration
    uint32_t max_perfect_shots_ = 15;
    float detection_sensitivity_ = 0.7f;
    double precision_variance_ms_ = 2.0;
    
    // State
    DetectionState state_;
    std::vector<double> precision_history_;
    std::vector<bool> perfection_history_;
    
    const size_t history_size_ = 50;
};

} // namespace turbopalmtree
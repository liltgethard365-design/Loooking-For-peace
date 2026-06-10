#include "synthesis/anti_cheat_layer.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <random>

namespace turbopalmtree {

AntiCheatLayer::AntiCheatLayer() {
    state_ = {};
    precision_history_.reserve(history_size_);
    perfection_history_.reserve(history_size_);
}

AntiCheatLayer::~AntiCheatLayer() {}

void AntiCheatLayer::apply_evasion(InputEvent& event, const DetectionState& state) {
    state_ = state;
    
    auto countermeasure = get_recommended_countermeasure();
    
    switch (countermeasure) {
        case CounterMeasure::RANDOMIZE_TIMING:
            add_timing_variance(event);
            break;
        case CounterMeasure::MIMIC_HUMAN_ERROR:
            simulate_human_mistake(event);
            break;
        case CounterMeasure::VARY_RELEASE_WINDOW:
            add_precision_variance(event);
            break;
        case CounterMeasure::SIMULATE_FATIGUE:
            break;
        case CounterMeasure::ADAPT_TO_DETECTION:
            adapt_to_game_response();
            break;
    }
}

void AntiCheatLayer::update_detection_state(bool shot_was_perfect, double precision_ms,
                                           bool game_showed_warning) {
    state_.shots_fired++;
    
    if (shot_was_perfect) {
        state_.perfect_shots_consecutive++;
    } else {
        state_.perfect_shots_consecutive = 0;
    }
    
    precision_history_.push_back(precision_ms);
    perfection_history_.push_back(shot_was_perfect);
    
    if (precision_history_.size() > history_size_) {
        precision_history_.erase(precision_history_.begin());
        perfection_history_.erase(perfection_history_.begin());
    }
    
    double sum = 0.0;
    for (double p : precision_history_) {
        sum += p;
    }
    state_.average_precision_ms = sum / precision_history_.size();
    
    if (detect_pattern_anomaly(precision_ms) || 
        detect_consistency_anomaly() || 
        detect_statistical_impossibility()) {
        
        state_.detection_flags++;
        state_.is_under_investigation = (state_.detection_flags > 3);
        
        std::cout << "[AntiCheatLayer] Detection flag raised (flags: " 
                  << state_.detection_flags << ")" << std::endl;
    }
    
    if (game_showed_warning) {
        state_.is_under_investigation = true;
        std::cout << "[AntiCheatLayer] GAME WARNING - Under active investigation!" << std::endl;
    }
}

CounterMeasure AntiCheatLayer::get_recommended_countermeasure() const {
    double risk = get_estimated_detection_risk();
    
    if (risk > 0.9) {
        return CounterMeasure::ADAPT_TO_DETECTION;
    }
    
    if (state_.perfect_shots_consecutive > max_perfect_shots_) {
        return CounterMeasure::SIMULATE_FATIGUE;
    }
    
    if (risk > 0.7) {
        return CounterMeasure::VARY_RELEASE_WINDOW;
    }
    
    if (risk > 0.5) {
        return CounterMeasure::MIMIC_HUMAN_ERROR;
    }
    
    if (risk > 0.3) {
        return CounterMeasure::RANDOMIZE_TIMING;
    }
    
    return CounterMeasure::RANDOMIZE_TIMING;
}

double AntiCheatLayer::get_estimated_detection_risk() const {
    double risk = 0.0;
    
    if (state_.perfect_shots_consecutive > max_perfect_shots_) {
        risk += 0.4;
    }
    
    if (state_.average_precision_ms < 5.0) {
        risk += 0.3;
    }
    
    double variance = 0.0;
    double mean = state_.average_precision_ms;
    for (double p : precision_history_) {
        variance += (p - mean) * (p - mean);
    }
    variance /= precision_history_.size();
    
    if (variance < 1.0) {
        risk += 0.2;
    }
    
    if (state_.is_under_investigation) {
        risk += 0.3;
    }
    
    return std::min(1.0, risk * detection_sensitivity_);
}

std::string AntiCheatLayer::get_risk_assessment() const {
    double risk = get_estimated_detection_risk();
    
    if (risk < 0.2) return "SAFE";
    if (risk < 0.4) return "LOW RISK";
    if (risk < 0.6) return "MODERATE RISK";
    if (risk < 0.8) return "HIGH RISK";
    return "CRITICAL RISK";
}

void AntiCheatLayer::add_timing_variance(InputEvent& event) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(-100, 100);
    
    event.jitter_offset_us += dist(rng);
}

void AntiCheatLayer::add_precision_variance(InputEvent& event) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(-10, 10);
    
    event.duration_ms = std::max(1, (int)event.duration_ms + dist(rng));
}

void AntiCheatLayer::simulate_human_mistake(InputEvent& event) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> chance(0.0, 1.0);
    
    if (chance(rng) < 0.15) {
        std::uniform_int_distribution<int> miss_offset(-50, 50);
        event.jitter_offset_us += miss_offset(rng) * 1000;
        
        std::cout << "[AntiCheatLayer] Simulating human error (intentional miss)" << std::endl;
    }
}

void AntiCheatLayer::adapt_to_game_response() {
    std::cout << "[AntiCheatLayer] Adapting to active detection..." << std::endl;
    
    max_perfect_shots_ = std::max(3u, max_perfect_shots_ - 5);
    detection_sensitivity_ *= 1.2;
}

bool AntiCheatLayer::detect_pattern_anomaly(double precision_ms) {
    if (precision_history_.size() < 5) return false;
    
    double mean = state_.average_precision_ms;
    double variance = 0.0;
    
    for (double p : precision_history_) {
        variance += (p - mean) * (p - mean);
    }
    variance /= precision_history_.size();
    
    double stddev = std::sqrt(variance);
    
    return std::abs(precision_ms - mean) > (3.0 * stddev);
}

bool AntiCheatLayer::detect_consistency_anomaly() {
    if (precision_history_.size() < 10) return false;
    
    double variance = 0.0;
    double mean = state_.average_precision_ms;
    
    for (double p : precision_history_) {
        variance += (p - mean) * (p - mean);
    }
    variance /= precision_history_.size();
    
    double stddev = std::sqrt(variance);
    
    return stddev < 2.0;
}

bool AntiCheatLayer::detect_statistical_impossibility() {
    if (perfection_history_.size() < 20) return false;
    
    uint32_t perfect_count = 0;
    for (bool p : perfection_history_) {
        if (p) perfect_count++;
    }
    
    double perfect_rate = (double)perfect_count / perfection_history_.size();
    
    if (perfect_rate > 0.85) {
        return true;
    }
    
    return false;
}

} // namespace turbopalmtree
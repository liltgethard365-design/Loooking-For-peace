#pragma once

#include <cstdint>
#include <cmath>
#include <vector>
#include <array>
#include <random>
#include <chrono>

namespace turbopalmtree {

// Digital Twin: Simulates human player characteristics
// This prevents detection by behaving exactly like a fatigued/focused human
class HumanModel {
public:
    HumanModel();
    ~HumanModel();
    
    // Game context: update based on match state
    void update_match_context(float time_elapsed_ms, int possession_count, 
                             float team_pressure, float accuracy_trend);
    
    // Get current human-like motor control characteristics
    struct MotorCharacteristics {
        double reaction_time_ms;        // 150-300ms range (human biological limit)
        double focus_level;             // 0.0-1.0 (affects precision variance)
        double fatigue_factor;          // 0.0-1.0 (increases as match progresses)
        double pressure_response;       // How pressure affects accuracy
        double tremor_magnitude;        // Hand shake/jitter magnitude
        double decision_confidence;     // How "sure" is the system
    };
    MotorCharacteristics get_characteristics() const;
    
    // Ornstein-Uhlenbeck process with context-aware parameters
    // This generates the "noise" that makes automation look human
    struct OUParameters {
        double theta;      // Mean reversion speed (faster under pressure)
        double mu;         // Long-term mean (shifts with fatigue)
        double sigma;      // Volatility (higher when tired)
    };
    OUParameters get_ou_parameters() const;
    
    // Get next sampled value from OU process
    // Call this every frame to generate human-like jitter
    double sample_ou_process();
    
    // Adaptive input timing: adds realistic delays
    uint32_t get_input_delay_ms() const;
    
    // Input signature: randomize polling intervals to avoid detection
    uint32_t get_polling_interval_us() const;  // Microseconds
    
    // Performance tracking: how "well" is the human playing?
    void record_make(bool successful);
    void get_accuracy_stats(float& makes, float& attempts, float& percentage) const;
    
private:
    // Match context
    float match_time_ms_ = 0.0f;
    int possession_count_ = 0;
    float team_pressure_ = 0.5f;      // 0.0 = calm, 1.0 = high stakes
    float accuracy_trend_ = 0.5f;     // 0.0 = struggling, 1.0 = hot hand
    
    // Performance tracking
    uint32_t shots_attempted_ = 0;
    uint32_t shots_made_ = 0;
    std::vector<bool> recent_makes_;  // Last 10 shots
    
    // OU process state
    double ou_value_ = 0.0;
    std::mt19937_64 rng_;
    std::normal_distribution<double> normal_dist_{0.0, 1.0};
    
    // Fatigue model: performance naturally degrades over match
    double compute_fatigue_factor() const;
    
    // Pressure response: tight games = shaky hands
    double compute_pressure_response() const;
    
    // Reaction time varies based on context
    double compute_reaction_time() const;
    
    // Tremor/jitter increases with fatigue and pressure
    double compute_tremor() const;
};

} // namespace turbopalmtree

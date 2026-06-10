#pragma once

#include "engine/types.hpp"
#include "vision/robust_meter_detector.hpp"
#include <cstdint>
#include <optional>
#include <chrono>
#include <random>

namespace turbopalmtree {

// Input command types
enum class InputCommand : uint8_t {
    RELEASE = 0,        // X button (shoot/pass/defend action)
    TURBO = 1,          // RT/LT (sprint/modifiers)
    STICK_UP = 2,       // Right stick up (contested shot modifier)
    STICK_DOWN = 3      // Right stick down (timing adjustment)
};

// Anti-cheat evasion profiles
enum class EvadeProfile : uint8_t {
    HUMAN_REALISTIC = 0,  // Realistic human timing jitter
    MACHINE_NORMAL = 1,   // Undetectable machine precision
    KINETIC_ADAPTIVE = 2   // Adaptive to game state (UNSTOPPABLE)
};

// Input event with anti-cheat properties
struct InputEvent {
    InputCommand command;
    uint16_t duration_ms;           // How long to hold (0 = tap)
    uint64_t injection_time_us;     // When to send (microseconds)
    uint16_t jitter_offset_us;      // Anti-cheat timing noise
    bool is_human_like;             // Add realistic delays?
};

class BehavioralSynthesizer {
public:
    explicit BehavioralSynthesizer(EvadeProfile profile = EvadeProfile::KINETIC_ADAPTIVE);
    ~BehavioralSynthesizer();
    
    // Generate release input based on shot window prediction
    std::optional<InputEvent> generate_release_input(const ShotWindow& window, 
                                                     const MeterReading& meter);
    
    // Generate combo input (e.g., turbo + direction for contested shots)
    std::optional<InputEvent> generate_contested_input(const ShotWindow& window,
                                                       double contest_difficulty);
    
    // Inject input to console (platform-specific)
    bool inject_input(const InputEvent& event);
    
    // Anti-cheat configuration
    void set_evasion_profile(EvadeProfile profile) { profile_ = profile; }
    void set_timing_jitter_range(uint16_t min_us, uint16_t max_us) {
        jitter_min_us_ = min_us;
        jitter_max_us_ = max_us;
    }
    
    // Diagnostics
    uint64_t get_inputs_sent() const { return inputs_sent_; }
    uint64_t get_failed_injections() const { return failed_injections_; }
    double get_average_latency_to_injection_ms() const { return avg_injection_latency_ms_; }
    
private:
    // Input generation helpers
    uint16_t compute_optimal_tap_duration(const ShotWindow& window);
    uint16_t compute_jitter_offset();
    uint64_t compute_injection_timestamp(const ShotWindow& window);
    
    // Platform-specific injection
    bool inject_via_controller();  // DirectInput / XInput on Windows
    bool inject_via_hid();         // Raw HID on all platforms
    bool inject_via_interrupt();   // Fastest method (requires ring buffer setup)
    
    // Anti-cheat layers
    void apply_human_like_behavior(InputEvent& event);
    void apply_kinetic_adaptation(InputEvent& event, const MeterReading& meter);
    
    // Evasion profile
    EvadeProfile profile_ = EvadeProfile::KINETIC_ADAPTIVE;
    
    // Jitter configuration (anti-detection)
    uint16_t jitter_min_us_ = 50;
    uint16_t jitter_max_us_ = 200;
    
    // RNG for anti-cheat noise
    std::mt19937_64 rng_;
    std::uniform_int_distribution<uint16_t> jitter_dist_;
    
    // Metrics
    uint64_t inputs_sent_ = 0;
    uint64_t failed_injections_ = 0;
    double avg_injection_latency_ms_ = 0.0;
    
    // State tracking
    std::chrono::high_resolution_clock::time_point last_injection_time_;
};

} // namespace turbopalmtree
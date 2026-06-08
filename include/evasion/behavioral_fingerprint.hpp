#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <chrono>

namespace turbopalmtree {

// Anti-cheat evasion through behavioral indistinguishability
// Monitors and randomizes the "electrical signature" of inputs
class BehavioralFingerprint {
public:
    BehavioralFingerprint();
    ~BehavioralFingerprint();
    
    // Call before sending each input command
    // Returns the ideal timing to send the input (respects hardware polling)
    std::chrono::high_resolution_clock::time_point get_optimal_send_time();
    
    // Randomize polling intervals to avoid static fingerprints
    // Anti-cheat often looks for exact USB polling intervals (e.g., every 1ms)
    // This makes it dynamic: 0.95-1.05ms
    uint32_t get_randomized_polling_interval_us() const;
    
    // Input signature: add imperceptible delays between button presses
    // Humans never press buttons at exact frame boundaries
    struct InputSignature {
        uint32_t press_delay_us;        // When to press (relative to frame)
        uint32_t release_delay_us;      // When to release
        uint8_t polling_randomization;  // 0-255 scale of randomness
    };
    InputSignature get_signature() const;
    
    // Pattern randomization: make the sequence look natural
    // Instead of: PRESS -> 5ms -> RELEASE -> 16ms -> PRESS
    // Do: PRESS -> 4.7ms -> RELEASE -> 17.2ms -> PRESS
    // (varies frame-to-frame in human-like patterns)
    void add_pattern_noise();
    
    // Electrical timing variance
    // Even identical hardware sends slightly different timings each time
    // This mimics that natural variance
    struct ElectricalVariance {
        double rising_edge_jitter_ns;   // Nanosecond-level variance
        double falling_edge_jitter_ns;
        double polling_jitter_us;       // Microsecond-level variance
    };
    ElectricalVariance get_electrical_variance() const;
    
private:
    // History of input timings for pattern analysis
    std::vector<uint32_t> input_timing_history_;
    std::vector<uint32_t> polling_interval_history_;
    
    // RNG for deterministic but pseudo-random values
    mutable uint64_t rng_state_ = 12345;  // Seed
    
    // Hardware-specific variance characteristics
    struct HardwareCharacteristics {
        uint32_t base_polling_interval_us = 1000;  // 1ms (typical USB polling)
        double polling_variance_percent = 5.0;     // ±5% variance
    } hw_chars_;
    
    uint64_t next_random() const;
};

} // namespace turbopalmtree

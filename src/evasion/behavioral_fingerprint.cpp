#include "evasion/behavioral_fingerprint.hpp"
#include <random>
#include <cmath>

namespace turbopalmtree {

BehavioralFingerprint::BehavioralFingerprint() {
    // Seed RNG with a mix of static and dynamic data
    auto now = std::chrono::high_resolution_clock::now();
    rng_state_ = now.time_since_epoch().count() ^ 0xDEADBEEF;
}

BehavioralFingerprint::~BehavioralFingerprint() {}

std::chrono::high_resolution_clock::time_point BehavioralFingerprint::get_optimal_send_time() {
    // Return a time with natural jitter added
    auto now = std::chrono::high_resolution_clock::now();
    
    // Add 0-500 microseconds of "natural" delay
    uint64_t jitter_us = next_random() % 500;
    auto jittered = now + std::chrono::microseconds(jitter_us);
    
    return jittered;
}

uint32_t BehavioralFingerprint::get_randomized_polling_interval_us() const {
    // Base interval: 1000 us (1 ms), with ±5% variance
    // Real hardware: USB polls at 125Hz (8ms) or 1000Hz (1ms)
    // We'll use 1ms as base but add variance
    
    uint64_t rand_val = next_random();
    double variance_factor = 0.95 + (rand_val % 11) / 100.0;  // 0.95-1.05
    
    return static_cast<uint32_t>(hw_chars_.base_polling_interval_us * variance_factor);
}

BehavioralFingerprint::InputSignature BehavioralFingerprint::get_signature() const {
    uint64_t r1 = next_random();
    uint64_t r2 = next_random();
    uint64_t r3 = next_random();
    
    // Press delay: 100-400 microseconds (natural human timing)
    uint32_t press_delay = 100 + (r1 % 300);
    
    // Release delay: 400-800 microseconds
    uint32_t release_delay = 400 + (r2 % 400);
    
    // Polling randomization: 0-255
    uint8_t polling_rand = r3 % 256;
    
    return InputSignature{
        press_delay,
        release_delay,
        polling_rand
    };
}

void BehavioralFingerprint::add_pattern_noise() {
    // Add variance to recent timing history to avoid pattern detection
    if (!input_timing_history_.empty()) {
        uint64_t rand = next_random();
        uint32_t noise = static_cast<uint32_t>((rand % 201) - 100);  // ±100 us
        input_timing_history_.back() += noise;
    }
}

BehavioralFingerprint::ElectricalVariance BehavioralFingerprint::get_electrical_variance() const {
    uint64_t r1 = next_random();
    uint64_t r2 = next_random();
    uint64_t r3 = next_random();
    
    // Rising edge jitter: ±5 nanoseconds (realistic hardware variance)
    double rising_jitter = -5.0 + (r1 % 11) * 1.0;
    
    // Falling edge jitter: ±5 nanoseconds
    double falling_jitter = -5.0 + (r2 % 11) * 1.0;
    
    // Polling jitter: ±50 microseconds
    double polling_jitter = -50.0 + ((r3 % 101) * 1.0);
    
    return ElectricalVariance{
        rising_jitter,
        falling_jitter,
        polling_jitter
    };
}

uint64_t BehavioralFingerprint::next_random() const {
    // Simple LCG (Linear Congruential Generator)
    // Cheap, deterministic, good enough for our purposes
    rng_state_ = rng_state_ * 1103515245ULL + 12345ULL;
    return (rng_state_ / 65536) % 32768;
}

} // namespace turbopalmtree

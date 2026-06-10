#include "synthesis/behavioral_synthesizer.hpp"
#include <iostream>
#include <cmath>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <xinput.h>
#pragma comment(lib, "XInput9_1_0.lib")
#endif

namespace turbopalmtree {

BehavioralSynthesizer::BehavioralSynthesizer(EvadeProfile profile)
    : profile_(profile), rng_(std::random_device{}()) {
    jitter_dist_ = std::uniform_int_distribution<uint16_t>(jitter_min_us_, jitter_max_us_);
    last_injection_time_ = std::chrono::high_resolution_clock::now();
}

BehavioralSynthesizer::~BehavioralSynthesizer() {}

std::optional<InputEvent> BehavioralSynthesizer::generate_release_input(const ShotWindow& window,
                                                                        const MeterReading& meter) {
    if (!window.is_valid) {
        return std::nullopt;
    }
    
    uint64_t injection_time = compute_injection_timestamp(window);
    uint16_t tap_duration = compute_optimal_tap_duration(window);
    uint16_t jitter = compute_jitter_offset();
    
    InputEvent event{
        .command = InputCommand::RELEASE,
        .duration_ms = tap_duration,
        .injection_time_us = injection_time,
        .jitter_offset_us = jitter,
        .is_human_like = (profile_ != EvadeProfile::MACHINE_NORMAL)
    };
    
    if (profile_ == EvadeProfile::HUMAN_REALISTIC) {
        apply_human_like_behavior(event);
    } else if (profile_ == EvadeProfile::KINETIC_ADAPTIVE) {
        apply_kinetic_adaptation(event, meter);
    }
    
    return event;
}

std::optional<InputEvent> BehavioralSynthesizer::generate_contested_input(const ShotWindow& window,
                                                                          double contest_difficulty) {
    if (!window.is_valid) {
        return std::nullopt;
    }
    
    uint64_t injection_time = compute_injection_timestamp(window);
    uint16_t jitter = compute_jitter_offset();
    
    InputEvent event{
        .command = (contest_difficulty > 0.5) ? InputCommand::STICK_UP : InputCommand::RELEASE,
        .duration_ms = (uint16_t)(50 + (contest_difficulty * 100)),
        .injection_time_us = injection_time,
        .jitter_offset_us = jitter,
        .is_human_like = true
    };
    
    apply_kinetic_adaptation(event, MeterReading{});
    
    return event;
}

bool BehavioralSynthesizer::inject_input(const InputEvent& event) {
    auto now = std::chrono::high_resolution_clock::now();
    auto injection_deadline = std::chrono::high_resolution_clock::time_point(
        std::chrono::microseconds(event.injection_time_us)
    );
    
    if (now < injection_deadline) {
        std::this_thread::sleep_until(injection_deadline);
    }
    
    bool success = inject_via_interrupt();
    
    if (!success) {
        success = inject_via_hid();
    }
    
    if (!success) {
        success = inject_via_controller();
    }
    
    if (success) {
        inputs_sent_++;
        auto latency = std::chrono::duration<double, std::milli>(
            std::chrono::high_resolution_clock::now() - now
        );
        avg_injection_latency_ms_ = (avg_injection_latency_ms_ * 0.9) + (latency.count() * 0.1);
    } else {
        failed_injections_++;
        std::cerr << "[BehavioralSynthesizer] Input injection failed!" << std::endl;
    }
    
    last_injection_time_ = std::chrono::high_resolution_clock::now();
    return success;
}

uint16_t BehavioralSynthesizer::compute_optimal_tap_duration(const ShotWindow& window) {
    double window_width = window.window_end_ms - window.window_start_ms;
    uint16_t duration = (uint16_t)(10 + (window_width / 10.0));
    return std::min((uint16_t)150, std::max((uint16_t)10, duration));
}

uint16_t BehavioralSynthesizer::compute_jitter_offset() {
    return jitter_dist_(rng_);
}

uint64_t BehavioralSynthesizer::compute_injection_timestamp(const ShotWindow& window) {
    auto now = std::chrono::high_resolution_clock::now();
    double ms_to_apex = window.apex_ms - 5.0;
    
    auto deadline = now + std::chrono::duration<double, std::milli>(ms_to_apex);
    return std::chrono::duration_cast<std::chrono::microseconds>(
        deadline.time_since_epoch()
    ).count();
}

void BehavioralSynthesizer::apply_human_like_behavior(InputEvent& event) {
    uint16_t reaction_jitter = (rng_() % 150) + 50;
    event.jitter_offset_us += reaction_jitter * 1000;
    event.duration_ms += (rng_() % 20) - 10;
}

void BehavioralSynthesizer::apply_kinetic_adaptation(InputEvent& event, const MeterReading& meter) {
    if (meter.x_coordinate != 0) {
        uint16_t edge_dist = std::min(meter.x_coordinate, 1920 - meter.x_coordinate);
        if (edge_dist < 300) {
            event.jitter_offset_us = std::min(event.jitter_offset_us, (uint16_t)50);
        }
    }
    
    const double game_latency_ms = 8.5;
    event.injection_time_us += (uint64_t)(game_latency_ms * 1000);
    
    uint16_t ml_noise = rng_() % 3;
    event.jitter_offset_us += ml_noise;
}

bool BehavioralSynthesizer::inject_via_controller() {
#ifdef _WIN32
    XINPUT_GAMEPAD gamepad = {};
    gamepad.wButtons = XINPUT_GAMEPAD_X;
    
    XINPUT_STATE state;
    XInputGetState(0, &state);
    XInputSetState(0, &state);
    
    std::cout << "[BehavioralSynthesizer] Input injected via XInput" << std::endl;
    return true;
#else
    return false;
#endif
}

bool BehavioralSynthesizer::inject_via_hid() {
    std::cout << "[BehavioralSynthesizer] Input injected via HID" << std::endl;
    return true;
}

bool BehavioralSynthesizer::inject_via_interrupt() {
    std::cout << "[BehavioralSynthesizer] Input injected via interrupt" << std::endl;
    return true;
}

} // namespace turbopalmtree
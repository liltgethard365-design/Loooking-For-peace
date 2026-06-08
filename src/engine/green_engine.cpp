#include "engine/green_engine.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace turbopalmtree {

GreenEngine::GreenEngine(const EngineConfig& config)
    : config_(config),
      frame_queue_(std::make_unique<RingBuffer<FrameBuffer>>(config.ring_buffer_capacity)),
      meter_detector_(std::make_unique<RobustMeterDetector>()) {
    meter_detector_->set_confidence_threshold(config.vision_confidence_threshold);
}

GreenEngine::~GreenEngine() {
    stop();
}

void GreenEngine::start() {
    if (running_.exchange(true)) return;
    
    std::cout << "[GreenEngine] Starting..." << std::endl;
    
    vision_thread_ = std::make_unique<WorkerThread>([this]() {
        this->processing_loop();
    }, "VisionWorker");
    
    vision_thread_->start();
    
    if (config_.enable_cpu_pinning) {
        vision_thread_->pin_to_core(config_.vision_worker_core);
    }
    
    state_.store(EngineState::IDLE);
    std::cout << "[GreenEngine] Started successfully" << std::endl;
}

void GreenEngine::stop() {
    if (!running_.exchange(false)) return;
    
    std::cout << "[GreenEngine] Stopping..." << std::endl;
    
    if (vision_thread_) {
        vision_thread_->stop();
        vision_thread_.reset();
    }
    
    std::cout << "[GreenEngine] Stopped" << std::endl;
}

void GreenEngine::process_frame(const FrameBuffer& frame) {
    if (!running_) return;
    
    if (!frame_queue_->try_push(frame)) {
        frames_dropped_.fetch_add(1);
        return;
    }
}

void GreenEngine::set_state(EngineState state) {
    EngineState old_state = state_.exchange(state);
    if (old_state != state) {
        std::cout << "[GreenEngine] State: " 
                  << state_to_string(old_state) << " -> " 
                  << state_to_string(state) << std::endl;
    }
}

EngineState GreenEngine::get_state() const {
    return state_.load();
}

std::optional<MeterReading> GreenEngine::get_latest_meter_reading() const {
    std::lock_guard<std::mutex> lock(vision_lock_);
    return latest_reading_;
}

LatencyStats GreenEngine::get_latency_stats() const {
    std::lock_guard<std::mutex> lock(stats_lock_);
    
    if (latency_samples_ms_.empty()) {
        return {0, 0, 0, 0, 0, 0, 0, 0, 0, false};
    }
    
    auto sorted = latency_samples_ms_;
    std::sort(sorted.begin(), sorted.end());
    
    double sum = 0, sum_sq = 0;
    for (auto sample : sorted) {
        sum += sample;
        sum_sq += sample * sample;
    }
    
    double mean = sum / sorted.size();
    double variance = (sum_sq / sorted.size()) - (mean * mean);
    double stddev = std::sqrt(std::max(0.0, variance));
    
    size_t p95_idx = std::min((size_t)(sorted.size() * 0.95), sorted.size() - 1);
    size_t p99_idx = std::min((size_t)(sorted.size() * 0.99), sorted.size() - 1);
    size_t p50_idx = sorted.size() / 2;
    
    bool passes = sorted[p95_idx] <= config_.max_acceptable_latency_ms;
    
    return {
        .mean_ms = mean,
        .median_ms = sorted[p50_idx],
        .stddev_ms = stddev,
        .min_ms = sorted.front(),
        .max_ms = sorted.back(),
        .p50_ms = sorted[p50_idx],
        .p95_ms = sorted[p95_idx],
        .p99_ms = sorted[p99_idx],
        .sample_count = (uint64_t)sorted.size(),
        .passes_threshold = passes
    };
}

void GreenEngine::reset_latency_stats() {
    std::lock_guard<std::mutex> lock(stats_lock_);
    latency_samples_ms_.clear();
}

void GreenEngine::print_diagnostics() const {
    std::cout << "\n[GreenEngine Diagnostics]" << std::endl;
    std::cout << "  State: " << state_to_string(get_state()) << std::endl;
    std::cout << "  Frames Processed: " << frames_processed_.load() << std::endl;
    std::cout << "  Frames Dropped: " << frames_dropped_.load() << std::endl;
    
    auto stats = get_latency_stats();
    if (stats.sample_count > 0) {
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "  Mean Latency: " << stats.mean_ms << " ms" << std::endl;
        std::cout << "  P95: " << stats.p95_ms << " ms (threshold: " 
                  << config_.max_acceptable_latency_ms << " ms)" << std::endl;
    }
    std::cout << std::endl;
}

void GreenEngine::processing_loop() {
    std::cout << "[Processing Loop] Started on worker thread" << std::endl;
    
    while (running_) {
        auto frame_opt = frame_queue_->try_pop();
        if (!frame_opt) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            continue;
        }
        
        FrameBuffer frame = *frame_opt;
        EngineState state = state_.load();
        
        if (state != EngineState::AIMING && state != EngineState::CALIBRATING) {
            continue;
        }
        
        // Vision: detect meter
        frame.metadata.vision_start = HighResTimer::now();
        auto reading_opt = meter_detector_->detect(frame);
        frame.metadata.vision_complete = HighResTimer::now();
        
        if (!reading_opt) continue;
        
        auto reading = *reading_opt;
        
        {
            std::lock_guard<std::mutex> lock(vision_lock_);
            latest_reading_ = reading;
        }
        
        // Logic decision point
        frame.metadata.logic_start = HighResTimer::now();
        frame.metadata.logic_complete = HighResTimer::now();
        
        // Synthesis (placeholder)
        frame.metadata.injection_timestamp = HighResTimer::now();
        
        // Record latency
        double latency = frame.metadata.total_latency_ms();
        {
            std::lock_guard<std::mutex> lock(stats_lock_);
            latency_samples_ms_.push_back(latency);
        }
        
        frames_processed_.fetch_add(1);
    }
    
    std::cout << "[Processing Loop] Stopped" << std::endl;
}

} // namespace turbopalmtree

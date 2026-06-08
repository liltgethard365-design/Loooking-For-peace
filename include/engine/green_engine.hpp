#pragma once

#include "engine/types.hpp"
#include "engine/ring_buffer.hpp"
#include "engine/worker_thread.hpp"
#include "vision/robust_meter_detector.hpp"

#include <memory>
#include <atomic>
#include <mutex>
#include <optional>

namespace turbopalmtree {

class GreenEngine {
public:
    explicit GreenEngine(const EngineConfig& config = EngineConfig());
    ~GreenEngine();
    
    void start();
    void stop();
    bool is_running() const { return running_.load(); }
    
    void process_frame(const FrameBuffer& frame);
    
    void set_state(EngineState state);
    EngineState get_state() const;
    
    std::optional<MeterReading> get_latest_meter_reading() const;
    
    LatencyStats get_latency_stats() const;
    uint64_t get_frames_processed() const { return frames_processed_.load(); }
    uint64_t get_frames_dropped() const { return frames_dropped_.load(); }
    void reset_latency_stats();
    void print_diagnostics() const;
    
private:
    void processing_loop();
    
    EngineConfig config_;
    std::atomic<EngineState> state_{EngineState::IDLE};
    std::atomic<bool> running_{false};
    
    std::unique_ptr<RingBuffer<FrameBuffer>> frame_queue_;
    std::unique_ptr<WorkerThread> vision_thread_;
    std::unique_ptr<RobustMeterDetector> meter_detector_;
    
    mutable std::mutex vision_lock_;
    std::optional<MeterReading> latest_reading_;
    
    std::vector<double> latency_samples_ms_;
    mutable std::mutex stats_lock_;
    
    std::atomic<uint64_t> frames_processed_{0};
    std::atomic<uint64_t> frames_dropped_{0};
};

} // namespace turbopalmtree

#include "engine/green_engine.hpp"
#include "benchmarks/latency_benchmark.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace turbopalmtree;
using namespace std::chrono_literals;

int main() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "  TurboPalmTree - Latency Benchmarking Utility" << std::endl;
    std::cout << "  Real-time Console Automation System" << std::endl;
    std::cout << std::string(70, '=') << std::endl << std::endl;
    
    EngineConfig config;
    config.enable_cpu_pinning = true;
    config.vision_worker_core = 0;
    
    std::cout << "[Setup] Initializing GreenEngine..." << std::endl;
    GreenEngine engine(config);
    engine.start();
    engine.set_state(EngineState::AIMING);
    
    std::cout << "[Setup] Engine ready. Injecting 1000 test frames...\n" << std::endl;
    
    const uint32_t FRAME_WIDTH = 1920;
    const uint32_t FRAME_HEIGHT = 1080;
    const uint8_t BYTES_PER_PIXEL = 3;
    const size_t FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT * BYTES_PER_PIXEL;
    
    uint8_t* frame_buffer = new uint8_t[FRAME_SIZE]();
    
    // Simulate green pixels in center region
    for (int y = 500; y < 600; ++y) {
        for (int x = 900; x < 1000; ++x) {
            size_t idx = (y * FRAME_WIDTH + x) * BYTES_PER_PIXEL;
            frame_buffer[idx] = 50;      // B
            frame_buffer[idx + 1] = 200; // G
            frame_buffer[idx + 2] = 50;  // R
        }
    }
    
    const int NUM_FRAMES = 1000;
    const int REPORT_INTERVAL = 100;
    
    auto injection_start = HighResTimer::now();
    
    for (int i = 0; i < NUM_FRAMES; ++i) {
        auto capture_time = HighResTimer::now();
        
        FrameBuffer fb{
            .data = frame_buffer,
            .size = FRAME_SIZE,
            .width = FRAME_WIDTH,
            .height = FRAME_HEIGHT,
            .bytes_per_pixel = BYTES_PER_PIXEL,
            .metadata = {
                .frame_id = (uint64_t)i,
                .capture_timestamp = capture_time,
                .vision_start = {},
                .vision_complete = {},
                .logic_start = {},
                .logic_complete = {},
                .injection_timestamp = {}
            }
        };
        
        engine.process_frame(fb);
        std::this_thread::sleep_for(1ms);
        
        if ((i + 1) % REPORT_INTERVAL == 0) {
            std::cout << "  [" << (i + 1) << "/" << NUM_FRAMES << "] Frames injected..." << std::endl;
        }
    }
    
    auto injection_end = HighResTimer::now();
    
    std::cout << "\n[Processing] Waiting for completion..." << std::endl;
    std::this_thread::sleep_for(1s);
    
    auto stats = engine.get_latency_stats();
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "  LATENCY ANALYSIS RESULTS" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    if (stats.sample_count == 0) {
        std::cout << "  ✗ No samples collected! Check engine initialization." << std::endl;
    } else {
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "\n  Statistics:" << std::endl;
        std::cout << "    Mean:       " << std::setw(8) << stats.mean_ms << " ms" << std::endl;
        std::cout << "    Median:     " << std::setw(8) << stats.median_ms << " ms" << std::endl;
        std::cout << "    Std Dev:    " << std::setw(8) << stats.stddev_ms << " ms" << std::endl;
        std::cout << "    Min/Max:    " << std::setw(8) << stats.min_ms << " / " 
                  << std::setw(8) << stats.max_ms << " ms" << std::endl;
        std::cout << "    P95:        " << std::setw(8) << stats.p95_ms << " ms";
        if (stats.passes_threshold) {
            std::cout << " ✓ PASS" << std::endl;
        } else {
            std::cout << " ✗ FAIL (target: <20ms)" << std::endl;
        }
        std::cout << "    P99:        " << std::setw(8) << stats.p99_ms << " ms" << std::endl;
        std::cout << "    Samples:    " << stats.sample_count << std::endl;
    }
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "  ASSESSMENT" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    if (stats.sample_count == 0) {
        std::cout << "\n  Issue: Engine not processing frames." << std::endl;
        std::cout << "  Check: Vision detector, frame delivery, state machine." << std::endl;
    } else if (!stats.passes_threshold) {
        std::cout << "\n  ⚠️  P95 latency exceeds 20ms threshold (" << stats.p95_ms << "ms)" << std::endl;
        std::cout << "  Next steps:" << std::endl;
        std::cout << "    1. Verify CPU pinning is active" << std::endl;
        std::cout << "    2. Optimize frame buffer handling" << std::endl;
        std::cout << "    3. Profile vision detector performance" << std::endl;
    } else {
        std::cout << "\n  ✓ System latency within acceptable range!" << std::endl;
        std::cout << "  Ready to: Calibrate detector, integrate capture card" << std::endl;
    }
    
    std::cout << std::string(70, '=') << std::endl << std::endl;
    
    engine.stop();
    delete[] frame_buffer;
    
    return stats.passes_threshold ? 0 : 1;
}

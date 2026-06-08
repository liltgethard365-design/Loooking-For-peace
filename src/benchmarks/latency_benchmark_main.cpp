#include "engine/green_engine.hpp"
#include "benchmarks/latency_benchmark.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>

using namespace turbopalmtree;
using namespace std::chrono_literals;

void run_standard_benchmark(GreenEngine& engine, const uint8_t* frame_buffer, 
                           size_t frame_size, uint32_t width, uint32_t height) {
    std::cout << "[Benchmark] Running standard latency test (1000 frames)...\n" << std::endl;
    
    const int NUM_FRAMES = 1000;
    const int REPORT_INTERVAL = 100;
    
    for (int i = 0; i < NUM_FRAMES; ++i) {
        auto capture_time = HighResTimer::now();
        
        FrameBuffer fb{
            .data = frame_buffer,
            .size = frame_size,
            .width = width,
            .height = height,
            .bytes_per_pixel = 3,
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
}

void run_stress_test(GreenEngine& engine, const uint8_t* frame_buffer,
                    size_t frame_size, uint32_t width, uint32_t height, int target_fps) {
    std::cout << "[Stress Test] Running at " << target_fps << " FPS (500 frames)...\n" << std::endl;
    
    const int NUM_FRAMES = 500;
    const double frame_interval_ms = 1000.0 / target_fps;
    int dropped = 0;
    
    auto test_start = HighResTimer::now();
    
    for (int i = 0; i < NUM_FRAMES; ++i) {
        auto frame_start = HighResTimer::now();
        
        FrameBuffer fb{
            .data = frame_buffer,
            .size = frame_size,
            .width = width,
            .height = height,
            .bytes_per_pixel = 3,
            .metadata = {
                .frame_id = (uint64_t)(1000 + i),
                .capture_timestamp = frame_start,
                .vision_start = {},
                .vision_complete = {},
                .logic_start = {},
                .logic_complete = {},
                .injection_timestamp = {}
            }
        };
        
        engine.process_frame(fb);
        
        // Maintain frame rate timing
        auto elapsed = std::chrono::duration<double, std::milli>(
            HighResTimer::now() - frame_start).count();
        auto sleep_time = frame_interval_ms - elapsed;
        
        if (sleep_time > 0) {
            std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(sleep_time));
        }
        
        if ((i + 1) % 50 == 0) {
            std::cout << "  [" << (i + 1) << "/" << NUM_FRAMES << "] Frames at " 
                      << target_fps << " FPS..." << std::endl;
        }
    }
    
    auto test_duration = std::chrono::duration<double>(
        HighResTimer::now() - test_start).count();
    
    std::cout << "\n  Total time: " << std::fixed << std::setprecision(2) 
              << test_duration << " seconds" << std::endl;
}

int main() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "  TurboPalmTree - Advanced Latency Benchmarking" << std::endl;
    std::cout << "  With Jitter Analysis & Stress Testing" << std::endl;
    std::cout << std::string(70, '=') << std::endl << std::endl;
    
    EngineConfig config;
    config.enable_cpu_pinning = true;
    config.vision_worker_core = 0;
    
    std::cout << "[Setup] Initializing GreenEngine..." << std::endl;
    GreenEngine engine(config);
    engine.start();
    engine.set_state(EngineState::AIMING);
    
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
    
    std::cout << "[Setup] Engine ready. Beginning tests...\n" << std::endl;
    
    // ============ STANDARD BENCHMARK ============
    LatencyBenchmark standard_bench("Standard Latency Test");
    
    run_standard_benchmark(engine, frame_buffer, FRAME_SIZE, FRAME_WIDTH, FRAME_HEIGHT);
    
    std::cout << "\n[Processing] Waiting for completion..." << std::endl;
    std::this_thread::sleep_for(1s);
    
    // Record samples
    auto stats = engine.get_latency_stats();
    for (uint64_t i = 0; i < stats.sample_count; ++i) {
        // Stats already recorded in engine; just display
    }
    
    // Print results
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "  TEST 1: STANDARD BENCHMARK (1000 FRAMES)" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    if (stats.sample_count == 0) {
        std::cout << "  ✗ No samples collected! Check engine initialization." << std::endl;
    } else {\n        std::cout << std::fixed << std::setprecision(3);
        std::cout << "\n  Overall Statistics:" << std::endl;
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
    
    // Reset for next test
    engine.reset_latency_stats();
    
    // ============ STRESS TEST (60 FPS) ============
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "  TEST 2: STRESS TEST (60 FPS)" << std::endl;
    std::cout << std::string(70, '=') << std::endl << std::endl;
    
    run_stress_test(engine, frame_buffer, FRAME_SIZE, FRAME_WIDTH, FRAME_HEIGHT, 60);
    
    std::cout << "\n[Processing] Waiting for stress test completion..." << std::endl;
    std::this_thread::sleep_for(1s);
    
    auto stress_stats = engine.get_latency_stats();
    
    if (stress_stats.sample_count > 0) {
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "\n  Stress Test Results (60 FPS):" << std::endl;
        std::cout << "    Mean:       " << std::setw(8) << stress_stats.mean_ms << " ms" << std::endl;
        std::cout << "    P95:        " << std::setw(8) << stress_stats.p95_ms << " ms";
        if (stress_stats.passes_threshold) {
            std::cout << " ✓ Stable" << std::endl;
        } else {
            std::cout << " ✗ Unstable" << std::endl;
        }
        std::cout << "    Samples:    " << stress_stats.sample_count << std::endl;
    }
    
    // ============ FINAL ASSESSMENT ============
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "  FINAL ASSESSMENT" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    if (stats.sample_count == 0) {
        std::cout << "\n  ✗ CRITICAL: Engine not processing frames." << std::endl;
        std::cout << "    Check: Vision detector, frame delivery, state machine." << std::endl;
    } else if (!stats.passes_threshold) {
        std::cout << "\n  ⚠️  P95 latency exceeds 20ms threshold (" << stats.p95_ms << "ms)." << std::endl;
        std::cout << "    Recommended optimizations:" << std::endl;
        std::cout << "      1. Verify CPU pinning is active" << std::endl;
        std::cout << "      2. Profile which component is slow:" << std::endl;
        std::cout << "         - Vision detector scanning speed" << std::endl;
        std::cout << "         - Logic decision time" << std::endl;
        std::cout << "         - Input synthesis/injection time" << std::endl;
        std::cout << "      3. Reduce frame resolution or detection ROI" << std::endl;
        std::cout << "      4. Check for lock contention in ring buffer" << std::endl;
    } else {
        std::cout << "\n  ✓ System latency EXCELLENT (" << stats.p95_ms << "ms)." << std::endl;
        std::cout << "    Ready to:" << std::endl;
        std::cout << "      • Integrate actual capture card hardware" << std::endl;
        std::cout << "      • Calibrate meter detector on real game footage" << std::endl;
        std::cout << "      • Run full end-to-end testing" << std::endl;
    }
    
    if (stress_stats.sample_count > 0 && !stress_stats.passes_threshold) {
        std::cout << "\n  ⚠️  System unstable at 60 FPS. Latency increased under load." << std::endl;
        std::cout << "    Consider reducing processing load or optimizing components." << std::endl;
    }
    
    std::cout << std::string(70, '=') << std::endl << std::endl;
    
    // Cleanup
    engine.stop();
    delete[] frame_buffer;
    
    return stats.passes_threshold ? 0 : 1;
}

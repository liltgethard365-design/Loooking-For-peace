#include "benchmarks/latency_benchmark.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>

namespace turbopalmtree {

LatencyBenchmark::LatencyBenchmark(const std::string& name)
    : benchmark_name_(name) {}

void LatencyBenchmark::record_sample(const FrameMetadata& metadata) {
    samples_ms_.push_back(metadata.total_latency_ms());
}

LatencyStats LatencyBenchmark::get_stats() const {
    if (samples_ms_.empty()) {
        return {0, 0, 0, 0, 0, 0, 0, 0, 0, false};
    }
    
    auto sorted = samples_ms_;
    std::sort(sorted.begin(), sorted.end());
    
    double sum = 0, sum_sq = 0;
    for (auto sample : sorted) {
        sum += sample;
        sum_sq += sample * sample;
    }
    
    double mean = sum / sorted.size();
    double variance = (sum_sq / sorted.size()) - (mean * mean);
    double stddev = std::sqrt(std::max(0.0, variance));
    
    size_t p50_idx = sorted.size() / 2;
    size_t p95_idx = std::min((size_t)(sorted.size() * 0.95), sorted.size() - 1);
    size_t p99_idx = std::min((size_t)(sorted.size() * 0.99), sorted.size() - 1);
    
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
        .passes_threshold = (sorted[p95_idx] <= 20.0)
    };
}

void LatencyBenchmark::print_report() const {
    auto stats = get_stats();
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "  " << benchmark_name_ << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "  Mean:               " << std::setw(8) << stats.mean_ms << " ms" << std::endl;
    std::cout << "  Median:             " << std::setw(8) << stats.median_ms << " ms" << std::endl;
    std::cout << "  Std Dev:            " << std::setw(8) << stats.stddev_ms << " ms" << std::endl;
    std::cout << "  Min/Max:            " << std::setw(8) << stats.min_ms << " / " 
              << std::setw(8) << stats.max_ms << " ms" << std::endl;
    std::cout << "  " << std::string(56, '-') << std::endl;
    std::cout << "  P95:                " << std::setw(8) << stats.p95_ms << " ms";
    if (stats.passes_threshold) {
        std::cout << " ✓" << std::endl;
    } else {
        std::cout << " ✗" << std::endl;
    }
    std::cout << "  P99:                " << std::setw(8) << stats.p99_ms << " ms" << std::endl;
    std::cout << "  Samples:            " << stats.sample_count << std::endl;
    std::cout << std::string(60, '=') << std::endl << std::endl;
}

void LatencyBenchmark::reset() {
    samples_ms_.clear();
}

} // namespace turbopalmtree

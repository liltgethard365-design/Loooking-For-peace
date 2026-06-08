#pragma once

#include "engine/types.hpp"
#include <vector>
#include <string>

namespace turbopalmtree {

class LatencyBenchmark {
public:
    explicit LatencyBenchmark(const std::string& name = "Latency Benchmark");
    
    void record_sample(const FrameMetadata& metadata);
    LatencyStats get_stats() const;
    void print_report() const;
    void reset();
    size_t sample_count() const { return samples_ms_.size(); }
    
private:
    std::string benchmark_name_;
    std::vector<double> samples_ms_;
};

} // namespace turbopalmtree

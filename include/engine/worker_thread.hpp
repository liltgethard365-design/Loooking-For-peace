#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <string>

namespace turbopalmtree {

class WorkerThread {
public:
    explicit WorkerThread(std::function<void()> task, const std::string& name = "Worker");
    ~WorkerThread();
    
    void start();
    void stop();
    bool is_running() const { return running_.load(); }
    bool pin_to_core(int core_id);
    
private:
    std::function<void()> task_;
    std::string name_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

} // namespace turbopalmtree

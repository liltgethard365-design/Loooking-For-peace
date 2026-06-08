#include "engine/worker_thread.hpp"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h>
#endif

namespace turbopalmtree {

WorkerThread::WorkerThread(std::function<void()> task, const std::string& name)
    : task_(task), name_(name) {}

WorkerThread::~WorkerThread() {
    stop();
}

void WorkerThread::start() {
    if (running_.exchange(true)) return;
    thread_ = std::thread([this]() { task_(); });
}

void WorkerThread::stop() {
    if (!running_.exchange(false)) return;
    if (thread_.joinable()) thread_.join();
}

bool WorkerThread::pin_to_core(int core_id) {
    if (!thread_.joinable()) {
        std::cerr << "[" << name_ << "] Cannot pin: thread not running" << std::endl;
        return false;
    }
    
#ifdef _WIN32
    HANDLE handle = (HANDLE)thread_.native_handle();
    if (!handle) return false;
    
    DWORD_PTR mask = 1ULL << core_id;
    if (SetThreadAffinityMask(handle, mask)) {
        std::cout << "[" << name_ << "] Pinned to CPU core " << core_id << std::endl;
        return true;
    } else {
        std::cerr << "[" << name_ << "] Failed to pin to core " << core_id << std::endl;
        return false;
    }
#elif defined(__unix__) || defined(__APPLE__)
    pthread_t handle = thread_.native_handle();
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    
    int result = pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset);
    if (result == 0) {
        std::cout << "[" << name_ << "] Pinned to CPU core " << core_id << std::endl;
        return true;
    } else {
        std::cerr << "[" << name_ << "] Failed to pin to core " << core_id << std::endl;
        return false;
    }
#else
    std::cerr << "[" << name_ << "] CPU pinning not supported" << std::endl;
    return false;
#endif
}

} // namespace turbopalmtree

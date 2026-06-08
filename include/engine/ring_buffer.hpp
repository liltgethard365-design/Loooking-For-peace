#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace turbopalmtree {

template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity) : capacity_(capacity), buffer_(capacity) {}
    
    bool try_push(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (size_ >= capacity_) return false;
        buffer_[write_idx_] = item;
        write_idx_ = (write_idx_ + 1) % capacity_;
        size_++;
        cv_.notify_one();
        return true;
    }
    
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (size_ == 0) return std::nullopt;
        T item = buffer_[read_idx_];
        read_idx_ = (read_idx_ + 1) % capacity_;
        size_--;
        return item;
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_;
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_ == 0;
    }
    
private:
    size_t capacity_;
    std::vector<T> buffer_;
    size_t read_idx_ = 0;
    size_t write_idx_ = 0;
    size_t size_ = 0;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace turbopalmtree

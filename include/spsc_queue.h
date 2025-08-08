#pragma once

#include <atomic>
#include <cstddef>
#include <type_traits>

template<typename T, size_t Capacity>
class SPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

public:
    SPSCQueue() : head_(0), tail_(0) {
        buffer_ = new T[Capacity];
    }

    ~SPSCQueue() {
        delete buffer_;
    }

    // Producer thread
    bool push(const T& item) {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t next_head = (head + 1) & index_mask;

        if (next_head == tail_.load(std::memory_order_acquire)) {
            return false; // full
        }

        buffer_[head] = item;
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    bool push(T&& item) {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t next_head = (head + 1) & index_mask;

        if (next_head == tail_.load(std::memory_order_acquire)) {
            return false; // full
        }

        buffer_[head] = std::move(item);
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    // Consumer thread
    bool pop(T& item) {
        const size_t tail = tail_.load(std::memory_order_relaxed);

        if (tail == head_.load(std::memory_order_acquire)) {
            return false; // empty
        }

        item = std::move(buffer_[tail]);
        tail_.store((tail + 1) & index_mask, std::memory_order_release);
        return true;
    }

    bool empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    bool full() const {
        const size_t next_head = (head_.load(std::memory_order_relaxed) + 1) & index_mask;
        return next_head == tail_.load(std::memory_order_acquire);
    }

private:
    static constexpr size_t index_mask = Capacity - 1;
    T* buffer_;
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
};

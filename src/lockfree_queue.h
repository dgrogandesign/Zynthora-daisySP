#include <cstddef>
#ifndef ZYNTHORA_QUEUE_H
#define ZYNTHORA_QUEUE_H

#include <atomic>
#include <vector>
#include "events.h"

// ============================================================================
// LOCK-FREE RING BUFFER (SPSC)
// Single-Producer (Main Thread/BLE) -> Single-Consumer (Audio Thread)
// ============================================================================

class EventQueue {
public:
    // Initialize with a power-of-two size (e.g., 256 events)
    EventQueue(size_t size = 256) : size_(size), head_(0), tail_(0) {
        buffer_.resize(size);
    }

    // PUSH: Called by Main Thread (WiFi/BLE)
    // Returns false if queue is full
    bool push(const SynthEvent& event) {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % size_;

        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Queue is full! Drop event (or handle overflow)
        }

        buffer_[current_tail] = event;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    // POP: Called by Audio Thread (DSP)
    // Returns false if queue is empty
    bool pop(SynthEvent& event) {
        size_t current_head = head_.load(std::memory_order_relaxed);

        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false; // Queue is empty
        }

        event = buffer_[current_head];
        head_.store((current_head + 1) % size_, std::memory_order_release);
        return true;
    }

private:
    std::vector<SynthEvent> buffer_;
    size_t size_;
    std::atomic<size_t> head_; // Read index
    std::atomic<size_t> tail_; // Write index
};

#endif // ZYNTHORA_QUEUE_H

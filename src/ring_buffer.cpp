#include "ring_buffer.h"

// At the end of the file, add explicit instantiation for EventPtr
template class LockFreeRingBuffer<EventPtr>;

template <typename T, size_t Size>
LockFreeRingBuffer<T, Size>::LockFreeRingBuffer()
    : capacity_(Size), buffer_(new T[Size]), read_index_(0), write_index_(0), seqLock(0) {
    for (size_t i = 0; i < capacity_; ++i) {
        buffer_[i] = nullptr;
    }
}

template <typename T, size_t Size>
LockFreeRingBuffer<T, Size>::~LockFreeRingBuffer() {
    for (size_t i = 0; i < capacity_; ++i) {
        if (T evt = buffer_[i]) {
            delete evt;
        }
    }
    delete[] buffer_;
}

template <typename T, size_t Size>
bool LockFreeRingBuffer<T, Size>::Push(T event) {
    size_t current_write;
    size_t next_write;
    do {
        current_write = write_index_.load(std::memory_order_relaxed);
        next_write = (current_write + 1) % capacity_;

        // Check if buffer is full
        if (next_write == read_index_.load(std::memory_order_acquire)) {
            return false;
        }
    } while (!write_index_.compare_exchange_weak(current_write, next_write, std::memory_order_acq_rel,
                                                 std::memory_order_relaxed));

    if (current_write >= capacity_) {
        return false;
    }

    buffer_[current_write] = event;
    return true;
}

template <typename T, size_t Size>
T LockFreeRingBuffer<T, Size>::Pop() {
    while (true) {
        size_t current_read = read_index_.load(std::memory_order_relaxed);
        size_t current_write = write_index_.load(std::memory_order_acquire);

        if (current_read == current_write) {
            return nullptr;  // Buffer is empty
        }

        if (current_read >= capacity_) {
            return nullptr;
        }

        T event = buffer_[current_read];
        buffer_[current_read] = nullptr;

        size_t next_read = (current_read + 1) % capacity_;
        if (read_index_.compare_exchange_strong(current_read, next_read, std::memory_order_release,
                                                std::memory_order_relaxed)) {
            return event;
        }
    }
}

template <typename T, size_t Size>
std::optional<std::pair<size_t, size_t>> LockFreeRingBuffer<T, Size>::TryReserveSpace(size_t requested) {
    if (!requested) {
        return std::nullopt;
    }

    // Try to acquire sequence lock
    size_t expected = 0;
    if (!seqLock.compare_exchange_weak(expected, 1, std::memory_order_acquire, std::memory_order_relaxed)) {
        return std::nullopt;
    }

    // RAII guard for sequence lock
    struct LockGuard {
        std::atomic<size_t>& lock;
        ~LockGuard() { lock.store(0, std::memory_order_release); }
    } guard{seqLock};

    size_t current_write = write_index_.load(std::memory_order_acquire);
    size_t current_read = read_index_.load(std::memory_order_acquire);

    // Calculate available space based on buffer state
    size_t free_space;
    if (current_write >= current_read) {
        // Try space at end of buffer first
        size_t to_end = capacity_ - current_write;
        if (to_end > 0) {
            free_space = std::min(to_end, requested);
            write_index_.store(current_write + free_space, std::memory_order_release);
            return std::make_pair(free_space, current_write);
        }

        // Try wrap around if possible
        if (current_read > 1) {
            free_space = std::min(current_read - 1, requested);
            if (free_space > 0) {
                write_index_.store(free_space, std::memory_order_release);
                return std::make_pair(free_space, 0);
            }
        }
    } else {
        // Space available between write and read
        free_space = std::min(current_read - current_write - 1, requested);
        if (free_space > 0) {
            write_index_.store(current_write + free_space, std::memory_order_release);
            return std::make_pair(free_space, current_write);
        }
    }

    return std::nullopt;
}

template <typename T, size_t Size>
size_t LockFreeRingBuffer<T, Size>::FreeSpace() const {
    size_t r = read_index_.load(std::memory_order_acquire);
    size_t w = write_index_.load(std::memory_order_acquire);

    if (r > capacity_ || w > capacity_) {
        return 0;
    }

    return w >= r ? capacity_ - (w - r) - 1 : r - w - 1;
}

template <typename T, size_t Size>
bool LockFreeRingBuffer<T, Size>::IsEmpty() const {
    return read_index_.load(std::memory_order_acquire) == write_index_.load(std::memory_order_acquire);
}
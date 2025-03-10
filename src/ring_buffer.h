#pragma once
#include "types.h"
#include <atomic>
#include <cstddef>
#include <new>
#include <cassert>
#include <optional>

/**
 * @brief Lock-free ring buffer implementation for concurrent producer-consumer scenarios
 * @tparam T Type of elements stored in the buffer
 * @tparam Size Buffer capacity, defaults to 4096
 */
template <typename T, size_t Size = 4096>
class LockFreeRingBuffer {
   public:
    explicit LockFreeRingBuffer();

    ~LockFreeRingBuffer();

    // Delete copy and move operations
    LockFreeRingBuffer(const LockFreeRingBuffer&) = delete;
    LockFreeRingBuffer& operator=(const LockFreeRingBuffer&) = delete;
    LockFreeRingBuffer(LockFreeRingBuffer&&) = delete;
    LockFreeRingBuffer& operator=(LockFreeRingBuffer&&) = delete;

    /**
     * @brief Attempts to push an event into the buffer
     * @param event Event to push
     * @return true if successful, false if buffer is full
     */
    bool Push(T event);

    /**
     * @brief Attempts to pop an event from the buffer
     * @return Event pointer if available, nullptr if empty
     */
    T Pop();

    /**
     * @brief Attempts to reserve contiguous space in the buffer
     * @param requested Number of slots requested
     * @return Optional pair of {reserved size, start index}, or nullopt if failed
     */
    std::optional<std::pair<size_t, size_t>> TryReserveSpace(size_t requested);

    /**
     * @brief Get available space in the buffer
     * @return Number of free slots
     */
    size_t FreeSpace() const;

    /**
     * @brief Check if buffer is empty
     * @return true if empty, false otherwise
     */
    bool IsEmpty() const;

   private:
    const size_t capacity_;            // Fixed buffer capacity
    T* buffer_;                        // Actual storage
    std::atomic<size_t> read_index_;   // Current read position
    std::atomic<size_t> write_index_;  // Current write position
    std::atomic<size_t> seqLock;       // Sequence lock for reservations
};

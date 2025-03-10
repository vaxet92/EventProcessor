#pragma once

#include "types.h"

#include <memory>
#include <queue>
#include <vector>
#include <iostream>
#include "ring_buffer.h"

class IEventProcessor {
   public:
    using Integer = int64_t;

    std::atomic<size_t> active_writers;

    explicit IEventProcessor(const size_t count)
        : active_writers{count},
          reserved_events_index_{0},
          ring_buffer_(std::make_unique<LockFreeRingBuffer<EventPtr>>()) {}

    void ProcessEvents() {
        while (active_writers.load() > 0 || !ring_buffer_->IsEmpty()) {
            if (EventPtr event = ring_buffer_->Pop()) {
                event->Process();
                delete event;
            }
        }
    }

    struct ReservedEvent {
        ReservedEvent() : sequence_number_(0), event_(nullptr) {}
        explicit ReservedEvent(const Integer sequence_number, void* const event)
            : sequence_number_(sequence_number), event_(event) {}

        ReservedEvent(const ReservedEvent&) = delete;
        ReservedEvent& operator=(const ReservedEvent&) = delete;

        ReservedEvent(ReservedEvent&&) = delete;
        ReservedEvent& operator=(ReservedEvent&&) = delete;

        Integer GetSequenceNumber() const { return sequence_number_; }
        void* GetEvent() const { return event_; }

        bool IsValid() const { return event_ != nullptr; }

       private:
        Integer sequence_number_;
        void* event_;
    };

    struct ReservedEvents {
        ReservedEvents(const Integer sequence_number, Event** const event, const size_t count,
                       const size_t event_size = sizeof(IEvent))
            : sequence_number_(sequence_number), events_(event), count_(count), event_size_(event_size) {}

        ~ReservedEvents() {
#ifdef DEBUG
            std::cout << "ReservedEvents destroyed" << std::endl;
#endif
            delete[] events_;
        }

        ReservedEvents(const ReservedEvents&) = delete;
        ReservedEvents& operator=(const ReservedEvents&) = delete;

        ReservedEvents(ReservedEvents&&) = delete;
        ReservedEvents& operator=(ReservedEvents&&) = delete;

        template <class TEvent, class... Args>
        void Emplace(const size_t index, Args&&... args) {
            // Use placement new to construct the Event
            if (index >= count_) {
#ifdef DEBUG
                std::cerr << "Emplace index overflow: " << index << " >= " << count_ << std::endl;
#endif
                return;
            }
            new (events_[index]) TEvent(std::forward<Args>(args)...);
#ifdef DEBUG
            std::cout << "Emplaced event at index " << index << std::endl;
#endif
        }

        Integer GetSequenceNumber() const { return sequence_number_; }

        Event* GetEvent(const size_t index) const {
            if (index >= count_) {
#ifdef DEBUG
                std::cout << " GetEvent index overflow " << " " << index << std::endl;
#endif
                return nullptr;
            }
            auto* ptr = static_cast<Event*>(events_[index]);
            return ptr;
        }

        size_t Count() const { return count_; }
        bool IsValid() const { return events_ != nullptr; }

       private:
        Integer sequence_number_;
        Event** events_;
        size_t count_;
        size_t event_size_;
    };

    // try to reserve T*
    template <typename T>
    std::pair<size_t, void* const> ReserveEvent() {
        if (size_t count = Count(); count > 0) {
            reserved_events_index_.fetch_add(1);
            // Allocate raw memory without constructing
            auto* ptr = operator new(sizeof(T));
            return std::make_pair(reserved_events_index_, ptr);
        }
        return std::make_pair(0, nullptr);
    }

    template <class T, class... Args>
    ReservedEvent Reserve(Args&&... args) {
        const auto reservation = ReserveEvent<T>();
        if (!reservation.second) return ReservedEvent();

        // Construct the object in the reserved memory
        auto* typed_ptr = static_cast<T*>(reservation.second);
        std::construct_at(typed_ptr, std::forward<Args>(args)...);
        return ReservedEvent(reservation.first, reservation.second);
    }

    std::optional<size_t> ReserveRange(const size_t size) {
        // Check ring buffer space first
        auto res = ring_buffer_->TryReserveSpace(size);
        if (!res.has_value() || res.value().first == 0) {
#ifdef DEBUG
            std::cerr << "Ring buffer space reservation failed" << std::endl;
#endif
            return std::nullopt;
        }
        auto [reserved_size, start_index] = res.value();

        // Allocate array of pointers
        auto** ptr = new Event*[reserved_size];
        // Allocate memory for each Event
        for (size_t i = 0; i < reserved_size; ++i) {
            ptr[i] = static_cast<Event*>(operator new(sizeof(Event)));
        }

        auto index = reserved_events_index_.fetch_add(1) % reserved_events_.size();

        // Use incrSize for both allocation and ReservedEvents construction
        reserved_events_[index] = std::make_unique<ReservedEvents>(start_index, ptr, reserved_size, sizeof(Event));
        return index;
    }

    void Commit(const size_t index, const Integer sequence_number, const size_t count) {
        if (index >= reserved_events_.size()) {
#ifdef DEBUG
            std::cerr << "Commit index overflow: " << index << " >= " << reserved_events_.size() << std::endl;
#endif
            return;
        }
        auto& eventsRef = reserved_events_[index];
        if (!eventsRef || count > eventsRef->Count()) return;  // Add size check

        for (size_t i = 0; i < count; ++i) {
            auto* eventPtr = eventsRef->GetEvent(i);
            if (eventPtr) {
                ring_buffer_->Push(eventPtr);
                eventPtr = nullptr;
            }
        }
        eventsRef.reset(nullptr);

#ifdef DEBUG
        std::cout << "Commit " << sequence_number << " " << count << std::endl;
#endif
    }

    ReservedEvents* GetReservedEvents(const size_t index) {
        if (index >= reserved_events_.size()) {
            return nullptr;
        }
        return reserved_events_[index].get();
    }

   private:
    size_t Count() const { return ring_buffer_->FreeSpace(); }

    std::array<std::unique_ptr<ReservedEvents>, 32> reserved_events_;
    std::atomic<std::size_t> reserved_events_index_;
    std::unique_ptr<LockFreeRingBuffer<EventPtr>> ring_buffer_;
};

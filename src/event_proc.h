#pragma once

#include "types.h"
#include <condition_variable>
#include <memory>
#include <queue>
#include <vector>
#include <iostream>

class IEvent {
   public:
    virtual ~IEvent() = default;
    virtual void Process() = 0;
};

class Event : public IEvent {
   public:
    explicit Event(const int value) : value_(value) {}
    // ~Event() override { std::cout << "Event " << value_ << " destroyed" << std::endl; }

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
    Event(Event&&) = delete;
    Event& operator=(Event&&) = delete;

    virtual void Process() override { std::cout << "Event " << value_ << std::endl; }

   private:
    int value_;
};

// using EventPtr = std::unique_ptr<Event>;
using EventPtr = Event*;
using Container = std::queue<EventPtr>;

class IEventProcessor {
   public:
    using Integer = int64_t;

    void PushEvent(EventPtr event) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        event_queue_.push(event);
    }

    EventPtr PopEvent() {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait(lock, [this]() { return !event_queue_.empty(); });
        if (event_queue_.empty()) {
            return nullptr;
        }
        EventPtr event = event_queue_.front();
        event_queue_.pop();
        return event;
    }

    void ProcessEvents(std::atomic<size_t>& active_writers) {
        // std::unique_lock<std::mutex> lock(queue_mutex_);
        // queue_cv_.wait(lock, [this]() { return !event_queue_.empty(); });
        while (active_writers.load() > 0) {
            std::unique_ptr<Event> event(PopEvent());
            if (event) {
                event->Process();
            }
        }
    }

    struct ReservedEvent {
        ReservedEvent() : sequence_number_(0), event_(nullptr) {}
        explicit ReservedEvent(const Integer sequence_number, void* const event) : sequence_number_(sequence_number), event_(event) {}

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
        ReservedEvents(const Integer sequence_number, Event** const event, const size_t count, const size_t event_size = sizeof(IEvent))
            : sequence_number_(sequence_number), events_(event), count_(count), event_size_(event_size) {}

        ~ReservedEvents() {
            std::cout << "ReservedEvents destroyed" << std::endl;
            delete[] events_;
        }

        ReservedEvents(const ReservedEvents&) = delete;
        ReservedEvents& operator=(const ReservedEvents&) = delete;

        ReservedEvents(ReservedEvents&&) = delete;
        ReservedEvents& operator=(ReservedEvents&&) = delete;

        template <class TEvent, class... Args>
        void Emplace(const size_t index, Args&&... args) {
            // Use placement new to construct the Event
            new (events_[index]) TEvent(std::forward<Args>(args)...);
        }

        Integer GetSequenceNumber() const { return sequence_number_; }

        Event* GetEvent(const size_t index) const {
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
        if (size_t count = Count(); count < max_capacity_) {
            ++count_;
            // Allocate raw memory without constructing
            auto* ptr = operator new(sizeof(T));
            return std::make_pair(count_, ptr);
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
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (count_ + size <= max_capacity_) {
            Integer sequence_number = count_ + 1;

            // Allocate array of pointers
            auto** ptr = new Event*[size];
            // Allocate memory for each Event
            for (size_t i = 0; i < size; ++i) {
                ptr[i] = static_cast<Event*>(operator new(sizeof(Event)));
            }

            count_ += size;
            auto index = reserved_events_index_.fetch_add(1) % reserved_events_.size();
            reserved_events_[index] = std::make_unique<ReservedEvents>(sequence_number, ptr, size, sizeof(Event));
            return index;
        }
        return std::nullopt;
    }

    // void Commit(const Integer sequence_number) {
    //     EventPtr event(static_cast<Event*>(GetEvent(sequence_number)));
    //     if (event) {
    //         PushEvent(std::move(reserved_events_[index]->GetEvent(sequence_number)));
    //         queue_cv_.notify_one();
    //         std::cout << "Commit " << sequence_number << std::endl;
    //     }
    // }

    void Commit(const size_t index, const Integer sequence_number, const size_t count) {
        auto& eventsRef = reserved_events_[index];
        if (!eventsRef) return;

        for (size_t i = 0; i < count; ++i) {
            // Create a new Event and transfer ownership of the memory
            auto* eventPtr = eventsRef->GetEvent(i);
            if (eventPtr) {
                // Take ownership of the event without deleting it
                // auto* event = static_cast<Event*>(eventPtr);
                PushEvent(eventPtr);
                eventPtr = nullptr;

                // Set the pointer to nullptr to avoid double deletion
            }
        }
        eventsRef.reset(nullptr);

        queue_cv_.notify_one();
        std::cout << "Commit " << sequence_number << " " << count << std::endl;
    }

    ReservedEvents* GetReservedEvents(const size_t index) { return reserved_events_[index].get(); }

   private:
    size_t Count() const { return event_queue_.size(); }

    Container event_queue_;
    size_t count_{0};
    const size_t max_capacity_{10};
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::array<std::unique_ptr<ReservedEvents>, 10> reserved_events_;
    std::atomic<std::size_t> reserved_events_index_{0};
};

#pragma once
#include <string>
#include <iostream>
#include <thread>
#include <optional>
#include <memory>
#include <chrono>

#include <vector>
#include <cstdint>

#include <iomanip>

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

    virtual void Process() override { /* std::cout << "Event " << value_ << std::endl; */ }

   private:
    int value_;
};

// using EventPtr = std::unique_ptr<Event>;
using EventPtr = Event*;
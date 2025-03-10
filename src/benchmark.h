#pragma once
#include <chrono>
#include <iostream>

class Benchmark {
   public:
    void start(std::string&& newTask);
    void stop();
    void stop(uint64_t& time);

   private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> end_time;
    std::string taskName;
};

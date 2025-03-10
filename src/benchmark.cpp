#include "benchmark.h"

void Benchmark::start(std::string&& newTask) {
    taskName = std::move(newTask);
    start_time = std::chrono::high_resolution_clock::now();
}

void Benchmark::stop(uint64_t& time) {
    end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    time = elapsed_time.count();
}

void Benchmark::stop() {
    end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::printf("Benchmark result [%s]: %llu ms\n", taskName.c_str(), static_cast<unsigned long long>(elapsed_time.count()));
}

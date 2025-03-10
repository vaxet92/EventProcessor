#include "event_proc.h"
#include "types.h"
#include <algorithm>
#include <optional>
#include "benchmark.h"

#define EVENT_COUNT 10000000u
#define THREAD_COUNT 16u

int main() {
    auto event_processor = std::make_shared<IEventProcessor>(THREAD_COUNT);
    Benchmark benchmark;
    std::cout << "EventProcessor start" << std::endl;
    benchmark.start("EventProcessor");
    auto makeEvent = [event_processor](size_t count) {
        // queue multiple events
        while (count) {
            auto reserved_events_collection = event_processor->ReserveRange(count);
            // It can reserve less items than requested !You should always check how many events have been reserved !
            if (!reserved_events_collection.has_value()) {
                // ERROR: ReserveRange() failed
            } else {
                auto index = reserved_events_collection.value();
                auto events = event_processor->GetReservedEvents(index);
                auto events_count = events->Count();
                for (size_t i = 0; i < events->Count(); ++i) {
                    events->Emplace<Event>(i, static_cast<int>(i));
                }
                auto sequence_number = events->GetSequenceNumber();

                event_processor->Commit(index, sequence_number, events_count);
                count -= events_count;
            }
        }
        event_processor->active_writers.fetch_sub(1);
    };

    std::vector<std::thread> writers;
    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        writers.emplace_back(makeEvent, EVENT_COUNT);
    }

    std::thread reader([event_processor]() { event_processor->ProcessEvents(); });

    for (auto& writer : writers) {
        writer.join();
    }
    reader.join();
    benchmark.stop();
    return 0;
}
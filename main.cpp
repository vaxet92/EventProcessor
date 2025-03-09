#include "event_proc.h"
#include "types.h"
#include <algorithm>
#include <optional>

/* int main() {
    auto event_processor = std::make_unique<IEventProcessor>();

    // std::cout << sizeof(Event) << std::endl;

    // auto reserved_event = event_processor->Reserve<Event>(2);
    // if (!reserved_event.IsValid()) {
    //     std::cout << "Reserve() failed" << std::endl;
    // } else {
    //     event_processor->Commit(reserved_event.GetSequenceNumber());
    // }

    //     // queue multiple events
    auto reserved_events_collection = event_processor->ReserveRange(2);
    // // It can reserve less items than requested !You should always check how many events have been reserved !
    if (!reserved_events_collection.has_value()) {
        // ERROR: ReserveRange() failed
    } else {
        auto index = reserved_events_collection.value();
        auto events = event_processor->GetReservedEvents(index);
        for (size_t i = 0; i < events->Count(); ++i) {
            events->Emplace<Event>(i, static_cast<int>(i + 3));
        }
        auto sequence_number = events->GetSequenceNumber();

        event_processor->Commit(index, sequence_number, events->Count());
    }

    event_processor->ProcessEvents();
    return 0;
} */

int main() {
    // constexpr size_t queue_size = 1024;
    auto event_processor = std::make_shared<IEventProcessor>();

    std::atomic<size_t> active_writers(16);

    auto makeEvent = [event_processor, &active_writers](size_t count) {
        // queue multiple events
        while (count) {
            auto reserved_events_collection = event_processor->ReserveRange(count);
            // It can reserve less items than requested !You should always check how many events have been reserved !
            if (!reserved_events_collection.has_value()) {
                // ERROR: ReserveRange() failed
            } else {
                auto index = reserved_events_collection.value();
                auto events = event_processor->GetReservedEvents(index);
                for (size_t i = 0; i < events->Count(); ++i) {
                    events->Emplace<Event>(i, static_cast<int>(i + 3));
                }
                auto sequence_number = events->GetSequenceNumber();

                event_processor->Commit(index, sequence_number, events->Count());
                count -= events->Count();
            }
        }
        active_writers.fetch_sub(1);
    };

    std::vector<std::thread> writers;
    for (int i = 0; i < 1; ++i) {
        writers.emplace_back(makeEvent, 100);
    }

    std::thread reader([event_processor, &active_writers]() { event_processor->ProcessEvents(active_writers); });

    for (auto& writer : writers) {
        writer.join();
    }
    reader.join();
    return 0;
}
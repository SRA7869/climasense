#include "EventBus.h"
#include <utility>

void EventBus::subscribe(EventType type, Handler handler) {
    typed_.push_back({type, std::move(handler)});
}

void EventBus::subscribeAll(Handler handler) {
    all_.push_back(std::move(handler));
}

void EventBus::publish(Event e) {
    queue_.push_back(std::move(e));
}

std::size_t EventBus::dispatchAll() {
    std::size_t delivered = 0;
    while (!queue_.empty()) {
        Event e = std::move(queue_.front());
        queue_.pop_front();
        for (auto& h : all_) h(e);
        for (auto& s : typed_) {
            if (s.type == e.type) s.handler(e);
        }
        ++delivered;
    }
    return delivered;
}

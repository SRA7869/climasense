#pragma once
#include <cstddef>
#include <deque>
#include <functional>
#include <vector>
#include "Event.h"

class EventBus {
public:
    using Handler = std::function<void(const Event&)>;

    void subscribe(EventType type, Handler handler);
    void subscribeAll(Handler handler);
    void publish(Event e);
    std::size_t dispatchAll();
    std::size_t pending() const { return queue_.size(); }

private:
    struct Subscription {
        EventType type;
        Handler handler;
    };
    std::vector<Subscription> typed_;
    std::vector<Handler> all_;
    std::deque<Event> queue_;
};

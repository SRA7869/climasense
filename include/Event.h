#pragma once
#include <chrono>
#include <string>
#include <utility>
#include "ComfortStateMachine.h"

enum class EventType {
    SYSTEM_STARTED,
    STATE_CHANGED,
    SENSOR_INVALID,
    SENSOR_FAILED,
    SENSOR_RECOVERED,
    DEVICE_COMMAND
};

const char* toString(EventType t);

struct Event {
    EventType type;
    std::string message;
    ComfortState state;
    std::chrono::steady_clock::time_point timestamp;
};

inline Event makeEvent(EventType type, std::string message,
                       ComfortState state = ComfortState::COMFORTABLE) {
    return Event{type, std::move(message), state,
                 std::chrono::steady_clock::now()};
}

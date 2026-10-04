#include "Event.h"

const char* toString(EventType t) {
    switch (t) {
        case EventType::SYSTEM_STARTED:   return "SYSTEM_STARTED";
        case EventType::STATE_CHANGED:    return "STATE_CHANGED";
        case EventType::SENSOR_INVALID:   return "SENSOR_INVALID";
        case EventType::SENSOR_FAILED:    return "SENSOR_FAILED";
        case EventType::SENSOR_RECOVERED: return "SENSOR_RECOVERED";
        case EventType::DEVICE_COMMAND:   return "DEVICE_COMMAND";
    }
    return "UNKNOWN";
}

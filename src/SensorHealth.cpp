#include "SensorHealth.h"

const char* toString(HealthChange c) {
    switch (c) {
        case HealthChange::NONE:      return "-";
        case HealthChange::FAILED:    return "FAILED";
        case HealthChange::RECOVERED: return "RECOVERED";
    }
    return "UNKNOWN";
}

SensorHealth::SensorHealth(int failAfter, int recoverAfter)
    : failAfter_(failAfter), recoverAfter_(recoverAfter) {}

HealthChange SensorHealth::update(bool readingOk) {
    if (readingOk) {
        ++goodStreak_;
        badStreak_ = 0;
    } else {
        ++badStreak_;
        goodStreak_ = 0;
    }

    if (!failed_ && badStreak_ >= failAfter_) {
        failed_ = true;
        return HealthChange::FAILED;
    }
    if (failed_ && goodStreak_ >= recoverAfter_) {
        failed_ = false;
        return HealthChange::RECOVERED;
    }
    return HealthChange::NONE;
}

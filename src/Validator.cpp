#include "Validator.h"
#include <cmath>

const char* toString(ValidationResult r) {
    switch (r) {
        case ValidationResult::OK:           return "OK";
        case ValidationResult::NOT_A_NUMBER: return "NOT_A_NUMBER";
        case ValidationResult::OUT_OF_RANGE: return "OUT_OF_RANGE";
        case ValidationResult::STALE:        return "STALE";
        case ValidationResult::STUCK:        return "STUCK";
    }
    return "UNKNOWN";
}

Validator::Validator(Limits limits, int stuckThreshold,
                     std::chrono::milliseconds maxAge)
    : limits_(limits), stuckThreshold_(stuckThreshold), maxAge_(maxAge) {}

ValidationResult Validator::check(const Reading& r,
                                  std::chrono::steady_clock::time_point now) {
    if (std::isnan(r.value)) return ValidationResult::NOT_A_NUMBER;
    if (r.value < limits_.min || r.value > limits_.max)
        return ValidationResult::OUT_OF_RANGE;
    if (now - r.timestamp > maxAge_) return ValidationResult::STALE;

    if (hasLast_ && r.value == lastValue_) {
        ++sameCount_;
    } else {
        sameCount_ = 1;
    }
    hasLast_ = true;
    lastValue_ = r.value;

    if (sameCount_ >= stuckThreshold_) return ValidationResult::STUCK;
    return ValidationResult::OK;
}

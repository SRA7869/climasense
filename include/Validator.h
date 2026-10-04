#pragma once
#include <chrono>
#include "SensorManager.h"

enum class ValidationResult { OK, NOT_A_NUMBER, OUT_OF_RANGE, STALE, STUCK };

const char* toString(ValidationResult r);

struct Limits {
    double min;
    double max;
};

class Validator {
public:
    Validator(Limits limits, int stuckThreshold = 5,
              std::chrono::milliseconds maxAge = std::chrono::milliseconds(2000));

    ValidationResult check(const Reading& r,
                           std::chrono::steady_clock::time_point now);

private:
    Limits limits_;
    int stuckThreshold_;
    std::chrono::milliseconds maxAge_;
    bool hasLast_ = false;
    double lastValue_ = 0.0;
    int sameCount_ = 0;
};

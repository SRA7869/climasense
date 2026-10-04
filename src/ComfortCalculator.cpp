#include "ComfortCalculator.h"

const char* toString(TempLevel l) {
    switch (l) {
        case TempLevel::NORMAL:    return "NORMAL";
        case TempLevel::HIGH:      return "HIGH";
        case TempLevel::VERY_HIGH: return "VERY_HIGH";
    }
    return "UNKNOWN";
}

const char* toString(HumidityLevel l) {
    switch (l) {
        case HumidityLevel::NORMAL: return "NORMAL";
        case HumidityLevel::HIGH:   return "HIGH";
    }
    return "UNKNOWN";
}

ComfortCalculator::ComfortCalculator(ComfortConfig cfg) : cfg_(cfg) {}

ComfortAssessment ComfortCalculator::assess(double t, double h) {
    const double highOff     = cfg_.tempHigh - cfg_.hysteresis;
    const double veryHighOff = cfg_.tempVeryHigh - cfg_.hysteresis;

    switch (tempLevel_) {
        case TempLevel::NORMAL:
            if (t >= cfg_.tempVeryHigh)      tempLevel_ = TempLevel::VERY_HIGH;
            else if (t >= cfg_.tempHigh)     tempLevel_ = TempLevel::HIGH;
            break;
        case TempLevel::HIGH:
            if (t >= cfg_.tempVeryHigh)      tempLevel_ = TempLevel::VERY_HIGH;
            else if (t < highOff)            tempLevel_ = TempLevel::NORMAL;
            break;
        case TempLevel::VERY_HIGH:
            if (t < veryHighOff)
                tempLevel_ = (t < highOff) ? TempLevel::NORMAL : TempLevel::HIGH;
            break;
    }

    if (humLevel_ == HumidityLevel::NORMAL) {
        if (h >= cfg_.humidityHigh) humLevel_ = HumidityLevel::HIGH;
    } else {
        if (h < cfg_.humidityHigh - cfg_.hysteresis) humLevel_ = HumidityLevel::NORMAL;
    }

    return {tempLevel_, humLevel_};
}

#pragma once

enum class TempLevel { NORMAL, HIGH, VERY_HIGH };
enum class HumidityLevel { NORMAL, HIGH };

const char* toString(TempLevel l);
const char* toString(HumidityLevel l);

struct ComfortConfig {
    double tempHigh      = 28.0;   // C
    double tempVeryHigh  = 33.0;   // C
    double humidityHigh  = 70.0;   // %RH
    double hysteresis    = 1.0;    // margin before dropping back down
};

struct ComfortAssessment {
    TempLevel temp;
    HumidityLevel humidity;
};

class ComfortCalculator {
public:
    explicit ComfortCalculator(ComfortConfig cfg = ComfortConfig{});
    ComfortAssessment assess(double tempC, double humidityPct);
    const ComfortConfig& config() const { return cfg_; }

private:
    ComfortConfig cfg_;
    TempLevel tempLevel_ = TempLevel::NORMAL;
    HumidityLevel humLevel_ = HumidityLevel::NORMAL;
};

#pragma once
#include <atomic>
#include <random>
#include <string>
#include "ISensor.h"

class HumiditySensor : public ISensor {
public:
    explicit HumiditySensor(double startPct = 50.0);
    double read() override;
    std::string name() const override { return "Humidity"; }
    std::string unit() const override { return "%"; }
    void setTarget(double targetPct);

private:
    double current_;
    std::atomic<double> target_;
    std::mt19937 rng_;
    std::normal_distribution<double> noise_;
};

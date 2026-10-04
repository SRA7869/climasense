#pragma once
#include <atomic>
#include <random>
#include <string>
#include "ISensor.h"

class TemperatureSensor : public ISensor {
public:
    explicit TemperatureSensor(double startC = 25.0);
    double read() override;
    std::string name() const override { return "Temperature"; }
    std::string unit() const override { return "C"; }
    void setTarget(double targetC);

private:
    double current_;
    std::atomic<double> target_;
    std::mt19937 rng_;
    std::normal_distribution<double> noise_;
};

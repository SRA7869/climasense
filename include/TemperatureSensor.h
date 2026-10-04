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
    void setTarget(double targetC);   // safe to call from any thread

private:
    double current_;                  // only touched by read()
    std::atomic<double> target_;      // written by setTarget(), read by read()
    std::mt19937 rng_;
    std::normal_distribution<double> noise_;
};

#pragma once
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include "ISensor.h"

struct Reading {
    std::string name;
    std::string unit;
    double value;
    std::chrono::steady_clock::time_point timestamp;
};

class SensorManager {
public:
    void addSensor(std::unique_ptr<ISensor> sensor);
    std::vector<Reading> pollAll();
    std::size_t count() const { return sensors_.size(); }

private:
    std::vector<std::unique_ptr<ISensor>> sensors_;
};

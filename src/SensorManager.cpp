#include "SensorManager.h"
#include <utility>

void SensorManager::addSensor(std::unique_ptr<ISensor> sensor) {
    sensors_.push_back(std::move(sensor));
}

std::vector<Reading> SensorManager::pollAll() {
    std::vector<Reading> readings;
    readings.reserve(sensors_.size());
    for (auto& s : sensors_) {
        readings.push_back({s->name(), s->unit(), s->read(),
                            std::chrono::steady_clock::now()});
    }
    return readings;
}

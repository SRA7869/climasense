#include "TemperatureSensor.h"

TemperatureSensor::TemperatureSensor(double startC)
    : current_(startC), target_(startC),
      rng_(std::random_device{}()), noise_(0.0, 0.15) {}

void TemperatureSensor::setTarget(double targetC) {
    target_.store(targetC);
}

double TemperatureSensor::read() {
    current_ += (target_.load() - current_) * 0.1;
    return current_ + noise_(rng_);
}

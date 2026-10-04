#include "HumiditySensor.h"
#include <algorithm>

HumiditySensor::HumiditySensor(double startPct)
    : current_(startPct), target_(startPct),
      rng_(std::random_device{}()), noise_(0.0, 0.5) {}

void HumiditySensor::setTarget(double targetPct) {
    target_.store(std::clamp(targetPct, 0.0, 100.0));
}

double HumiditySensor::read() {
    current_ += (target_.load() - current_) * 0.08;
    return std::clamp(current_ + noise_(rng_), 0.0, 100.0);
}

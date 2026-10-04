#include "FaultySensor.h"
#include <limits>
#include <utility>

const char* toString(Fault f) {
    switch (f) {
        case Fault::NONE:         return "NONE";
        case Fault::NAN_VALUE:    return "NAN_VALUE";
        case Fault::OUT_OF_RANGE: return "OUT_OF_RANGE";
        case Fault::STUCK:        return "STUCK";
    }
    return "UNKNOWN";
}

FaultySensor::FaultySensor(std::unique_ptr<ISensor> inner)
    : inner_(std::move(inner)) {}

void FaultySensor::setFault(Fault f) {
    fault_.store(f);
}

double FaultySensor::read() {
    const double real = inner_->read();   // the real room keeps evolving

    switch (fault_.load()) {
        case Fault::NAN_VALUE:    return std::numeric_limits<double>::quiet_NaN();
        case Fault::OUT_OF_RANGE: return 999.0;
        case Fault::STUCK:        return hasLastGood_ ? lastGood_ : real;
        case Fault::NONE:         break;
    }
    lastGood_ = real;
    hasLastGood_ = true;
    return real;
}

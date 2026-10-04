#pragma once
#include <atomic>
#include <memory>
#include <string>
#include "ISensor.h"

enum class Fault { NONE, NAN_VALUE, OUT_OF_RANGE, STUCK };

const char* toString(Fault f);

class FaultySensor : public ISensor {
public:
    explicit FaultySensor(std::unique_ptr<ISensor> inner);

    double read() override;
    std::string name() const override { return inner_->name(); }
    std::string unit() const override { return inner_->unit(); }

    void setFault(Fault f);

private:
    std::unique_ptr<ISensor> inner_;
    std::atomic<Fault> fault_{Fault::NONE};
    double lastGood_ = 0.0;
    bool hasLastGood_ = false;
};

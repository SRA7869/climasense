#pragma once
#include <chrono>
#include <string>
#include "IDevice.h"

class VirtualDevice : public IDevice {
public:
    VirtualDevice(std::string name, double watts);

    void setOn(bool on) override;
    bool isOn() const override { return on_; }
    std::string name() const override { return name_; }
    double ratedWatts() const override { return watts_; }
    int switchCount() const override { return switches_; }
    std::chrono::milliseconds onTime() const override;

private:
    std::string name_;
    double watts_;
    bool on_ = false;
    int switches_ = 0;
    std::chrono::steady_clock::time_point onSince_{};
    std::chrono::milliseconds accumulated_{0};
};

class Fan : public VirtualDevice {
public:
    Fan() : VirtualDevice("FAN", 60.0) {}
};

class AirConditioner : public VirtualDevice {
public:
    AirConditioner() : VirtualDevice("AC", 1500.0) {}
};

class Exhaust : public VirtualDevice {
public:
    Exhaust() : VirtualDevice("EXHAUST", 40.0) {}
};

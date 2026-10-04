#pragma once
#include <chrono>
#include <string>

class IDevice {
public:
    virtual ~IDevice() = default;
    virtual void setOn(bool on) = 0;
    virtual bool isOn() const = 0;
    virtual std::string name() const = 0;
    virtual double ratedWatts() const = 0;
    virtual int switchCount() const = 0;
    virtual std::chrono::milliseconds onTime() const = 0;
};

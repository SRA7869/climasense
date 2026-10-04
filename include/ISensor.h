#pragma once
#include <string>

class ISensor {
public:
    virtual ~ISensor() = default;
    virtual double read() = 0;
    virtual std::string name() const = 0;
    virtual std::string unit() const = 0;
};

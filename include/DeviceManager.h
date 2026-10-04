#pragma once
#include <array>
#include <memory>
#include <string>
#include "AutomationEngine.h"
#include "IDevice.h"

class DeviceManager {
public:
    DeviceManager();

    void apply(const DeviceCommand& cmd);
    const IDevice& device(DeviceId id) const;
    std::string statusLine() const;
    int totalSwitches() const;

private:
    std::array<std::unique_ptr<IDevice>, 3> devices_;
};

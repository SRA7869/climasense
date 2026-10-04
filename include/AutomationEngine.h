#pragma once
#include <vector>
#include "ComfortCalculator.h"

enum class DeviceId { FAN, AC, EXHAUST };

const char* toString(DeviceId d);

struct DeviceCommand {
    DeviceId device;
    bool on;
};

struct DeviceDemand {      // which devices should be on right now
    bool fan = false;
    bool ac = false;
    bool exhaust = false;
};

struct AutomationConfig {
    bool fanRunsWithAc = true;   // keep the fan on while the AC is on
};

class AutomationEngine {
public:
    explicit AutomationEngine(AutomationConfig cfg = AutomationConfig{});

    static DeviceDemand decide(const ComfortAssessment& a,
                               const AutomationConfig& cfg);

    std::vector<DeviceCommand> evaluate(const ComfortAssessment& a);
    std::vector<DeviceCommand> shutdownAll();   // fail-safe: everything off
    const DeviceDemand& current() const { return current_; }

private:
    AutomationConfig cfg_;
    DeviceDemand current_;
};

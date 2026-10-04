#include "AutomationEngine.h"

const char* toString(DeviceId d) {
    switch (d) {
        case DeviceId::FAN:     return "FAN";
        case DeviceId::AC:      return "AC";
        case DeviceId::EXHAUST: return "EXHAUST";
    }
    return "UNKNOWN";
}

AutomationEngine::AutomationEngine(AutomationConfig cfg) : cfg_(cfg) {}

DeviceDemand AutomationEngine::decide(const ComfortAssessment& a,
                                      const AutomationConfig& cfg) {
    DeviceDemand d;
    d.ac      = (a.temp == TempLevel::VERY_HIGH);
    d.fan     = (a.temp == TempLevel::HIGH) || (d.ac && cfg.fanRunsWithAc);
    d.exhaust = (a.humidity == HumidityLevel::HIGH);
    return d;
}

std::vector<DeviceCommand> AutomationEngine::evaluate(const ComfortAssessment& a) {
    const DeviceDemand next = decide(a, cfg_);
    std::vector<DeviceCommand> cmds;
    if (next.fan     != current_.fan)     cmds.push_back({DeviceId::FAN,     next.fan});
    if (next.ac      != current_.ac)      cmds.push_back({DeviceId::AC,      next.ac});
    if (next.exhaust != current_.exhaust) cmds.push_back({DeviceId::EXHAUST, next.exhaust});
    current_ = next;
    return cmds;
}

std::vector<DeviceCommand> AutomationEngine::shutdownAll() {
    return evaluate({TempLevel::NORMAL, HumidityLevel::NORMAL});
}

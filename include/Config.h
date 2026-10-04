#pragma once
#include <string>
#include <vector>
#include "AutomationEngine.h"
#include "ComfortCalculator.h"

struct SystemConfig {
    ComfortConfig comfort;
    AutomationConfig automation;
};

struct ConfigReport {
    bool fileFound = false;
    std::vector<std::string> problems;
};

// Starts from whatever is in cfg (the defaults), applies valid lines from the
// file, and reverts to defaults if the result is inconsistent.
ConfigReport loadConfig(const std::string& path, SystemConfig& cfg);

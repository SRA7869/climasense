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

ConfigReport loadConfig(const std::string& path, SystemConfig& cfg);

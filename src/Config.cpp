#include "Config.h"
#include <cmath>
#include <exception>
#include <fstream>

namespace {

std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r");
    if (b == std::string::npos) return "";
    const auto e = s.find_last_not_of(" \t\r");
    return s.substr(b, e - b + 1);
}

bool parseDouble(const std::string& s, double& out) {
    try {
        std::size_t used = 0;
        out = std::stod(s, &used);
        return used == s.size() && std::isfinite(out);
    } catch (const std::exception&) {
        return false;
    }
}

bool parseBool(const std::string& s, bool& out) {
    if (s == "true" || s == "1")  { out = true;  return true; }
    if (s == "false" || s == "0") { out = false; return true; }
    return false;
}

}  // namespace

ConfigReport loadConfig(const std::string& path, SystemConfig& cfg) {
    ConfigReport report;
    std::ifstream in(path);
    if (!in) return report;          // file missing: fileFound stays false, defaults kept
    report.fileFound = true;

    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        const auto hash = line.find('#');
        if (hash != std::string::npos) line.erase(hash);
        line = trim(line);
        if (line.empty()) continue;

        const std::string where = "line " + std::to_string(lineNo) + ": ";
        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            report.problems.push_back(where + "expected key = value");
            continue;
        }
        const std::string key = trim(line.substr(0, eq));
        const std::string value = trim(line.substr(eq + 1));

        auto number = [&](double& target) {
            double d = 0.0;
            if (parseDouble(value, d)) {
                target = d;
            } else {
                report.problems.push_back(where + key + ": '" + value +
                                          "' is not a number");
            }
        };

        if (key == "temp_high")           number(cfg.comfort.tempHigh);
        else if (key == "temp_very_high") number(cfg.comfort.tempVeryHigh);
        else if (key == "humidity_high")  number(cfg.comfort.humidityHigh);
        else if (key == "hysteresis")     number(cfg.comfort.hysteresis);
        else if (key == "fan_runs_with_ac") {
            bool b = false;
            if (parseBool(value, b)) {
                cfg.automation.fanRunsWithAc = b;
            } else {
                report.problems.push_back(where + key + ": '" + value +
                                          "' is not true/false");
            }
        } else {
            report.problems.push_back(where + "unknown key '" + key + "'");
        }
    }

    const ComfortConfig& c = cfg.comfort;
    const bool consistent =
        c.tempHigh < c.tempVeryHigh &&
        c.hysteresis >= 0.0 &&
        c.hysteresis < (c.tempVeryHigh - c.tempHigh) &&
        c.humidityHigh > 0.0 && c.humidityHigh <= 100.0 &&
        c.hysteresis < c.humidityHigh;
    if (!consistent) {
        cfg = SystemConfig{};
        report.problems.push_back(
            "thresholds inconsistent (need temp_high < temp_very_high, "
            "0 <= hysteresis < that gap, 0 < humidity_high <= 100), using defaults");
    }
    return report;
}

#include "Monitor.h"
#include <iomanip>
#include <sstream>
#include <utility>

void RunningStat::add(double v) {
    if (count == 0 || v < min) min = v;
    if (count == 0 || v > max) max = v;
    sum += v;
    ++count;
}

void Snapshot::pushRecent(std::string line) {
    recent.push_back(std::move(line));
    while (recent.size() > kRecent) recent.pop_front();
}

void StatusBoard::update(const std::function<void(Snapshot&)>& change) {
    std::lock_guard<std::mutex> lock(m_);
    change(s_);
}

Snapshot StatusBoard::snapshot() const {
    std::lock_guard<std::mutex> lock(m_);
    return s_;
}

namespace {
const char* onOff(bool b) { return b ? "ON " : "OFF"; }
}

std::string renderDashboard(const Snapshot& s, long long uptimeMs) {
    std::ostringstream os;
    os << std::fixed;
    const std::string rule(52, '-');

    os << "==================== ClimaSense ====================\n";
    os << " Uptime " << std::setprecision(1) << static_cast<double>(uptimeMs) / 1000.0
       << " s   |   samples " << s.stats.samples << "\n";
    os << rule << "\n";

    os << std::setprecision(2);
    auto sensor = [&](const char* label, const char* unit, double value,
                      bool failed, const RunningStat& st) {
        os << " " << std::left << std::setw(12) << label << std::right
           << std::setw(7) << value << " " << unit << "   "
           << (failed ? "FAILED" : "OK") << "\n";
        os << "   min " << st.min << "   avg " << st.mean()
           << "   max " << st.max << "\n";
    };
    sensor("Temperature", "C", s.temp, s.tempFailed, s.stats.temp);
    sensor("Humidity", "%", s.humidity, s.humidityFailed, s.stats.humidity);
    os << rule << "\n";

    os << " Comfort state : " << toString(s.state) << "\n";
    os << " Devices       : FAN " << onOff(s.fan) << " | AC " << onOff(s.ac)
       << " | EXHAUST " << onOff(s.exhaust) << "\n";
    os << rule << "\n";

    os << " Statistics\n";
    os << "   invalid readings " << s.stats.invalidReadings
       << "   failures " << s.stats.failures
       << "   recoveries " << s.stats.recoveries << "\n";
    os << "   state changes " << s.stats.stateChanges
       << "   device switches " << s.stats.deviceSwitches << "\n";
    os << std::setprecision(1);
    const auto& t = s.stats.secondsInState;
    os << "   seconds in state: COMFORTABLE " << t[0] << "  WARM " << t[1] << "\n";
    os << "                     HUMID " << t[2]
       << "  VERY_UNCOMFORTABLE " << t[3] << "\n";
    os << rule << "\n";

    os << " Recent events\n";
    for (std::size_t i = 0; i < Snapshot::kRecent; ++i) {
        os << "   " << (i < s.recent.size() ? s.recent[i] : std::string()) << "\n";
    }
    os << std::string(52, '=');
    return os.str();
}

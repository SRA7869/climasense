#include <chrono>
#include <thread>
#include <vector>
#include "AutomationEngine.h"
#include "ComfortCalculator.h"
#include "ComfortStateMachine.h"
#include "DeviceManager.h"
#include "VirtualDevices.h"
#include "test.h"

using namespace std::chrono_literals;

namespace {

bool single(const std::vector<DeviceCommand>& v, DeviceId d, bool on) {
    return v.size() == 1 && v[0].device == d && v[0].on == on;
}

}

TEST(automation_decides_devices_for_every_level) {
    struct Row {
        TempLevel t;
        HumidityLevel h;
        bool fan;
        bool ac;
        bool exhaust;
    };
    const Row rows[] = {
        {TempLevel::NORMAL,    HumidityLevel::NORMAL, false, false, false},
        {TempLevel::NORMAL,    HumidityLevel::HIGH,   false, false, true},
        {TempLevel::HIGH,      HumidityLevel::NORMAL, true,  false, false},
        {TempLevel::HIGH,      HumidityLevel::HIGH,   true,  false, true},
        {TempLevel::VERY_HIGH, HumidityLevel::NORMAL, true,  true,  false},
        {TempLevel::VERY_HIGH, HumidityLevel::HIGH,   true,  true,  true},
    };
    for (const Row& r : rows) {
        const DeviceDemand d = AutomationEngine::decide({r.t, r.h}, AutomationConfig{});
        CHECK(d.fan == r.fan);
        CHECK(d.ac == r.ac);
        CHECK(d.exhaust == r.exhaust);
    }
}

TEST(automation_fan_can_be_independent_of_ac) {
    AutomationConfig cfg;
    cfg.fanRunsWithAc = false;
    const DeviceDemand d = AutomationEngine::decide(
        {TempLevel::VERY_HIGH, HumidityLevel::NORMAL}, cfg);
    CHECK(d.ac);
    CHECK(!d.fan);
}

TEST(automation_commands_only_on_change) {
    AutomationEngine e;
    CHECK(e.evaluate({TempLevel::NORMAL, HumidityLevel::NORMAL}).empty());
    CHECK(single(e.evaluate({TempLevel::HIGH, HumidityLevel::NORMAL}), DeviceId::FAN, true));
    CHECK(e.evaluate({TempLevel::HIGH, HumidityLevel::NORMAL}).empty());
    CHECK(single(e.evaluate({TempLevel::HIGH, HumidityLevel::HIGH}), DeviceId::EXHAUST, true));
    CHECK(single(e.evaluate({TempLevel::VERY_HIGH, HumidityLevel::HIGH}), DeviceId::AC, true));
    CHECK(single(e.evaluate({TempLevel::HIGH, HumidityLevel::HIGH}), DeviceId::AC, false));
}

TEST(automation_shutdown_all_turns_everything_off) {
    AutomationEngine e;
    CHECK(e.evaluate({TempLevel::VERY_HIGH, HumidityLevel::HIGH}).size() == 3);

    const std::vector<DeviceCommand> off = e.shutdownAll();
    CHECK(off.size() == 3);
    for (const DeviceCommand& c : off) CHECK(!c.on);

    CHECK(e.shutdownAll().empty());

    const std::vector<DeviceCommand> on = e.evaluate({TempLevel::VERY_HIGH, HumidityLevel::HIGH});
    CHECK(on.size() == 3);
    for (const DeviceCommand& c : on) CHECK(c.on);
}

TEST(devices_virtual_device_ignores_repeated_state) {
    Fan f;
    f.setOn(true);
    f.setOn(true);
    CHECK(f.isOn());
    CHECK(f.switchCount() == 1);
    f.setOn(false);
    CHECK(!f.isOn());
    CHECK(f.switchCount() == 2);
    CHECK(f.name() == "FAN");
}

TEST(devices_manager_routes_commands) {
    DeviceManager dm;
    dm.apply({DeviceId::AC, true});
    CHECK(dm.device(DeviceId::AC).isOn());
    CHECK(!dm.device(DeviceId::FAN).isOn());
    CHECK(dm.statusLine() == "FAN:OFF AC:ON EXHAUST:OFF");
    CHECK(dm.totalSwitches() == 1);
}

TEST(devices_on_time_accumulates_only_while_on) {
    Fan f;
    f.setOn(true);
    std::this_thread::sleep_for(50ms);
    f.setOn(false);
    const auto t1 = f.onTime();
    CHECK(t1 >= 40ms);
    std::this_thread::sleep_for(30ms);
    CHECK(f.onTime() == t1);
}

TEST(pipeline_heat_up_and_cool_down) {
    ComfortCalculator calc;
    ComfortStateMachine sm;
    AutomationEngine engine;
    DeviceManager devices;

    auto feed = [&](double t, double h) {
        const ComfortAssessment a = calc.assess(t, h);
        sm.update(a);
        for (const DeviceCommand& c : engine.evaluate(a)) devices.apply(c);
    };

    feed(25.0, 50.0);
    CHECK(sm.state() == ComfortState::COMFORTABLE);
    CHECK(devices.statusLine() == "FAN:OFF AC:OFF EXHAUST:OFF");

    feed(29.0, 50.0);
    CHECK(sm.state() == ComfortState::WARM);
    CHECK(devices.statusLine() == "FAN:ON AC:OFF EXHAUST:OFF");

    feed(34.0, 75.0);
    CHECK(sm.state() == ComfortState::VERY_UNCOMFORTABLE);
    CHECK(devices.statusLine() == "FAN:ON AC:ON EXHAUST:ON");

    feed(31.0, 75.0);
    CHECK(sm.state() == ComfortState::VERY_UNCOMFORTABLE);
    CHECK(devices.statusLine() == "FAN:ON AC:OFF EXHAUST:ON");

    feed(26.0, 60.0);
    CHECK(sm.state() == ComfortState::COMFORTABLE);
    CHECK(devices.statusLine() == "FAN:OFF AC:OFF EXHAUST:OFF");

    CHECK(devices.totalSwitches() == 6);
    CHECK(sm.transitionCount() == 3);
}

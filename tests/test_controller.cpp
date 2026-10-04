#include <chrono>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "Controller.h"
#include "HumiditySensor.h"
#include "TemperatureSensor.h"
#include "test.h"

using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;

namespace {

std::vector<Reading> sample(double t, double h) {
    const auto now = Clock::now();
    return {Reading{"Temperature", "C", t, now}, Reading{"Humidity", "%", h, now}};
}

void feed(Controller& c, double t, double h) {
    c.processBatch(sample(t, h), Clock::now());
}

const char* kAllOn  = "FAN:ON AC:ON EXHAUST:ON";
const char* kAllOff = "FAN:OFF AC:OFF EXHAUST:OFF";

void heatUp(Controller& c) {
    feed(c, 25.0, 50.0);
    feed(c, 29.0, 50.5);
    feed(c, 34.0, 75.0);
}

}

TEST(controller_heats_up_and_switches_devices) {
    Controller c(SystemConfig{});

    feed(c, 25.0, 50.0);
    CHECK(c.machine().state() == ComfortState::COMFORTABLE);
    CHECK(c.devices().statusLine() == kAllOff);

    feed(c, 29.0, 50.5);
    CHECK(c.machine().state() == ComfortState::WARM);
    CHECK(c.devices().statusLine() == "FAN:ON AC:OFF EXHAUST:OFF");

    feed(c, 34.0, 75.0);
    CHECK(c.machine().state() == ComfortState::VERY_UNCOMFORTABLE);
    CHECK(c.devices().statusLine() == kAllOn);

    const Snapshot s = c.snapshot();
    CHECK(s.fan && s.ac && s.exhaust);
    CHECK(s.state == ComfortState::VERY_UNCOMFORTABLE);
    CHECK(s.stats.samples == 3);
    CHECK(s.stats.stateChanges == 2);
    CHECK(s.stats.deviceSwitches == 3);
}

TEST(controller_fails_safe_then_recovers) {
    Controller c(SystemConfig{});
    heatUp(c);
    CHECK(c.devices().statusLine() == kAllOn);

    feed(c, 999.0, 75.1);
    CHECK(c.devices().statusLine() == kAllOn);
    feed(c, 999.0, 75.2);
    CHECK(c.devices().statusLine() == kAllOn);
    CHECK(!c.snapshot().tempFailed);

    feed(c, 999.0, 75.3);
    CHECK(c.snapshot().tempFailed);
    CHECK(c.devices().statusLine() == kAllOff);

    feed(c, 34.1, 75.4);
    feed(c, 34.2, 75.5);
    CHECK(c.snapshot().tempFailed);
    CHECK(c.devices().statusLine() == kAllOff);

    feed(c, 34.3, 75.6);
    CHECK(!c.snapshot().tempFailed);
    CHECK(c.devices().statusLine() == kAllOn);

    const Snapshot s = c.snapshot();
    CHECK(s.stats.invalidReadings == 3);
    CHECK(s.stats.failures == 1);
    CHECK(s.stats.recoveries == 1);
    CHECK(c.machine().transitionCount() == 2);
}

TEST(controller_ignores_a_single_bad_reading) {
    Controller c(SystemConfig{});
    heatUp(c);
    feed(c, 999.0, 75.1);
    feed(c, 34.1, 75.2);
    CHECK(c.devices().statusLine() == kAllOn);
    const Snapshot s = c.snapshot();
    CHECK(!s.tempFailed);
    CHECK(s.stats.invalidReadings == 1);
    CHECK(s.stats.failures == 0);
}

TEST(controller_statistics_exclude_invalid_readings) {
    Controller c(SystemConfig{});
    feed(c, 30.0, 50.0);
    feed(c, 999.0, 51.0);
    feed(c, 32.0, 52.0);
    const Snapshot s = c.snapshot();
    CHECK(s.stats.temp.count == 2);
    CHECK_NEAR(s.stats.temp.max, 32.0, 1e-9);
    CHECK_NEAR(s.temp, 32.0, 1e-9);
    CHECK(s.stats.humidity.count == 3);
    CHECK(s.stats.invalidReadings == 1);
}

TEST(controller_runs_and_stops_cleanly) {
    Controller c(SystemConfig{}, "", 20ms);
    c.addSensor(std::make_unique<TemperatureSensor>(25.0));
    c.addSensor(std::make_unique<HumiditySensor>(50.0));
    CHECK(!c.running());
    c.start();
    CHECK(c.running());
    std::this_thread::sleep_for(300ms);
    c.stop();
    CHECK(!c.running());
    CHECK(c.processed() >= 3);
    c.stop();

    Controller idle(SystemConfig{});
    idle.stop();
    CHECK(!idle.running());
}

TEST(controller_writes_events_to_the_log_file) {
    const char* path = "test_tmp.log";
    std::remove(path);
    {
        Controller c(SystemConfig{}, path);
        c.note("hello from the test");
        feed(c, 25.0, 50.0);
        feed(c, 29.0, 50.5);
    }
    std::ifstream in(path);
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string text = ss.str();
    std::remove(path);
    CHECK(text.find("run started") != std::string::npos);
    CHECK(text.find("hello from the test") != std::string::npos);
    CHECK(text.find("COMFORTABLE -> WARM") != std::string::npos);
    CHECK(text.find("DEVICE_COMMAND") != std::string::npos);
}

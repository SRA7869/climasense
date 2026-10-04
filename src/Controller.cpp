#include "Controller.h"
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <utility>

Controller::Controller(SystemConfig config, const std::string& logPath,
                       std::chrono::milliseconds samplePeriod)
    : start_(Clock::now()),
      period_(samplePeriod),
      calc_(config.comfort),
      engine_(config.automation),
      lastTick_(start_) {
    if (!logPath.empty()) logger_ = std::make_unique<Logger>(logPath, start_);

    // Statistics and the recent-events panel are just another subscriber.
    bus_.subscribeAll([this](const Event& e) {
        std::ostringstream os;
        os << std::left << std::setw(17) << toString(e.type) << std::right << e.message;
        const std::string line = stamp(e.timestamp, os.str());
        board_.update([&](Snapshot& s) {
            s.pushRecent(line);
            switch (e.type) {
                case EventType::SENSOR_INVALID:   ++s.stats.invalidReadings; break;
                case EventType::SENSOR_FAILED:    ++s.stats.failures;        break;
                case EventType::SENSOR_RECOVERED: ++s.stats.recoveries;      break;
                case EventType::STATE_CHANGED:    ++s.stats.stateChanges;    break;
                case EventType::DEVICE_COMMAND:   ++s.stats.deviceSwitches;  break;
                case EventType::SYSTEM_STARTED:   break;
            }
        });
    });
    bus_.subscribeAll([this](const Event& e) {
        if (logger_) logger_->log(e);
    });
}

Controller::~Controller() {
    stop();
}

void Controller::addSensor(std::unique_ptr<ISensor> sensor) {
    manager_.addSensor(std::move(sensor));
}

void Controller::note(const std::string& text) {
    if (logger_) logger_->note(text);
}

void Controller::mark(const std::string& text) {
    const std::string line = stamp(Clock::now(), text);
    board_.update([&](Snapshot& s) { s.pushRecent(line); });
}

long long Controller::uptimeMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start_).count();
}

std::string Controller::stamp(Clock::time_point when, const std::string& text) const {
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(when - start_).count();
    std::ostringstream os;
    os << "[" << std::setw(5) << ms << " ms] " << text;
    return os.str();
}

void Controller::start() {
    if (running_.exchange(true)) return;                // already running

    bus_.publish(makeEvent(EventType::SYSTEM_STARTED, "controller started"));
    bus_.dispatchAll();

    sensorThread_ = std::thread([this] {
        while (running_) {
            samples_.push(manager_.pollAll());
            std::this_thread::sleep_for(period_);
        }
    });
    controlThread_ = std::thread([this] {
        std::vector<Reading> batch;
        while (samples_.pop(batch)) processBatch(batch, Clock::now());
    });
}

void Controller::stop() {
    if (!running_.exchange(false)) return;              // never started, or already stopped
    sensorThread_.join();                               // stop the producer
    samples_.close();                                   // let the consumer drain and finish
    controlThread_.join();
    note("system stopped");
}

// Judge one reading: publish events, update health, say if it is usable.
bool Controller::judge(const Reading& r, Validator& val, SensorHealth& health,
                       Clock::time_point now) {
    const ValidationResult v = val.check(r, now);
    const bool ok = (v == ValidationResult::OK);
    if (!ok && !health.failed()) {
        bus_.publish(makeEvent(EventType::SENSOR_INVALID, r.name + ": " + toString(v)));
    }
    const HealthChange c = health.update(ok);
    if (c == HealthChange::FAILED) {
        bus_.publish(makeEvent(EventType::SENSOR_FAILED, r.name + " sensor failed"));
    } else if (c == HealthChange::RECOVERED) {
        bus_.publish(makeEvent(EventType::SENSOR_RECOVERED, r.name + " sensor recovered"));
    }
    return ok;
}

void Controller::processBatch(const std::vector<Reading>& batch, Clock::time_point now) {
    ++processed_;
    double t = 0.0, h = 0.0;
    bool tOk = false, hOk = false;
    for (const Reading& r : batch) {
        if (r.name == "Temperature") {
            t = r.value;
            tOk = judge(r, tempVal_, tempHealth_, now);
        } else if (r.name == "Humidity") {
            h = r.value;
            hOk = judge(r, humVal_, humHealth_, now);
        }
    }

    std::vector<DeviceCommand> cmds;
    if (tempHealth_.failed() || humHealth_.failed()) {
        cmds = engine_.shutdownAll();                   // fail-safe
    } else if (tOk && hOk) {
        const ComfortAssessment a = calc_.assess(t, h);
        const StateTransition tr = machine_.update(a);
        if (tr.changed) {
            bus_.publish(makeEvent(
                EventType::STATE_CHANGED,
                std::string(toString(tr.from)) + " -> " + toString(tr.to),
                tr.to));
        }
        cmds = engine_.evaluate(a);
    }   // else: unusable sample, but the sensor is not failed yet; skip it

    for (const DeviceCommand& c : cmds) {
        devices_.apply(c);
        bus_.publish(makeEvent(
            EventType::DEVICE_COMMAND,
            std::string(toString(c.device)) + (c.on ? " ON" : " OFF")));
    }

    // Publish what the screen needs, in one short locked step.
    const double dt = std::max(0.0, std::chrono::duration<double>(now - lastTick_).count());
    lastTick_ = now;
    board_.update([&](Snapshot& s) {
        ++s.stats.samples;
        s.stats.secondsInState[static_cast<std::size_t>(s.state)] += dt;
        if (tOk) { s.temp = t;     s.stats.temp.add(t); }
        if (hOk) { s.humidity = h; s.stats.humidity.add(h); }
        s.tempFailed = tempHealth_.failed();
        s.humidityFailed = humHealth_.failed();
        s.state = machine_.state();
        s.fan = devices_.device(DeviceId::FAN).isOn();
        s.ac = devices_.device(DeviceId::AC).isOn();
        s.exhaust = devices_.device(DeviceId::EXHAUST).isOn();
    });

    bus_.dispatchAll();
}

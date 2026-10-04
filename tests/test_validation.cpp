#include <chrono>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include "FaultySensor.h"
#include "ISensor.h"
#include "SensorHealth.h"
#include "SensorManager.h"
#include "Validator.h"
#include "test.h"

using Clock = std::chrono::steady_clock;

namespace {

Reading reading(double value, Clock::time_point when) {
    return Reading{"Temperature", "C", value, when};
}

class RampSensor : public ISensor {
public:
    double read() override { return value_ += 1.0; }
    std::string name() const override { return "Ramp"; }
    std::string unit() const override { return "u"; }

private:
    double value_ = 0.0;
};

}

TEST(validator_accepts_normal_reading) {
    Validator v(Limits{-40.0, 80.0});
    const auto now = Clock::now();
    CHECK(v.check(reading(25.0, now), now) == ValidationResult::OK);
}

TEST(validator_rejects_nan) {
    Validator v(Limits{-40.0, 80.0});
    const auto now = Clock::now();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    CHECK(v.check(reading(nan, now), now) == ValidationResult::NOT_A_NUMBER);
}

TEST(validator_rejects_out_of_range) {
    Validator v(Limits{-40.0, 80.0});
    const auto now = Clock::now();
    const double inf = std::numeric_limits<double>::infinity();
    CHECK(v.check(reading(150.0, now), now) == ValidationResult::OUT_OF_RANGE);
    CHECK(v.check(reading(-50.0, now), now) == ValidationResult::OUT_OF_RANGE);
    CHECK(v.check(reading(inf, now), now) == ValidationResult::OUT_OF_RANGE);
    CHECK(v.check(reading(80.0, now), now) == ValidationResult::OK);
    CHECK(v.check(reading(-40.0, now), now) == ValidationResult::OK);
}

TEST(validator_flags_stale_reading) {
    Validator v(Limits{-40.0, 80.0});
    const auto now = Clock::now();
    CHECK(v.check(reading(25.0, now - std::chrono::seconds(5)), now) == ValidationResult::STALE);
    CHECK(v.check(reading(25.0, now - std::chrono::seconds(1)), now) == ValidationResult::OK);
}

TEST(validator_detects_stuck_value) {
    Validator v(Limits{-40.0, 80.0}, 3);
    const auto now = Clock::now();
    CHECK(v.check(reading(30.0, now), now) == ValidationResult::OK);
    CHECK(v.check(reading(30.0, now), now) == ValidationResult::OK);
    CHECK(v.check(reading(30.0, now), now) == ValidationResult::STUCK);
    CHECK(v.check(reading(30.5, now), now) == ValidationResult::OK);
}

TEST(health_fails_after_three_bad_readings) {
    SensorHealth h;
    CHECK(h.update(false) == HealthChange::NONE);
    CHECK(h.update(false) == HealthChange::NONE);
    CHECK(!h.failed());
    CHECK(h.update(false) == HealthChange::FAILED);
    CHECK(h.failed());
    CHECK(h.update(false) == HealthChange::NONE);
}

TEST(health_needs_consecutive_bad_readings) {
    SensorHealth h;
    h.update(false);
    h.update(false);
    h.update(true);
    h.update(false);
    h.update(false);
    CHECK(!h.failed());
    CHECK(h.update(false) == HealthChange::FAILED);
}

TEST(health_recovers_after_three_good_readings) {
    SensorHealth h;
    for (int i = 0; i < 3; ++i) h.update(false);
    CHECK(h.update(true) == HealthChange::NONE);
    CHECK(h.update(true) == HealthChange::NONE);
    CHECK(h.failed());
    CHECK(h.update(true) == HealthChange::RECOVERED);
    CHECK(!h.failed());
}

TEST(health_recovery_streak_resets_on_bad_reading) {
    SensorHealth h;
    for (int i = 0; i < 3; ++i) h.update(false);
    h.update(true);
    h.update(true);
    h.update(false);
    h.update(true);
    h.update(true);
    CHECK(h.failed());
    CHECK(h.update(true) == HealthChange::RECOVERED);
}

TEST(faulty_sensor_passes_through_when_healthy) {
    FaultySensor s(std::make_unique<RampSensor>());
    CHECK(s.name() == "Ramp");
    CHECK(s.unit() == "u");
    CHECK_NEAR(s.read(), 1.0, 1e-9);
    CHECK_NEAR(s.read(), 2.0, 1e-9);
}

TEST(faulty_sensor_nan_value_returns_nan) {
    FaultySensor s(std::make_unique<RampSensor>());
    s.setFault(Fault::NAN_VALUE);
    CHECK(std::isnan(s.read()));
}

TEST(faulty_sensor_out_of_range_returns_999) {
    FaultySensor s(std::make_unique<RampSensor>());
    s.setFault(Fault::OUT_OF_RANGE);
    CHECK_NEAR(s.read(), 999.0, 1e-9);
}

TEST(faulty_sensor_stuck_repeats_last_good_value) {
    FaultySensor s(std::make_unique<RampSensor>());
    s.read();
    s.read();
    s.setFault(Fault::STUCK);
    CHECK_NEAR(s.read(), 2.0, 1e-9);
    CHECK_NEAR(s.read(), 2.0, 1e-9);
    s.setFault(Fault::NONE);
    CHECK_NEAR(s.read(), 5.0, 1e-9);
}

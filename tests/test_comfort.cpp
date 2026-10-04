#include <initializer_list>
#include "ComfortCalculator.h"
#include "ComfortStateMachine.h"
#include "test.h"

namespace {

int countTempChanges(ComfortCalculator& calc, std::initializer_list<double> temps) {
    TempLevel last = TempLevel::NORMAL;
    int changes = 0;
    for (double t : temps) {
        const TempLevel now = calc.assess(t, 50.0).temp;
        if (now != last) {
            ++changes;
            last = now;
        }
    }
    return changes;
}

}  // namespace

// ---------- ComfortCalculator ----------

TEST(comfort_temperature_levels_with_hysteresis) {
    ComfortCalculator c;                            // 28 / 33, hysteresis 1
    CHECK(c.assess(25.0, 50.0).temp == TempLevel::NORMAL);
    CHECK(c.assess(27.9, 50.0).temp == TempLevel::NORMAL);
    CHECK(c.assess(28.0, 50.0).temp == TempLevel::HIGH);
    CHECK(c.assess(33.0, 50.0).temp == TempLevel::VERY_HIGH);
    CHECK(c.assess(32.5, 50.0).temp == TempLevel::VERY_HIGH);   // not below 32 yet
    CHECK(c.assess(31.9, 50.0).temp == TempLevel::HIGH);
    CHECK(c.assess(27.5, 50.0).temp == TempLevel::HIGH);        // not below 27 yet
    CHECK(c.assess(26.9, 50.0).temp == TempLevel::NORMAL);
}

TEST(comfort_very_high_can_drop_straight_to_normal) {
    ComfortCalculator c;
    CHECK(c.assess(34.0, 50.0).temp == TempLevel::VERY_HIGH);
    CHECK(c.assess(20.0, 50.0).temp == TempLevel::NORMAL);
}

TEST(comfort_humidity_levels_with_hysteresis) {
    ComfortCalculator c;                            // 70, hysteresis 1
    CHECK(c.assess(25.0, 60.0).humidity == HumidityLevel::NORMAL);
    CHECK(c.assess(25.0, 69.9).humidity == HumidityLevel::NORMAL);
    CHECK(c.assess(25.0, 70.0).humidity == HumidityLevel::HIGH);
    CHECK(c.assess(25.0, 75.0).humidity == HumidityLevel::HIGH);
    CHECK(c.assess(25.0, 69.5).humidity == HumidityLevel::HIGH);    // not below 69 yet
    CHECK(c.assess(25.0, 68.9).humidity == HumidityLevel::NORMAL);
}

TEST(comfort_hysteresis_prevents_chatter) {
    ComfortCalculator withMargin;
    CHECK(countTempChanges(withMargin, {27.9, 28.1, 27.9, 28.1, 27.9}) == 1);

    ComfortConfig cfg;
    cfg.hysteresis = 0.0;
    ComfortCalculator withoutMargin(cfg);
    CHECK(countTempChanges(withoutMargin, {27.9, 28.1, 27.9, 28.1, 27.9}) == 4);
}

TEST(comfort_custom_thresholds_are_respected) {
    ComfortConfig cfg;
    cfg.tempHigh = 20.0;
    cfg.tempVeryHigh = 25.0;
    ComfortCalculator c(cfg);
    CHECK(c.assess(21.0, 50.0).temp == TempLevel::HIGH);
    CHECK(c.assess(26.0, 50.0).temp == TempLevel::VERY_HIGH);
}

// ---------- ComfortStateMachine ----------

TEST(state_machine_maps_every_combination) {
    struct Row {
        TempLevel t;
        HumidityLevel h;
        ComfortState expected;
    };
    const Row rows[] = {
        {TempLevel::NORMAL,    HumidityLevel::NORMAL, ComfortState::COMFORTABLE},
        {TempLevel::NORMAL,    HumidityLevel::HIGH,   ComfortState::HUMID},
        {TempLevel::HIGH,      HumidityLevel::NORMAL, ComfortState::WARM},
        {TempLevel::HIGH,      HumidityLevel::HIGH,   ComfortState::VERY_UNCOMFORTABLE},
        {TempLevel::VERY_HIGH, HumidityLevel::NORMAL, ComfortState::VERY_UNCOMFORTABLE},
        {TempLevel::VERY_HIGH, HumidityLevel::HIGH,   ComfortState::VERY_UNCOMFORTABLE},
    };
    for (const Row& r : rows) {
        ComfortStateMachine sm;
        sm.update({r.t, r.h});
        CHECK(sm.state() == r.expected);
    }
}

TEST(state_machine_reports_transitions) {
    ComfortStateMachine sm;
    const StateTransition a = sm.update({TempLevel::NORMAL, HumidityLevel::NORMAL});
    CHECK(!a.changed);
    const StateTransition b = sm.update({TempLevel::HIGH, HumidityLevel::NORMAL});
    CHECK(b.changed);
    CHECK(b.from == ComfortState::COMFORTABLE);
    CHECK(b.to == ComfortState::WARM);
    const StateTransition c = sm.update({TempLevel::HIGH, HumidityLevel::NORMAL});
    CHECK(!c.changed);
    CHECK(sm.transitionCount() == 1);
}

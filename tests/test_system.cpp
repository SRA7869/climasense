#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include "Config.h"
#include "EventBus.h"
#include "Monitor.h"
#include "ThreadSafeQueue.h"
#include "test.h"

using namespace std::chrono_literals;

namespace {

const char* kTmp = "test_tmp.conf";

void writeFile(const std::string& text) {
    std::ofstream out(kTmp);
    out << text;
}

bool has(const std::string& text, const char* part) {
    return text.find(part) != std::string::npos;
}

int newlines(const std::string& s) {
    return static_cast<int>(std::count(s.begin(), s.end(), '\n'));
}

}  // namespace

// ---------- Config ----------

TEST(config_missing_file_keeps_defaults) {
    SystemConfig cfg;
    const ConfigReport rep = loadConfig("definitely_missing.conf", cfg);
    CHECK(!rep.fileFound);
    CHECK(rep.problems.empty());
    CHECK_NEAR(cfg.comfort.tempHigh, 28.0, 1e-9);
}

TEST(config_loads_valid_values) {
    writeFile("temp_high = 26.5\n"
              "# a comment line\n"
              "humidity_high = 60 # inline comment\n"
              "\n"
              "fan_runs_with_ac = false\n");
    SystemConfig cfg;
    const ConfigReport rep = loadConfig(kTmp, cfg);
    std::remove(kTmp);
    CHECK(rep.fileFound);
    CHECK(rep.problems.empty());
    CHECK_NEAR(cfg.comfort.tempHigh, 26.5, 1e-9);
    CHECK_NEAR(cfg.comfort.humidityHigh, 60.0, 1e-9);
    CHECK(!cfg.automation.fanRunsWithAc);
    CHECK_NEAR(cfg.comfort.tempVeryHigh, 33.0, 1e-9);   // untouched default
}

TEST(config_reports_bad_lines) {
    writeFile("temp_high = abc\n"
              "colour = blue\n"
              "no equals sign here\n");
    SystemConfig cfg;
    const ConfigReport rep = loadConfig(kTmp, cfg);
    std::remove(kTmp);
    CHECK(rep.problems.size() == 3);
    CHECK_NEAR(cfg.comfort.tempHigh, 28.0, 1e-9);
}

TEST(config_rejects_inconsistent_thresholds) {
    writeFile("temp_high = 35\n"
              "temp_very_high = 30\n");
    SystemConfig cfg;
    const ConfigReport rep = loadConfig(kTmp, cfg);
    std::remove(kTmp);
    CHECK(rep.problems.size() == 1);
    CHECK_NEAR(cfg.comfort.tempHigh, 28.0, 1e-9);       // fell back to defaults
    CHECK_NEAR(cfg.comfort.tempVeryHigh, 33.0, 1e-9);
}

TEST(config_rejects_hysteresis_larger_than_gap) {
    writeFile("hysteresis = 10\n");
    SystemConfig cfg;
    const ConfigReport rep = loadConfig(kTmp, cfg);
    std::remove(kTmp);
    CHECK(rep.problems.size() == 1);
    CHECK_NEAR(cfg.comfort.hysteresis, 1.0, 1e-9);
}

TEST(config_rejects_nan_value) {
    writeFile("temp_high = nan\n");
    SystemConfig cfg;
    const ConfigReport rep = loadConfig(kTmp, cfg);
    std::remove(kTmp);
    CHECK(rep.problems.size() == 1);
    CHECK_NEAR(cfg.comfort.tempHigh, 28.0, 1e-9);
}

// ---------- EventBus ----------

TEST(events_are_delivered_in_order) {
    EventBus bus;
    std::vector<std::string> seen;
    bus.subscribeAll([&seen](const Event& e) { seen.push_back(e.message); });
    bus.publish(makeEvent(EventType::SYSTEM_STARTED, "a"));
    bus.publish(makeEvent(EventType::SYSTEM_STARTED, "b"));
    bus.publish(makeEvent(EventType::SYSTEM_STARTED, "c"));
    CHECK(bus.pending() == 3);
    CHECK(seen.empty());                            // publish only queues
    CHECK(bus.dispatchAll() == 3);
    CHECK(bus.pending() == 0);
    CHECK((seen == std::vector<std::string>{"a", "b", "c"}));
}

TEST(events_typed_subscribers_get_only_their_type) {
    EventBus bus;
    int all = 0;
    int typed = 0;
    bus.subscribeAll([&all](const Event&) { ++all; });
    bus.subscribe(EventType::STATE_CHANGED, [&typed](const Event&) { ++typed; });
    bus.publish(makeEvent(EventType::SYSTEM_STARTED, "x"));
    bus.publish(makeEvent(EventType::STATE_CHANGED, "x"));
    bus.publish(makeEvent(EventType::SENSOR_INVALID, "x"));
    bus.publish(makeEvent(EventType::STATE_CHANGED, "x"));
    bus.dispatchAll();
    CHECK(all == 4);
    CHECK(typed == 2);
}

TEST(events_handlers_can_publish_follow_ups) {
    EventBus bus;
    int delivered = 0;
    bus.subscribeAll([&delivered](const Event&) { ++delivered; });
    bus.subscribe(EventType::SYSTEM_STARTED, [&bus](const Event&) {
        bus.publish(makeEvent(EventType::SENSOR_INVALID, "follow-up"));
    });
    bus.publish(makeEvent(EventType::SYSTEM_STARTED, "start"));
    CHECK(bus.dispatchAll() == 2);                  // the follow-up is delivered in the same call
    CHECK(delivered == 2);
}

// ---------- ThreadSafeQueue ----------

TEST(queue_delivers_in_order_and_drains_after_close) {
    ThreadSafeQueue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);
    q.close();
    int v = 0;
    CHECK(q.pop(v));
    CHECK(v == 1);
    CHECK(q.pop(v));
    CHECK(v == 2);
    CHECK(q.pop(v));
    CHECK(v == 3);
    CHECK(!q.pop(v));                               // closed and empty: consumer stops
}

TEST(queue_works_across_threads) {
    ThreadSafeQueue<int> q;
    std::thread producer([&q] {
        for (int i = 1; i <= 1000; ++i) q.push(i);
        q.close();
    });
    long long sum = 0;
    int count = 0;
    int v = 0;
    while (q.pop(v)) {
        sum += v;
        ++count;
    }
    producer.join();
    CHECK(sum == 500500);
    CHECK(count == 1000);
}

TEST(queue_close_wakes_waiting_consumer) {
    ThreadSafeQueue<int> q;
    bool gotItem = true;
    std::thread consumer([&] {
        int v = 0;
        gotItem = q.pop(v);                         // blocks: the queue is empty
    });
    std::this_thread::sleep_for(50ms);
    q.close();
    consumer.join();                                // would hang forever if close() did not wake it
    CHECK(!gotItem);
}

// ---------- Statistics and dashboard ----------

TEST(stats_running_stat_tracks_min_max_mean) {
    RunningStat r;
    CHECK(r.count == 0);
    CHECK_NEAR(r.mean(), 0.0, 1e-9);
    r.add(4.0);
    r.add(2.0);
    r.add(6.0);
    CHECK(r.count == 3);
    CHECK_NEAR(r.min, 2.0, 1e-9);
    CHECK_NEAR(r.max, 6.0, 1e-9);
    CHECK_NEAR(r.mean(), 4.0, 1e-9);
}

TEST(stats_recent_events_keep_newest_six) {
    Snapshot s;
    for (int i = 1; i <= 10; ++i) s.pushRecent("line" + std::to_string(i));
    CHECK(s.recent.size() == 6);
    CHECK(s.recent.front() == "line5");
    CHECK(s.recent.back() == "line10");
}

TEST(dashboard_shows_key_information) {
    Snapshot s;
    s.temp = 35.2;
    s.state = ComfortState::VERY_UNCOMFORTABLE;
    s.fan = true;
    s.ac = false;
    s.exhaust = true;
    s.tempFailed = true;
    const std::string frame = renderDashboard(s, 12000);
    CHECK(has(frame, "ClimaSense"));
    CHECK(has(frame, "VERY_UNCOMFORTABLE"));
    CHECK(has(frame, "FAN ON"));
    CHECK(has(frame, "AC OFF"));
    CHECK(has(frame, "EXHAUST ON"));
    CHECK(has(frame, "FAILED"));
    CHECK(has(frame, "35.20"));
    CHECK(has(frame, "Uptime 12.0"));
}

TEST(dashboard_frame_height_is_constant) {
    Snapshot empty;
    Snapshot full;
    for (int i = 0; i < 10; ++i) full.pushRecent("event " + std::to_string(i));
    full.stats.samples = 99;
    full.stats.temp.add(30.0);
    CHECK(newlines(renderDashboard(empty, 0)) == 24);       // the live redraw depends on this
    CHECK(newlines(renderDashboard(full, 5000)) == 24);
}

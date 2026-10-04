#pragma once
#include <array>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include "ComfortStateMachine.h"

struct RunningStat {
    long count = 0;
    double min = 0.0;
    double max = 0.0;
    double sum = 0.0;

    void add(double v);
    double mean() const { return count > 0 ? sum / static_cast<double>(count) : 0.0; }
};

struct Stats {
    long samples = 0;
    int invalidReadings = 0;
    int failures = 0;
    int recoveries = 0;
    int stateChanges = 0;
    int deviceSwitches = 0;
    RunningStat temp;
    RunningStat humidity;
    std::array<double, 4> secondsInState{};
};

struct Snapshot {
    static constexpr std::size_t kRecent = 6;

    double temp = 0.0;
    double humidity = 0.0;
    bool tempFailed = false;
    bool humidityFailed = false;
    ComfortState state = ComfortState::COMFORTABLE;
    bool fan = false;
    bool ac = false;
    bool exhaust = false;
    Stats stats;
    std::deque<std::string> recent;

    void pushRecent(std::string line);
};

class StatusBoard {
public:
    void update(const std::function<void(Snapshot&)>& change);
    Snapshot snapshot() const;

private:
    mutable std::mutex m_;
    Snapshot s_;
};

std::string renderDashboard(const Snapshot& s, long long uptimeMs);

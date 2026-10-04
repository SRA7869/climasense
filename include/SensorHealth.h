#pragma once

enum class HealthChange { NONE, FAILED, RECOVERED };

const char* toString(HealthChange c);

class SensorHealth {
public:
    explicit SensorHealth(int failAfter = 3, int recoverAfter = 3);

    HealthChange update(bool readingOk);
    bool failed() const { return failed_; }

private:
    int failAfter_;
    int recoverAfter_;
    bool failed_ = false;
    int badStreak_ = 0;
    int goodStreak_ = 0;
};

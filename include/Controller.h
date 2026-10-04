#pragma once
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "AutomationEngine.h"
#include "ComfortCalculator.h"
#include "ComfortStateMachine.h"
#include "Config.h"
#include "DeviceManager.h"
#include "EventBus.h"
#include "ISensor.h"
#include "Logger.h"
#include "Monitor.h"
#include "SensorHealth.h"
#include "SensorManager.h"
#include "ThreadSafeQueue.h"
#include "Validator.h"

class Controller {
public:
    using Clock = std::chrono::steady_clock;

    explicit Controller(SystemConfig config, const std::string& logPath = "",
                        std::chrono::milliseconds samplePeriod = std::chrono::milliseconds(200));
    ~Controller();
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

    void addSensor(std::unique_ptr<ISensor> sensor);
    void note(const std::string& text);
    bool logging() const { return logger_ && logger_->isOpen(); }

    void start();
    void stop();
    bool running() const { return running_; }

    void processBatch(const std::vector<Reading>& batch, Clock::time_point now);

    Snapshot snapshot() const { return board_.snapshot(); }
    long long uptimeMs() const;
    void mark(const std::string& text);

    int processed() const { return processed_; }
    const ComfortStateMachine& machine() const { return machine_; }
    const DeviceManager& devices() const { return devices_; }

private:
    bool judge(const Reading& r, Validator& val, SensorHealth& health,
               Clock::time_point now);
    std::string stamp(Clock::time_point when, const std::string& text) const;

    Clock::time_point start_;
    std::chrono::milliseconds period_;
    std::unique_ptr<Logger> logger_;
    SensorManager manager_;
    ThreadSafeQueue<std::vector<Reading>> samples_;
    std::atomic<bool> running_{false};
    std::thread sensorThread_;
    std::thread controlThread_;

    Validator tempVal_{Limits{-40.0, 80.0}};
    Validator humVal_{Limits{0.0, 100.0}};
    SensorHealth tempHealth_;
    SensorHealth humHealth_;
    ComfortCalculator calc_;
    AutomationEngine engine_;
    ComfortStateMachine machine_;
    DeviceManager devices_;
    EventBus bus_;
    StatusBoard board_;
    int processed_ = 0;
    Clock::time_point lastTick_;
};

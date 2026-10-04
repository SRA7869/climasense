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

    // logPath empty = no log file.
    explicit Controller(SystemConfig config, const std::string& logPath = "",
                        std::chrono::milliseconds samplePeriod = std::chrono::milliseconds(200));
    ~Controller();
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

    // Setup: call before start().
    void addSensor(std::unique_ptr<ISensor> sensor);
    void note(const std::string& text);     // only while stopped: before start() or after stop()
    bool logging() const { return logger_ && logger_->isOpen(); }

    void start();
    void stop();                            // safe to call twice, or without start()
    bool running() const { return running_; }

    // One control cycle. The control thread calls this in a loop; tests call it directly.
    void processBatch(const std::vector<Reading>& batch, Clock::time_point now);

    // Safe to call while running.
    Snapshot snapshot() const { return board_.snapshot(); }
    long long uptimeMs() const;
    void mark(const std::string& text);     // adds a line to the recent-events panel

    // Read these only after stop(), or when the controller was never started.
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

    // Touched only by processBatch(), so only by one thread at a time.
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

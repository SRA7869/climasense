#pragma once
#include <atomic>
#include <thread>
#include "Controller.h"

class Dashboard {
public:
    explicit Dashboard(const Controller& controller);
    ~Dashboard();
    Dashboard(const Dashboard&) = delete;
    Dashboard& operator=(const Dashboard&) = delete;

    bool live() const { return tty_; }
    void start();
    void stop();
    void showFinal();

private:
    const Controller& ctl_;
    bool tty_;
    std::atomic<bool> running_{false};
    std::thread thread_;
    int frameLines_ = 0;
};

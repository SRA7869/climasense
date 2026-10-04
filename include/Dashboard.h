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

    bool live() const { return tty_; }   // false when output is a pipe or file
    void start();                        // no-op when output is not a terminal
    void stop();                         // stops redrawing and joins
    void showFinal();                    // draws the last frame once; call after stop()

private:
    const Controller& ctl_;
    bool tty_;
    std::atomic<bool> running_{false};
    std::thread thread_;
    int frameLines_ = 0;
};

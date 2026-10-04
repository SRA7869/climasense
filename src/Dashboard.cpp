#include "Dashboard.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <unistd.h>
#include "Console.h"
#include "Monitor.h"

namespace {

int lineCount(const std::string& frame) {
    return static_cast<int>(std::count(frame.begin(), frame.end(), '\n')) + 1;
}


std::string redraw(const std::string& frame, int prevLines) {
    std::string out;
    if (prevLines > 0) out += "\033[" + std::to_string(prevLines) + "A";
    for (char c : frame) {
        if (c == '\n') out += "\033[K";
        out += c;
    }
    out += "\033[K";
    return out;
}

}

Dashboard::Dashboard(const Controller& controller)
    : ctl_(controller), tty_(isatty(STDOUT_FILENO) != 0) {}

Dashboard::~Dashboard() {
    stop();
}

void Dashboard::start() {
    if (!tty_ || running_.exchange(true)) return;
    thread_ = std::thread([this] {
        while (running_) {
            const std::string frame = renderDashboard(ctl_.snapshot(), ctl_.uptimeMs());
            printLine(redraw(frame, frameLines_));
            frameLines_ = lineCount(frame);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
}

void Dashboard::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

void Dashboard::showFinal() {
    const std::string frame = renderDashboard(ctl_.snapshot(), ctl_.uptimeMs());
    std::cout << (tty_ ? redraw(frame, frameLines_) : frame) << "\n";
}

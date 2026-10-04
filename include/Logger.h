#pragma once
#include <chrono>
#include <fstream>
#include <string>
#include "Event.h"

class Logger {
public:
    Logger(const std::string& path, std::chrono::steady_clock::time_point start);

    bool isOpen() const { return file_.is_open(); }
    void log(const Event& e);               // one line per event
    void note(const std::string& text);     // a line that is not an event

private:
    std::ofstream file_;
    std::chrono::steady_clock::time_point start_;
};

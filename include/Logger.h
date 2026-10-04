#pragma once
#include <chrono>
#include <fstream>
#include <string>
#include "Event.h"

class Logger {
public:
    Logger(const std::string& path, std::chrono::steady_clock::time_point start);

    bool isOpen() const { return file_.is_open(); }
    void log(const Event& e);
    void note(const std::string& text);

private:
    std::ofstream file_;
    std::chrono::steady_clock::time_point start_;
};

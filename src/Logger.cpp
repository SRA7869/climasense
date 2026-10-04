#include "Logger.h"
#include <ctime>
#include <iomanip>

Logger::Logger(const std::string& path, std::chrono::steady_clock::time_point start)
    : file_(path, std::ios::app), start_(start) {
    if (file_) {
        const std::time_t now = std::time(nullptr);
        file_ << "=== run started "
              << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S")
              << " ===\n";
        file_.flush();
    }
}

void Logger::log(const Event& e) {
    if (!file_) return;
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        e.timestamp - start_).count();
    file_ << '[' << std::setw(6) << ms << " ms] "
          << std::left << std::setw(17) << toString(e.type)
          << std::right << e.message << '\n';
    file_.flush();
}

void Logger::note(const std::string& text) {
    if (!file_) return;
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start_).count();
    file_ << '[' << std::setw(6) << ms << " ms] "
          << std::left << std::setw(17) << "NOTE"
          << std::right << text << '\n';
    file_.flush();
}

#pragma once
#include <iostream>
#include <mutex>
#include <string>

inline void printLine(const std::string& s) {
    static std::mutex m;
    std::lock_guard<std::mutex> lock(m);
    std::cout << s << '\n';
}

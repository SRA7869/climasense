#include <algorithm>
#include <cstring>
#include <iostream>
#include "test.h"

int main() {
    auto& tests = registry();
    std::sort(tests.begin(), tests.end(), [](const TestCase& a, const TestCase& b) {
        return std::strcmp(a.name, b.name) < 0;
    });

    int failedTests = 0;
    for (const TestCase& t : tests) {
        const int before = counters().failures;
        t.fn();
        if (counters().failures == before) {
            std::cout << "[  OK  ] " << t.name << "\n";
        } else {
            std::cout << "[ FAIL ] " << t.name << "\n";
            ++failedTests;
        }
    }

    std::cout << "\n" << tests.size() << " tests, " << counters().checks
              << " checks, " << counters().failures << " failed\n";
    return failedTests == 0 ? 0 : 1;
}

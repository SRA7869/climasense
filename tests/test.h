#pragma once
#include <cmath>
#include <iostream>
#include <vector>

struct TestCase {
    const char* name;
    void (*fn)();
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}

struct Counters {
    int checks = 0;
    int failures = 0;
};

inline Counters& counters() {
    static Counters c;
    return c;
}

struct Registrar {
    Registrar(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

#define TEST(name)                                     \
    static void name();                                \
    static Registrar registrar_##name(#name, name);    \
    static void name()

#define CHECK(cond)                                                       \
    do {                                                                  \
        ++counters().checks;                                              \
        if (!(cond)) {                                                    \
            ++counters().failures;                                        \
            std::cout << "    FAILED " << __FILE__ << ":" << __LINE__     \
                      << "  CHECK(" #cond ")\n";                          \
        }                                                                 \
    } while (0)

#define CHECK_NEAR(a, b, eps) CHECK(std::fabs((a) - (b)) <= (eps))

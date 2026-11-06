#pragma once
// Minimal hand-written test framework: registration macro, assertions,
// summary output. No third-party test library.

#include <cstdio>
#include <string>
#include <type_traits>
#include <vector>

namespace testfw {

struct TestCase {
    const char* name;
    void (*fn)();
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> registry;
    return registry;
}

inline int& failureCount() {
    static int count = 0;
    return count;
}

struct Registrar {
    Registrar(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

inline int runAll() {
    int failedTests = 0;
    for (const TestCase& testCase : registry()) {
        const int failuresBefore = failureCount();
        testCase.fn();
        const bool ok = failureCount() == failuresBefore;
        std::printf("%-4s %s\n", ok ? "PASS" : "FAIL", testCase.name);
        if (!ok) {
            ++failedTests;
        }
    }
    std::printf("----------------------------------------\n");
    std::printf("%zu tests, %d failed\n", registry().size(), failedTests);
    return failedTests == 0 ? 0 : 1;
}

template <typename T>
std::string toText(const T& value) {
    if constexpr (std::is_arithmetic_v<std::decay_t<T>>) {
        return std::to_string(value);
    } else {
        return std::string(value);
    }
}

}  // namespace testfw

#define TEST(name)                                               \
    static void test_##name();                                   \
    static ::testfw::Registrar reg_##name(#name, &test_##name);  \
    static void test_##name()

#define CHECK(condition)                                                      \
    do {                                                                      \
        if (!(condition)) {                                                   \
            ++::testfw::failureCount();                                       \
            std::printf("  CHECK failed: %s (%s:%d)\n", #condition, __FILE__, \
                        __LINE__);                                            \
        }                                                                     \
    } while (false)

#define CHECK_EQ(actual, expected)                                            \
    do {                                                                      \
        auto&& checkEqActual = (actual);                                      \
        auto&& checkEqExpected = (expected);                                  \
        if (!(checkEqActual == checkEqExpected)) {                            \
            ++::testfw::failureCount();                                       \
            std::printf(                                                      \
                "  CHECK_EQ failed: %s == %s\n    actual:   %s\n    "         \
                "expected: %s (%s:%d)\n",                                     \
                #actual, #expected,                                           \
                ::testfw::toText(checkEqActual).c_str(),                      \
                ::testfw::toText(checkEqExpected).c_str(), __FILE__,          \
                __LINE__);                                                    \
        }                                                                     \
    } while (false)

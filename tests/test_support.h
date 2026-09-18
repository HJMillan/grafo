#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include <iostream>
#include <string>

inline int g_failed = 0;
inline int g_passed = 0;

inline void check(bool cond, std::string &errors, const std::string &detail) {
    if (cond) return;
    if (!errors.empty()) errors += "; ";
    errors += detail;
}

inline void finishCase(const char *id, const std::string &errors) {
    if (errors.empty()) {
        ++g_passed;
        std::cout << "  PASS  " << id << "\n";
        return;
    }
    ++g_failed;
    std::cerr << "  FAIL  " << id << " — " << errors << "\n";
}

#endif

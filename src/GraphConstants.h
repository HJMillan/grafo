#ifndef GRAPHCONSTANTS_H
#define GRAPHCONSTANTS_H

#include <cmath>

namespace GraphConstants {

constexpr double INF = 1e15;
constexpr double kEps = 1e-12;

inline bool isMissing(double w, double inf = INF) {
    return w >= inf / 2;
}

inline bool sameWeight(double a, double b) {
    return std::abs(a - b) < kEps;
}

} // namespace GraphConstants

#endif

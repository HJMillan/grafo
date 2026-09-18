#ifndef GRAPHFORMAT_H
#define GRAPHFORMAT_H

#include "GraphConstants.h"

#include <QString>
#include <cmath>

inline QString formatWeight(double w) {
    if (std::abs(w - std::round(w)) < GraphConstants::kEps) {
        return QString::number(static_cast<qint64>(std::llround(w)));
    }
    return QString::number(w, 'f', 2);
}

#endif

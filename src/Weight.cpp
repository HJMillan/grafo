#include "Weight.h"

#include <QLocale>
#include <cmath>

WeightParse parseWeight(const QString &text) {
    WeightParse r;
    QString s = text.trimmed();
    if (s.isEmpty()) return r;

    s.replace(QLatin1Char(','), QLatin1Char('.'));
    if (s.count(QLatin1Char('.')) > 1) {
        r.error = WeightParse::NotNumber;
        return r;
    }

    bool ok = false;
    const double v = QLocale::c().toDouble(s, &ok);
    if (!ok) {
        r.error = WeightParse::NotNumber;
        return r;
    }
    if (!std::isfinite(v)) {
        r.error = WeightParse::NotFinite;
        return r;
    }
    if (std::abs(v) > WeightParse::MaxAbs) {
        r.error = WeightParse::OutOfRange;
        return r;
    }
    r.error = WeightParse::None;
    r.value = v;
    return r;
}

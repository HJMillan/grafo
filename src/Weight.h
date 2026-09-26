#ifndef WEIGHT_H
#define WEIGHT_H

#include <QString>

// Interpretación del texto del campo «Peso».
struct WeightParse {
    enum Error { None, Empty, NotNumber, NotFinite, OutOfRange };

    static constexpr double MaxAbs = 1e9;

    Error error = Empty;
    double value = 0.0;

    bool ok() const { return error == None; }
};

// Acepta coma o punto como separador decimal (2,5 y 2.5), sin separador de miles.
WeightParse parseWeight(const QString &text);

#endif // WEIGHT_H

#pragma once

#include <QImage>
#include <QString>
#include <QStringList>
#include <QMap>
#include <vector>

// Desktop-side wrapper around pigmenta_core: describes the available
// filters and their adjustable parameters, and applies them to a QImage.

namespace pigmenta::desktop {

enum class FilterId {
    Original,
    Decorrelation,
    Clahe,
    Gamma,
    Saturation,
    Relief,
    IrSimulate,
};

enum class ParamKind { Float, Choice };

struct ParamSpec {
    QString name;   // e.g. "intensity"
    QString label;  // e.g. "Intensity"
    ParamKind kind = ParamKind::Float;
    double min = 0.0;
    double max = 1.0;
    double step = 0.01;
    double defaultValue = 0.0;
    QStringList choices;  // used when kind == Choice
};

struct FilterSpec {
    FilterId id;
    QString name;
    std::vector<ParamSpec> params;
};

// All filters exposed in the desktop UI, in display order.
const std::vector<FilterSpec>& filterSpecs();

// Applies filterId to a copy of src (converted to RGBA8888) using the given
// parameter values (keyed by ParamSpec::name) and returns the result.
// For FilterId::Original, returns src unmodified (converted to RGBA8888).
QImage applyFilter(const QImage& src, FilterId filterId, const QMap<QString, double>& values);

}  // namespace pigmenta::desktop

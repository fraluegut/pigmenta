#include <catch2/catch_test_macros.hpp>
#include <QSet>
#include <QColor>
#include "Engine.h"

using namespace pigmenta::desktop;

TEST_CASE("filterSpecs lists every FilterId exactly once, with a non-empty name", "[engine]") {
    const auto& specs = filterSpecs();
    REQUIRE(specs.size() == 7);

    QSet<int> seen;
    for (const FilterSpec& spec : specs) {
        REQUIRE_FALSE(spec.name.isEmpty());
        REQUIRE_FALSE(seen.contains(static_cast<int>(spec.id)));
        seen.insert(static_cast<int>(spec.id));
    }
}

TEST_CASE("the Original filter has no parameters", "[engine]") {
    for (const FilterSpec& spec : filterSpecs()) {
        if (spec.id == FilterId::Original) {
            REQUIRE(spec.params.empty());
            return;
        }
    }
    FAIL("FilterId::Original missing from filterSpecs()");
}

TEST_CASE("every non-Original filter has at least one parameter", "[engine]") {
    for (const FilterSpec& spec : filterSpecs()) {
        if (spec.id != FilterId::Original) {
            REQUIRE_FALSE(spec.params.empty());
        }
    }
}

TEST_CASE("applyFilter(Original) returns an unmodified copy", "[engine]") {
    QImage src(4, 4, QImage::Format_RGB32);
    src.fill(QColor(10, 20, 30));

    QImage out = applyFilter(src, FilterId::Original, {});

    REQUIRE(out.size() == src.size());
    REQUIRE(out.pixelColor(0, 0) == QColor(10, 20, 30));
}

TEST_CASE("applyFilter preserves image dimensions for every filter", "[engine]") {
    QImage src(6, 5, QImage::Format_RGB32);
    src.fill(QColor(80, 120, 200));

    for (const FilterSpec& spec : filterSpecs()) {
        QMap<QString, double> values;
        for (const ParamSpec& p : spec.params)
            values[p.name] = p.defaultValue;

        QImage out = applyFilter(src, spec.id, values);
        REQUIRE(out.width() == src.width());
        REQUIRE(out.height() == src.height());
    }
}

TEST_CASE("applyFilter(Gamma) with gamma=1 is a near-identity", "[engine]") {
    QImage src(2, 2, QImage::Format_RGB32);
    src.fill(QColor(128, 64, 32));

    QImage out = applyFilter(src, FilterId::Gamma, {{"gamma", 1.0}});

    QColor outColor = out.pixelColor(0, 0);
    REQUIRE(std::abs(outColor.red() - 128) <= 1);
    REQUIRE(std::abs(outColor.green() - 64) <= 1);
    REQUIRE(std::abs(outColor.blue() - 32) <= 1);
}

TEST_CASE("applyFilter(Decorrelation) accepts every colorspace index", "[engine]") {
    QImage src(3, 3, QImage::Format_RGB32);
    src.fill(QColor(90, 60, 30));

    for (double csIndex : {0.0, 1.0, 2.0, 3.0, 4.0}) {
        REQUIRE_NOTHROW(applyFilter(src, FilterId::Decorrelation, {{"colorspace", csIndex}}));
    }
}

TEST_CASE("applyFilter works with missing values (falls back to defaults)", "[engine]") {
    QImage src(3, 3, QImage::Format_RGB32);
    src.fill(QColor(50, 50, 50));
    REQUIRE_NOTHROW(applyFilter(src, FilterId::Decorrelation, {}));
}

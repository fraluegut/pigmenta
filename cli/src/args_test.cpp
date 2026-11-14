#include <catch2/catch_test_macros.hpp>
#include "args.h"

using namespace pigmenta::cli;
using pigmenta::ColorSpace;

TEST_CASE("parse_colorspace maps every known name", "[args]") {
    REQUIRE(parse_colorspace("ycbcr") == ColorSpace::YCBCR);
    REQUIRE(parse_colorspace("lab") == ColorSpace::LAB);
    REQUIRE(parse_colorspace("rgb") == ColorSpace::RGB);
    REQUIRE(parse_colorspace("yrd") == ColorSpace::YRD);
    REQUIRE(parse_colorspace("vds") == ColorSpace::VDS);
}

TEST_CASE("parse_colorspace rejects unknown names", "[args]") {
    REQUIRE_FALSE(parse_colorspace("xyz").has_value());
    REQUIRE_FALSE(parse_colorspace("").has_value());
    REQUIRE_FALSE(parse_colorspace("LAB").has_value());  // case-sensitive
}

TEST_CASE("has_jpg_extension", "[args]") {
    REQUIRE(has_jpg_extension("out.jpg"));
    REQUIRE(has_jpg_extension("/a/b/c.jpg"));
    REQUIRE_FALSE(has_jpg_extension("out.png"));
    REQUIRE_FALSE(has_jpg_extension("out.jpeg"));
    REQUIRE_FALSE(has_jpg_extension("jpg"));
    REQUIRE_FALSE(has_jpg_extension(""));
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "pigmenta/color_spaces.h"

using namespace pigmenta;
using Catch::Approx;

namespace {
void require_close(Vec3 a, Vec3 b, float margin = 1e-3f) {
    REQUIRE(a.x == Approx(b.x).margin(margin));
    REQUIRE(a.y == Approx(b.y).margin(margin));
    REQUIRE(a.z == Approx(b.z).margin(margin));
}

const Vec3 kSamples[] = {
    {0.0f, 0.0f, 0.0f},  // black
    {1.0f, 1.0f, 1.0f},  // white
    {1.0f, 0.0f, 0.0f},  // red
    {0.0f, 1.0f, 0.0f},  // green
    {0.0f, 0.0f, 1.0f},  // blue
    {0.5f, 0.3f, 0.8f},  // arbitrary
    {0.2f, 0.9f, 0.1f},  // arbitrary
};
}  // namespace

TEST_CASE("clamp01", "[color_spaces]") {
    REQUIRE(clamp01(-1.0f) == Approx(0.0f));
    REQUIRE(clamp01(0.0f) == Approx(0.0f));
    REQUIRE(clamp01(0.5f) == Approx(0.5f));
    REQUIRE(clamp01(1.0f) == Approx(1.0f));
    REQUIRE(clamp01(2.0f) == Approx(1.0f));
}

TEST_CASE("clamp", "[color_spaces]") {
    REQUIRE(clamp(-5.0f, 0.0f, 10.0f) == Approx(0.0f));
    REQUIRE(clamp(5.0f, 0.0f, 10.0f) == Approx(5.0f));
    REQUIRE(clamp(15.0f, 0.0f, 10.0f) == Approx(10.0f));
}

TEST_CASE("rgb_to_ycbcr known values", "[color_spaces]") {
    // Pure white: Y=1, Cb=Cr=0 (the BT.601 coefficients sum to exactly 1/0/0).
    Vec3 white_ycc = rgb_to_ycbcr({1.0f, 1.0f, 1.0f});
    REQUIRE(white_ycc.x == Approx(1.0f).margin(1e-4));
    REQUIRE(white_ycc.y == Approx(0.0f).margin(1e-4));
    REQUIRE(white_ycc.z == Approx(0.0f).margin(1e-4));

    // Pure black: everything zero.
    Vec3 black_ycc = rgb_to_ycbcr({0.0f, 0.0f, 0.0f});
    require_close(black_ycc, {0.0f, 0.0f, 0.0f});
}

TEST_CASE("ycbcr round trip", "[color_spaces]") {
    for (Vec3 rgb : kSamples) {
        Vec3 ycc = rgb_to_ycbcr(rgb);
        Vec3 back = ycbcr_to_rgb(ycc);
        require_close(back, rgb, 1e-3f);
    }
}

TEST_CASE("ycbcr_to_rgb clips out-of-range input", "[color_spaces]") {
    // Y/Cb/Cr outside the valid gamut must still produce clamped [0,1] RGB.
    Vec3 rgb = ycbcr_to_rgb({2.0f, 5.0f, -5.0f});
    REQUIRE(rgb.x >= 0.0f);
    REQUIRE(rgb.x <= 1.0f);
    REQUIRE(rgb.y >= 0.0f);
    REQUIRE(rgb.y <= 1.0f);
    REQUIRE(rgb.z >= 0.0f);
    REQUIRE(rgb.z <= 1.0f);
}

TEST_CASE("rgb_to_lab known values", "[color_spaces]") {
    // Pure white -> L*=100, a*=b*=0. Pure black -> L*=0.
    Vec3 white_lab = rgb_to_lab({1.0f, 1.0f, 1.0f});
    REQUIRE(white_lab.x == Approx(100.0f).margin(0.1));
    REQUIRE(white_lab.y == Approx(0.0f).margin(0.1));
    REQUIRE(white_lab.z == Approx(0.0f).margin(0.1));

    Vec3 black_lab = rgb_to_lab({0.0f, 0.0f, 0.0f});
    REQUIRE(black_lab.x == Approx(0.0f).margin(0.1));
}

TEST_CASE("lab round trip", "[color_spaces]") {
    for (Vec3 rgb : kSamples) {
        Vec3 lab = rgb_to_lab(rgb);
        Vec3 back = lab_to_rgb(lab);
        require_close(back, rgb, 2e-3f);
    }
}

TEST_CASE("rgb_to_lab clamps out-of-range input", "[color_spaces]") {
    // linearize()/f_lab() must not be fed negative or >1 values unclamped.
    Vec3 lab_over = rgb_to_lab({1.5f, -0.5f, 2.0f});
    REQUIRE(lab_over.x >= 0.0f);
    REQUIRE(lab_over.x <= 100.5f);
}

TEST_CASE("lab_to_rgb exercises both linearize/delinearize branches", "[color_spaces]") {
    // f_lab_inv has a piecewise branch at t <= delta (~0.2069); L*=0 forces
    // fy below that threshold, L*=100 keeps it above.
    Vec3 near_black = lab_to_rgb({0.5f, 0.0f, 0.0f});
    Vec3 near_white = lab_to_rgb({99.5f, 0.0f, 0.0f});
    REQUIRE(near_black.x < near_white.x);

    // delinearize()'s own threshold branch (c <= 0.0031308).
    Vec3 very_dark = lab_to_rgb({2.0f, 0.0f, 0.0f});
    REQUIRE(very_dark.x >= 0.0f);
}

TEST_CASE("yrd round trip", "[color_spaces]") {
    for (Vec3 rgb : kSamples) {
        Vec3 yrd = rgb_to_yrd(rgb);
        Vec3 back = yrd_to_rgb(yrd);
        require_close(back, rgb, 1e-3f);
    }
}

TEST_CASE("vds round trip", "[color_spaces]") {
    for (Vec3 rgb : kSamples) {
        Vec3 vds = rgb_to_vds(rgb);
        Vec3 back = vds_to_rgb(vds);
        require_close(back, rgb, 1e-3f);
    }
}

TEST_CASE("yrd and vds are opposite-sign rotations of ycbcr", "[color_spaces]") {
    Vec3 rgb{0.6f, 0.2f, 0.4f};
    Vec3 ycc = rgb_to_ycbcr(rgb);
    Vec3 yrd = rgb_to_yrd(rgb);
    Vec3 vds = rgb_to_vds(rgb);

    // Both preserve the luma channel exactly.
    REQUIRE(yrd.x == Approx(ycc.x));
    REQUIRE(vds.x == Approx(ycc.x));

    // YRD and VDS rotate Cb/Cr by +45 and -45 degrees respectively, so their
    // chroma components are swapped-and-negated relative to each other.
    REQUIRE(yrd.y == Approx(vds.z).margin(1e-4));
    REQUIRE(yrd.z == Approx(vds.y).margin(1e-4));
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "pigmenta/filters.h"
#include <vector>
#include <cstdint>

using namespace pigmenta;
using Catch::Approx;

namespace {

uint32_t pack(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16) |
           (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
}

uint8_t alpha_of(uint32_t p) {
    return (p >> 24) & 0xFF;
}
uint8_t red_of(uint32_t p) {
    return (p >> 16) & 0xFF;
}
uint8_t green_of(uint32_t p) {
    return (p >> 8) & 0xFF;
}
uint8_t blue_of(uint32_t p) {
    return p & 0xFF;
}

// A small image with varied, non-uniform colours — enough to exercise tile
// boundaries (CLAHE) and gradients (relief shading) without being a single
// flat colour.
std::vector<uint32_t> make_test_image(int width, int height) {
    std::vector<uint32_t> px(static_cast<size_t>(width) * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t r = static_cast<uint8_t>((x * 37 + y * 11) % 256);
            uint8_t g = static_cast<uint8_t>((x * 13 + y * 29) % 256);
            uint8_t b = static_cast<uint8_t>((x * 7 + y * 53) % 256);
            px[y * width + x] = pack(200, r, g, b);
        }
    }
    return px;
}

}  // namespace

TEST_CASE("gamma_correct with gamma=1 is a near-identity", "[filters]") {
    auto img = make_test_image(6, 6);
    auto before = img;
    gamma_correct(img.data(), 6, 6, 6 * 4, 1.0f);
    for (size_t i = 0; i < img.size(); ++i) {
        REQUIRE(alpha_of(img[i]) == alpha_of(before[i]));
        REQUIRE(static_cast<int>(red_of(img[i])) == Approx(red_of(before[i])).margin(1));
        REQUIRE(static_cast<int>(green_of(img[i])) == Approx(green_of(before[i])).margin(1));
        REQUIRE(static_cast<int>(blue_of(img[i])) == Approx(blue_of(before[i])).margin(1));
    }
}

TEST_CASE("gamma_correct < 1 brightens, > 1 darkens", "[filters]") {
    std::vector<uint32_t> mid = {pack(255, 128, 128, 128)};
    std::vector<uint32_t> bright = mid;
    std::vector<uint32_t> dark = mid;

    gamma_correct(bright.data(), 1, 1, 4, 0.5f);
    gamma_correct(dark.data(), 1, 1, 4, 2.0f);

    REQUIRE(red_of(bright[0]) > red_of(mid[0]));
    REQUIRE(red_of(dark[0]) < red_of(mid[0]));
    REQUIRE(alpha_of(bright[0]) == 255);
}

TEST_CASE("saturation_boost factor=1 leaves pixels unchanged", "[filters]") {
    std::vector<uint32_t> img = {pack(255, 200, 50, 80), pack(255, 10, 10, 10)};
    auto before = img;
    saturation_boost(img.data(), 2, 1, 2 * 4, 1.0f);
    for (size_t i = 0; i < img.size(); ++i) {
        REQUIRE(static_cast<int>(red_of(img[i])) == Approx(red_of(before[i])).margin(1));
        REQUIRE(static_cast<int>(green_of(img[i])) == Approx(green_of(before[i])).margin(1));
        REQUIRE(static_cast<int>(blue_of(img[i])) == Approx(blue_of(before[i])).margin(1));
    }
}

TEST_CASE("saturation_boost factor=0 fully desaturates to grey", "[filters]") {
    std::vector<uint32_t> img = {pack(255, 200, 50, 80)};
    saturation_boost(img.data(), 1, 1, 4, 0.0f);
    // Grey means R == G == B (within rounding).
    int r = red_of(img[0]), g = green_of(img[0]), b = blue_of(img[0]);
    REQUIRE(std::abs(r - g) <= 1);
    REQUIRE(std::abs(g - b) <= 1);
}

TEST_CASE("saturation_boost handles an achromatic pixel (delta ~ 0)", "[filters]") {
    // R == G == B triggers the `delta > 1e-6f` false branch in rgb_to_hsv_native.
    std::vector<uint32_t> img = {pack(255, 128, 128, 128)};
    auto before = img;
    saturation_boost(img.data(), 1, 1, 4, 2.0f);
    REQUIRE(red_of(img[0]) == red_of(before[0]));
    REQUIRE(green_of(img[0]) == green_of(before[0]));
    REQUIRE(blue_of(img[0]) == blue_of(before[0]));
}

TEST_CASE("saturation_boost hits every hue sector", "[filters]") {
    // One saturated colour per 60-degree HSV sector, each taking a
    // different branch of hsv_to_rgb_native's switch.
    std::vector<uint32_t> img = {
        pack(255, 255, 0, 0),    // hue ~0   (sector 0)
        pack(255, 255, 255, 0),  // hue ~60  (sector 1)
        pack(255, 0, 255, 0),    // hue ~120 (sector 2)
        pack(255, 0, 255, 255),  // hue ~180 (sector 3)
        pack(255, 0, 0, 255),    // hue ~240 (sector 4)
        pack(255, 255, 0, 255),  // hue ~300 (sector 5 / default)
    };
    REQUIRE_NOTHROW(saturation_boost(
        img.data(), static_cast<int>(img.size()), 1, static_cast<int>(img.size()) * 4, 1.3f));
}

TEST_CASE("ir_simulate intensity=0 is an exact identity", "[filters]") {
    std::vector<uint32_t> img = {pack(255, 10, 200, 90), pack(128, 0, 0, 0)};
    auto before = img;
    ir_simulate(img.data(), 2, 1, 2 * 4, 0.0f);
    REQUIRE(img == before);
}

TEST_CASE("ir_simulate intensity=1 shifts channels", "[filters]") {
    std::vector<uint32_t> img = {pack(255, 10, 200, 90)};
    auto before = img;
    ir_simulate(img.data(), 1, 1, 4, 1.0f);
    REQUIRE(alpha_of(img[0]) == alpha_of(before[0]));
    // Green should shrink (channel is scaled down by (1 - 0.2*intensity) at
    // best, more if ir term doesn't offset it) — assert it actually changed.
    REQUIRE(img[0] != before[0]);
}

TEST_CASE("relief_shade strength=0 is an exact identity", "[filters]") {
    auto img = make_test_image(5, 5);
    auto before = img;
    relief_shade(img.data(), 5, 5, 5 * 4, 315.0f, 0.0f);
    for (size_t i = 0; i < img.size(); ++i) {
        REQUIRE(alpha_of(img[i]) == alpha_of(before[i]));
        REQUIRE(static_cast<int>(red_of(img[i])) == Approx(red_of(before[i])).margin(1));
        REQUIRE(static_cast<int>(green_of(img[i])) == Approx(green_of(before[i])).margin(1));
        REQUIRE(static_cast<int>(blue_of(img[i])) == Approx(blue_of(before[i])).margin(1));
    }
}

TEST_CASE("relief_shade strength=1 replaces colour with a relief-shaded grey", "[filters]") {
    auto img = make_test_image(8, 8);
    relief_shade(img.data(), 8, 8, 8 * 4, 45.0f, 1.0f);
    for (uint32_t p : img) {
        int r = red_of(p), g = green_of(p), b = blue_of(p);
        REQUIRE(std::abs(r - g) <= 1);
        REQUIRE(std::abs(g - b) <= 1);
    }
}

TEST_CASE("relief_shade on a 1x1 image does not crash (no interior pixels)", "[filters]") {
    std::vector<uint32_t> img = {pack(255, 100, 100, 100)};
    REQUIRE_NOTHROW(relief_shade(img.data(), 1, 1, 4, 0.0f, 0.5f));
}

TEST_CASE("clahe_lab preserves alpha and stays in range", "[filters]") {
    auto img = make_test_image(16, 16);
    auto before = img;
    clahe_lab(img.data(), 16, 16, 16 * 4, 8, 2.0f);
    for (size_t i = 0; i < img.size(); ++i) {
        REQUIRE(alpha_of(img[i]) == alpha_of(before[i]));
    }
}

TEST_CASE("clahe_lab with a single tile covering the whole image", "[filters]") {
    auto img = make_test_image(10, 10);
    // tile_size >= image size collapses the grid to a single tile (nx=ny=1),
    // exercising the lut_at() edge-clamping branch on every pixel.
    REQUIRE_NOTHROW(clahe_lab(img.data(), 10, 10, 10 * 4, 64, 2.0f));
}

TEST_CASE("clahe_lab on a flat-colour image (clipping saturates every bin)", "[filters]") {
    std::vector<uint32_t> img(8 * 8, pack(255, 120, 120, 120));
    REQUIRE_NOTHROW(clahe_lab(img.data(), 8, 8, 8 * 4, 4, 1.0f));
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "pigmenta/decorrelation.h"
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

std::vector<uint32_t> make_test_image(int width, int height) {
    std::vector<uint32_t> px(static_cast<size_t>(width) * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t r = static_cast<uint8_t>((x * 37 + y * 11) % 256);
            uint8_t g = static_cast<uint8_t>((x * 13 + y * 29) % 256);
            uint8_t b = static_cast<uint8_t>((x * 7 + y * 53) % 256);
            px[y * width + x] = pack(210, r, g, b);
        }
    }
    return px;
}

}  // namespace

TEST_CASE("decorrelation_stretch on an empty image is a no-op", "[decorrelation]") {
    std::vector<uint32_t> img;
    REQUIRE_NOTHROW(decorrelation_stretch(img.data(), 0, 0, 0, ColorSpace::LAB, 45.0f, 1.0f));
}

TEST_CASE("decorrelation_stretch intensity=0 is an exact identity", "[decorrelation]") {
    auto img = make_test_image(6, 6);
    auto before = img;
    decorrelation_stretch(img.data(), 6, 6, 6 * 4, ColorSpace::LAB, 45.0f, 0.0f);
    REQUIRE(img == before);
}

TEST_CASE("decorrelation_stretch preserves alpha for every colour space", "[decorrelation]") {
    const ColorSpace spaces[] = {
        ColorSpace::YCBCR,
        ColorSpace::LAB,
        ColorSpace::RGB,
        ColorSpace::YRD,
        ColorSpace::VDS,
    };
    for (ColorSpace cs : spaces) {
        auto img = make_test_image(8, 8);
        auto before = img;
        decorrelation_stretch(img.data(), 8, 8, 8 * 4, cs, 45.0f, 1.0f);
        for (size_t i = 0; i < img.size(); ++i) {
            REQUIRE(alpha_of(img[i]) == alpha_of(before[i]));
        }
    }
}

TEST_CASE("decorrelation_stretch with partial intensity blends toward the original", "[decorrelation]") {
    auto full = make_test_image(6, 6);
    auto half = full;
    auto original = full;

    decorrelation_stretch(full.data(), 6, 6, 6 * 4, ColorSpace::RGB, 60.0f, 1.0f);
    decorrelation_stretch(half.data(), 6, 6, 6 * 4, ColorSpace::RGB, 60.0f, 0.5f);

    // At intensity=0.5 the result must differ both from the untouched
    // original and from the fully-stretched output (unless they happen to
    // coincide, which a 6x6 varied image won't).
    REQUIRE(half != original);
    REQUIRE(half != full);
}

TEST_CASE("decorrelation_stretch on a flat-colour image does not crash (zero variance)", "[decorrelation]") {
    // Covariance is all-zero -> eigenvalues ~0 -> scales divide by the eps
    // floor. Must not produce NaNs or crash.
    std::vector<uint32_t> img(10 * 10, pack(255, 80, 80, 80));
    REQUIRE_NOTHROW(decorrelation_stretch(img.data(), 10, 10, 10 * 4, ColorSpace::LAB, 45.0f, 1.0f));
    for (uint32_t p : img) {
        REQUIRE(alpha_of(p) == 255);
    }
}

TEST_CASE("decorrelation_stretch on a single pixel does not crash (N-1 == 0)", "[decorrelation]") {
    std::vector<uint32_t> img = {pack(255, 50, 60, 70)};
    REQUIRE_NOTHROW(decorrelation_stretch(img.data(), 1, 1, 4, ColorSpace::YCBCR, 45.0f, 1.0f));
}

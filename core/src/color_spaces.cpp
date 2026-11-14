#include "pigmenta/color_spaces.h"
#include <cmath>

namespace pigmenta {

// ── YCbCr (BT.601) ───────────────────────────────────────────────────────────

Vec3 rgb_to_ycbcr(Vec3 r) {
    return {
         0.299f  * r.x + 0.587f  * r.y + 0.114f  * r.z,
        -0.16875f* r.x - 0.33126f* r.y + 0.5f    * r.z,
         0.5f    * r.x - 0.41869f* r.y - 0.08131f* r.z
    };
}

Vec3 ycbcr_to_rgb(Vec3 y) {
    return {
        clamp01(y.x +  1.402f   * y.z),
        clamp01(y.x -  0.34414f * y.y - 0.71414f * y.z),
        clamp01(y.x +  1.772f   * y.y)
    };
}

// ── CIE L*a*b* ───────────────────────────────────────────────────────────────

// sRGB → linear
static float linearize(float c) {
    return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

static float delinearize(float c) {
    return c <= 0.0031308f ? 12.92f * c : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
}

static float f_lab(float t) {
    constexpr float delta = 6.0f / 29.0f;
    constexpr float delta2 = delta * delta;
    constexpr float delta3 = delta2 * delta;
    return t > delta3 ? std::cbrt(t) : t / (3.0f * delta2) + 4.0f / 29.0f;
}

static float f_lab_inv(float t) {
    constexpr float delta = 6.0f / 29.0f;
    return t > delta ? t * t * t : 3.0f * delta * delta * (t - 4.0f / 29.0f);
}

// D65 white point
static constexpr float Xn = 0.95047f, Yn = 1.00000f, Zn = 1.08883f;

Vec3 rgb_to_lab(Vec3 rgb) {
    float r = linearize(clamp01(rgb.x));
    float g = linearize(clamp01(rgb.y));
    float b = linearize(clamp01(rgb.z));

    float X = (0.4124564f * r + 0.3575761f * g + 0.1804375f * b) / Xn;
    float Y = (0.2126729f * r + 0.7151522f * g + 0.0721750f * b) / Yn;
    float Z = (0.0193339f * r + 0.1191920f * g + 0.9503041f * b) / Zn;

    float fx = f_lab(X), fy = f_lab(Y), fz = f_lab(Z);

    return {
        116.0f * fy - 16.0f,
        500.0f * (fx - fy),
        200.0f * (fy - fz)
    };
}

Vec3 lab_to_rgb(Vec3 lab) {
    float fy = (lab.x + 16.0f) / 116.0f;
    float fx = lab.y / 500.0f + fy;
    float fz = fy - lab.z / 200.0f;

    float X = f_lab_inv(fx) * Xn;
    float Y = f_lab_inv(fy) * Yn;
    float Z = f_lab_inv(fz) * Zn;

    float r =  3.2404542f * X - 1.5371385f * Y - 0.4985314f * Z;
    float g = -0.9692660f * X + 1.8760108f * Y + 0.0415560f * Z;
    float b =  0.0556434f * X - 0.2040259f * Y + 1.0572252f * Z;

    return {
        clamp01(delinearize(r)),
        clamp01(delinearize(g)),
        clamp01(delinearize(b))
    };
}

// ── YRD (warm rotation, 45° in Cb/Cr) ────────────────────────────────────────

constexpr float SQRT2_INV = 0.70710678f;

Vec3 rgb_to_yrd(Vec3 rgb) {
    Vec3 ycc = rgb_to_ycbcr(rgb);
    return {
        ycc.x,
         SQRT2_INV * ycc.y + SQRT2_INV * ycc.z,
        -SQRT2_INV * ycc.y + SQRT2_INV * ycc.z
    };
}

Vec3 yrd_to_rgb(Vec3 yrd) {
    Vec3 ycc = {
        yrd.x,
         SQRT2_INV * yrd.y - SQRT2_INV * yrd.z,
         SQRT2_INV * yrd.y + SQRT2_INV * yrd.z
    };
    return ycbcr_to_rgb(ycc);
}

// ── VDS (violet rotation, −45° in Cb/Cr) ─────────────────────────────────────

Vec3 rgb_to_vds(Vec3 rgb) {
    Vec3 ycc = rgb_to_ycbcr(rgb);
    return {
        ycc.x,
        -SQRT2_INV * ycc.y + SQRT2_INV * ycc.z,
         SQRT2_INV * ycc.y + SQRT2_INV * ycc.z
    };
}

Vec3 vds_to_rgb(Vec3 vds) {
    Vec3 ycc = {
        vds.x,
        -SQRT2_INV * vds.y + SQRT2_INV * vds.z,
         SQRT2_INV * vds.y + SQRT2_INV * vds.z
    };
    return ycbcr_to_rgb(ycc);
}

} // namespace pigmenta

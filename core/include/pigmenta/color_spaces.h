#pragma once
/**
 * Color space conversion functions for the Pigmenta native engine.
 * All inputs/outputs use float [0,1] unless noted.
 */

#include "pigmenta/matrix3x3.h"

namespace pigmenta {

// ── RGB ↔ YCbCr (BT.601) ─────────────────────────────────────────────────────

/**
 * Convert a single float RGB pixel [0,1] → YCbCr.
 * Y  ∈ [0, 1],  Cb/Cr ∈ [-0.5, 0.5].
 */
Vec3 rgb_to_ycbcr(Vec3 rgb);

/**
 * Convert YCbCr → RGB [0,1], clipped.
 */
Vec3 ycbcr_to_rgb(Vec3 ycbcr);

// ── RGB ↔ CIE L*a*b* (D65) ───────────────────────────────────────────────────

/**
 * RGB [0,1] → Lab.  L ∈ [0,100], a/b ∈ [-128, 127].
 */
Vec3 rgb_to_lab(Vec3 rgb);

/**
 * Lab → RGB [0,1], clipped.
 */
Vec3 lab_to_rgb(Vec3 lab);

// ── Channel rotation presets ──────────────────────────────────────────────────

/**
 * YRD: YCbCr + 45° rotation in the Cb/Cr plane (warms yellows/reds).
 */
Vec3 rgb_to_yrd(Vec3 rgb);
Vec3 yrd_to_rgb(Vec3 yrd);

/**
 * VDS: YCbCr + −45° rotation (emphasises violet/purple).
 */
Vec3 rgb_to_vds(Vec3 rgb);
Vec3 vds_to_rgb(Vec3 vds);

// ── Utility ───────────────────────────────────────────────────────────────────

inline float clamp01(float v) {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}
inline float clamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

}  // namespace pigmenta

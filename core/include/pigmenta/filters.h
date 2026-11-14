#pragma once
/**
 * Additional image enhancement filters for Pigmenta.
 */

#include <cstdint>

namespace pigmenta {

/**
 * CLAHE-inspired adaptive histogram equalisation on the L* channel.
 * @param tile_size  Grid tile size in pixels.
 * @param clip_limit Histogram clip limit (relative; 1.0 = no clipping).
 */
void clahe_lab(uint32_t* pixels, int width, int height, int stride_bytes, int tile_size, float clip_limit);

/**
 * Gamma correction applied to all channels.
 * gamma < 1 brightens shadows; gamma > 1 darkens.
 */
void gamma_correct(uint32_t* pixels, int width, int height, int stride_bytes, float gamma);

/**
 * Boost HSV saturation by the given factor (clipped to 1).
 */
void saturation_boost(uint32_t* pixels, int width, int height, int stride_bytes, float factor);

/**
 * Relief shading — directional gradient of the L* channel.
 * @param angle_deg  Light direction in degrees (0=E, 90=N, 315=NW).
 * @param strength   Blend strength [0,1].
 */
void relief_shade(uint32_t* pixels, int width, int height, int stride_bytes, float angle_deg, float strength);

/**
 * Near-IR simulation: emphasises iron-oxide pigments.
 */
void ir_simulate(uint32_t* pixels, int width, int height, int stride_bytes, float intensity);

}  // namespace pigmenta

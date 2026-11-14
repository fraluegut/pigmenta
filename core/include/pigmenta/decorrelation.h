#pragma once
/**
 * Decorrelation stretch and related colour enhancement algorithms.
 */

#include <cstdint>

namespace pigmenta {

/**
 * Colour space identifier passed from Kotlin/JNI.
 */
enum class ColorSpace : int {
    YCBCR = 0,
    LAB = 1,
    RGB = 2,
    YRD = 3,
    VDS = 4,
};

/**
 * Apply decorrelation stretch to an ARGB_8888 pixel buffer in-place.
 *
 * @param pixels     Pointer to raw ARGB pixel data (Android Bitmap locked pixels).
 * @param width      Image width in pixels.
 * @param height     Image height in pixels.
 * @param stride     Row stride in bytes.
 * @param cs         Target colour space for the stretch.
 * @param target_std Target standard deviation for each output channel (0–127).
 * @param intensity  Blend factor [0,1]: 0 = original, 1 = fully processed.
 */
void decorrelation_stretch(
    uint32_t* pixels, int width, int height, int stride, ColorSpace cs, float target_std, float intensity);

}  // namespace pigmenta

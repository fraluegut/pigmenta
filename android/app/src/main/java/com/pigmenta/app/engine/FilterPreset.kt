package com.pigmenta.app.engine

import com.pigmenta.app.engine.PigmentaNative.ColorSpace

/**
 * Sealed hierarchy of available filter presets.
 * Each subclass carries the parameters needed to apply the filter.
 */
sealed class FilterPreset(
    val id: Int,
    val displayName: String,
    val description: String,
    val iconRes: Int = 0,
) {

    /** Original — imagen sin ningún filtro aplicado. */
    object Original : FilterPreset(
        id = 0,
        displayName = "Original",
        description = "Imagen sin filtro",
    )

    /** YDS — YCbCr decorrelation, great for red/orange ochre pigments. */
    data class DecorrelationStretch(
        val colorSpace: ColorSpace = ColorSpace.YCBCR,
        val targetStd: Float = defaultTargetStd(ColorSpace.YCBCR),
    ) : FilterPreset(
        id = colorSpace.id + 1,
        displayName = when (colorSpace) {
            ColorSpace.YCBCR -> "YDS"
            ColorSpace.LAB   -> "LDS"
            ColorSpace.RGB   -> "CDS"
            ColorSpace.YRD   -> "YRD"
            ColorSpace.VDS   -> "VDS"
        },
        description = when (colorSpace) {
            ColorSpace.YCBCR -> "Red/orange ochre (YCbCr)"
            ColorSpace.LAB   -> "General purpose (Lab)"
            ColorSpace.RGB   -> "Direct RGB decorrelation"
            ColorSpace.YRD   -> "Warm yellows & reds"
            ColorSpace.VDS   -> "Violet & purple pigments"
        },
    )

    /** Adaptive histogram equalisation on L* channel. */
    data class Clahe(
        val tileSize: Int   = 8,
        val clipLimit: Float = 2f,
        val useHsv: Boolean  = false,
    ) : FilterPreset(
        id = 6,
        displayName = if (useHsv) "CLAHE HSV" else "CLAHE Lab",
        description = "Contrast-limited adaptive equalisation",
    )

    /** Gamma correction — brightens shadows. */
    data class Gamma(val gamma: Float = 0.45f) : FilterPreset(
        id = 8,
        displayName = "Gamma",
        description = "Shadow brightening",
    )

    /** Saturation boost. */
    object Saturation : FilterPreset(
        id = 9,
        displayName = "Saturation",
        description = "Colour saturation boost",
    )

    /** Relief shading for petroglyphs / engravings. */
    data class Relief(val angleDeg: Float = 315f) : FilterPreset(
        id = 10,
        displayName = "Relief",
        description = "Surface texture (engravings)",
    )

    /** Near-IR simulation — highlights iron-oxide pigments. */
    object IrSimulate : FilterPreset(
        id = 11,
        displayName = "IR",
        description = "Near-IR (iron oxides)",
    )

    companion object {
        // Appropriate targetStd per colour space.
        // Lab channels (L*=0-100, a/b=+-128) -> 45 is reasonable.
        // YCbCr/RGB/YRD/VDS channels (0-1 or +-0.5) -> 0.35 avoids over-saturation.
        fun defaultTargetStd(cs: ColorSpace): Float = when (cs) {
            ColorSpace.LAB -> 45f
            else           -> 0.35f
        }

        /** All built-in presets in display order. */
        fun allPresets(): List<FilterPreset> = listOf(
            Original,
            DecorrelationStretch(ColorSpace.YCBCR, defaultTargetStd(ColorSpace.YCBCR)),
            DecorrelationStretch(ColorSpace.LAB,   defaultTargetStd(ColorSpace.LAB)),
            DecorrelationStretch(ColorSpace.YRD,   defaultTargetStd(ColorSpace.YRD)),
            DecorrelationStretch(ColorSpace.VDS,   defaultTargetStd(ColorSpace.VDS)),
            DecorrelationStretch(ColorSpace.RGB,   defaultTargetStd(ColorSpace.RGB)),
            Clahe(),
            IrSimulate,
            Saturation,
            Gamma(),
            Relief(),
        )
    }
}

package com.pigmenta.app.engine

import android.graphics.Bitmap

/**
 * Kotlin interface to the Pigmenta C++ native engine.
 * All processing functions copy [src] into [dst] (same dimensions, ARGB_8888)
 * and apply the filter in-place on [dst].
 */
object PigmentaNative {

    init {
        System.loadLibrary("pigmenta-engine")
    }

    /**
     * Color space identifiers matching the C++ [ColorSpace] enum.
     */
    enum class ColorSpace(val id: Int) {
        YCBCR(0),
        LAB(1),
        RGB(2),
        YRD(3),
        VDS(4),
    }

    // ── Decorrelation stretch ─────────────────────────────────────────────────

    /**
     * Apply decorrelation stretch (DStretch-style) to [src], writing result into [dst].
     *
     * @param colorSpace Working colour space for the stretch.
     * @param targetStd  Per-channel output standard deviation (5–100; default 45).
     * @param intensity  Blend between original (0f) and processed (1f).
     */
    external fun applyDecorrelationStretch(
        src: Bitmap,
        dst: Bitmap,
        colorSpace: Int,
        targetStd: Float,
        intensity: Float,
    ): Boolean

    // ── Other filters ─────────────────────────────────────────────────────────

    external fun applyClahe(src: Bitmap, dst: Bitmap, tileSize: Int, clipLimit: Float): Boolean

    external fun applyGamma(src: Bitmap, dst: Bitmap, gamma: Float): Boolean

    external fun applySaturation(src: Bitmap, dst: Bitmap, factor: Float): Boolean

    external fun applyReliefShade(src: Bitmap, dst: Bitmap, angleDeg: Float, strength: Float): Boolean

    external fun applyIrSimulate(src: Bitmap, dst: Bitmap, intensity: Float): Boolean

    // ── Convenience wrapper ───────────────────────────────────────────────────

    /**
     * Apply [preset] to [src] and return a new Bitmap with the result.
     * The returned Bitmap uses ARGB_8888 config and the same dimensions as [src].
     */
    fun applyPreset(
        src: Bitmap,
        preset: FilterPreset,
        intensity: Float = 1.0f,
    ): Bitmap {
        val argbSrc = src.toArgb8888()
        val dst = Bitmap.createBitmap(argbSrc.width, argbSrc.height, Bitmap.Config.ARGB_8888)

        val ok = when (preset) {
            is FilterPreset.Original ->
                return argbSrc.copy(Bitmap.Config.ARGB_8888, false)
            is FilterPreset.DecorrelationStretch ->
                applyDecorrelationStretch(
                    argbSrc, dst,
                    preset.colorSpace.id,
                    preset.targetStd,
                    intensity,
                )
            is FilterPreset.Clahe ->
                applyClahe(argbSrc, dst, preset.tileSize, preset.clipLimit * (1f + intensity))
            is FilterPreset.Gamma ->
                applyGamma(argbSrc, dst, preset.gamma)
            is FilterPreset.Saturation ->
                applySaturation(argbSrc, dst, 1f + intensity * 3f)
            is FilterPreset.Relief ->
                applyReliefShade(argbSrc, dst, preset.angleDeg, intensity)
            is FilterPreset.IrSimulate ->
                applyIrSimulate(argbSrc, dst, intensity)
        }

        return if (ok) dst else argbSrc.copy(Bitmap.Config.ARGB_8888, false)
    }

    private fun Bitmap.toArgb8888(): Bitmap =
        if (config == Bitmap.Config.ARGB_8888) this
        else copy(Bitmap.Config.ARGB_8888, false)
}

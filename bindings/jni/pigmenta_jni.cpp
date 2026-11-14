/**
 * JNI bridge — connects Kotlin code to the Pigmenta C++ engine.
 *
 * Function name convention: Java_<package_underscores>_<class>_<method>
 * Package: com.pigmenta.app.engine → com_pigmenta_app_engine
 */

#include <jni.h>
#include <android/bitmap.h>
#include <android/log.h>

#include "pigmenta/decorrelation.h"
#include "pigmenta/filters.h"
#include <cstring>

#define LOG_TAG "PigmentaJNI"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

using namespace pigmenta;

// ── Helper: lock bitmap, run op, unlock ──────────────────────────────────────

struct BitmapInfo {
    AndroidBitmapInfo info;
    void* pixels;
    bool ok;
};

static BitmapInfo lock(JNIEnv* env, jobject bitmap) {
    BitmapInfo bi{};
    if (AndroidBitmap_getInfo(env, bitmap, &bi.info) < 0) {
        LOGE("AndroidBitmap_getInfo failed");
        return bi;
    }
    if (bi.info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap must be ARGB_8888");
        return bi;
    }
    if (AndroidBitmap_lockPixels(env, bitmap, &bi.pixels) < 0) {
        LOGE("AndroidBitmap_lockPixels failed");
        return bi;
    }
    bi.ok = true;
    return bi;
}

// ── applyDecorrelationStretch ─────────────────────────────────────────────────

extern "C" JNIEXPORT jboolean JNICALL Java_com_pigmenta_app_engine_PigmentaNative_applyDecorrelationStretch(
    JNIEnv* env,
    jobject /*thiz*/,
    jobject srcBitmap,
    jobject dstBitmap,
    jint colorSpace,  // ColorSpace enum ordinal
    jfloat targetStd,
    jfloat intensity) {
    BitmapInfo src = lock(env, srcBitmap);
    if (!src.ok)
        return JNI_FALSE;

    BitmapInfo dst = lock(env, dstBitmap);
    if (!dst.ok) {
        AndroidBitmap_unlockPixels(env, srcBitmap);
        return JNI_FALSE;
    }

    if (src.info.width != dst.info.width || src.info.height != dst.info.height) {
        LOGE("Source and destination bitmaps must have the same dimensions");
        AndroidBitmap_unlockPixels(env, srcBitmap);
        AndroidBitmap_unlockPixels(env, dstBitmap);
        return JNI_FALSE;
    }

    // Copy src → dst
    int stride_bytes = static_cast<int>(src.info.stride);
    int n_bytes = stride_bytes * static_cast<int>(src.info.height);
    memcpy(dst.pixels, src.pixels, n_bytes);

    AndroidBitmap_unlockPixels(env, srcBitmap);

    // Apply in-place on dst
    decorrelation_stretch(static_cast<uint32_t*>(dst.pixels),
                          static_cast<int>(dst.info.width),
                          static_cast<int>(dst.info.height),
                          stride_bytes,
                          static_cast<ColorSpace>(colorSpace),
                          targetStd,
                          intensity);

    AndroidBitmap_unlockPixels(env, dstBitmap);
    return JNI_TRUE;
}

// ── applyClahe ───────────────────────────────────────────────────────────────

extern "C" JNIEXPORT jboolean JNICALL Java_com_pigmenta_app_engine_PigmentaNative_applyClahe(
    JNIEnv* env, jobject /*thiz*/, jobject srcBitmap, jobject dstBitmap, jint tileSize, jfloat clipLimit) {
    BitmapInfo src = lock(env, srcBitmap);
    if (!src.ok)
        return JNI_FALSE;
    BitmapInfo dst = lock(env, dstBitmap);
    if (!dst.ok) {
        AndroidBitmap_unlockPixels(env, srcBitmap);
        return JNI_FALSE;
    }

    int stride = static_cast<int>(src.info.stride);
    memcpy(dst.pixels, src.pixels, stride * static_cast<int>(src.info.height));
    AndroidBitmap_unlockPixels(env, srcBitmap);

    clahe_lab(static_cast<uint32_t*>(dst.pixels),
              static_cast<int>(dst.info.width),
              static_cast<int>(dst.info.height),
              stride,
              tileSize,
              clipLimit);
    AndroidBitmap_unlockPixels(env, dstBitmap);
    return JNI_TRUE;
}

// ── applyGamma ────────────────────────────────────────────────────────────────

extern "C" JNIEXPORT jboolean JNICALL Java_com_pigmenta_app_engine_PigmentaNative_applyGamma(
    JNIEnv* env, jobject /*thiz*/, jobject srcBitmap, jobject dstBitmap, jfloat gamma) {
    BitmapInfo src = lock(env, srcBitmap);
    if (!src.ok)
        return JNI_FALSE;
    BitmapInfo dst = lock(env, dstBitmap);
    if (!dst.ok) {
        AndroidBitmap_unlockPixels(env, srcBitmap);
        return JNI_FALSE;
    }

    int stride = static_cast<int>(src.info.stride);
    memcpy(dst.pixels, src.pixels, stride * static_cast<int>(src.info.height));
    AndroidBitmap_unlockPixels(env, srcBitmap);

    gamma_correct(static_cast<uint32_t*>(dst.pixels),
                  static_cast<int>(dst.info.width),
                  static_cast<int>(dst.info.height),
                  stride,
                  gamma);
    AndroidBitmap_unlockPixels(env, dstBitmap);
    return JNI_TRUE;
}

// ── applySaturation ───────────────────────────────────────────────────────────

extern "C" JNIEXPORT jboolean JNICALL Java_com_pigmenta_app_engine_PigmentaNative_applySaturation(
    JNIEnv* env, jobject /*thiz*/, jobject srcBitmap, jobject dstBitmap, jfloat factor) {
    BitmapInfo src = lock(env, srcBitmap);
    if (!src.ok)
        return JNI_FALSE;
    BitmapInfo dst = lock(env, dstBitmap);
    if (!dst.ok) {
        AndroidBitmap_unlockPixels(env, srcBitmap);
        return JNI_FALSE;
    }

    int stride = static_cast<int>(src.info.stride);
    memcpy(dst.pixels, src.pixels, stride * static_cast<int>(src.info.height));
    AndroidBitmap_unlockPixels(env, srcBitmap);

    saturation_boost(static_cast<uint32_t*>(dst.pixels),
                     static_cast<int>(dst.info.width),
                     static_cast<int>(dst.info.height),
                     stride,
                     factor);
    AndroidBitmap_unlockPixels(env, dstBitmap);
    return JNI_TRUE;
}

// ── applyReliefShade ──────────────────────────────────────────────────────────

extern "C" JNIEXPORT jboolean JNICALL Java_com_pigmenta_app_engine_PigmentaNative_applyReliefShade(
    JNIEnv* env, jobject /*thiz*/, jobject srcBitmap, jobject dstBitmap, jfloat angleDeg, jfloat strength) {
    BitmapInfo src = lock(env, srcBitmap);
    if (!src.ok)
        return JNI_FALSE;
    BitmapInfo dst = lock(env, dstBitmap);
    if (!dst.ok) {
        AndroidBitmap_unlockPixels(env, srcBitmap);
        return JNI_FALSE;
    }

    int stride = static_cast<int>(src.info.stride);
    memcpy(dst.pixels, src.pixels, stride * static_cast<int>(src.info.height));
    AndroidBitmap_unlockPixels(env, srcBitmap);

    relief_shade(static_cast<uint32_t*>(dst.pixels),
                 static_cast<int>(dst.info.width),
                 static_cast<int>(dst.info.height),
                 stride,
                 angleDeg,
                 strength);
    AndroidBitmap_unlockPixels(env, dstBitmap);
    return JNI_TRUE;
}

// ── applyIrSimulate ───────────────────────────────────────────────────────────

extern "C" JNIEXPORT jboolean JNICALL Java_com_pigmenta_app_engine_PigmentaNative_applyIrSimulate(
    JNIEnv* env, jobject /*thiz*/, jobject srcBitmap, jobject dstBitmap, jfloat intensity) {
    BitmapInfo src = lock(env, srcBitmap);
    if (!src.ok)
        return JNI_FALSE;
    BitmapInfo dst = lock(env, dstBitmap);
    if (!dst.ok) {
        AndroidBitmap_unlockPixels(env, srcBitmap);
        return JNI_FALSE;
    }

    int stride = static_cast<int>(src.info.stride);
    memcpy(dst.pixels, src.pixels, stride * static_cast<int>(src.info.height));
    AndroidBitmap_unlockPixels(env, srcBitmap);

    ir_simulate(static_cast<uint32_t*>(dst.pixels),
                static_cast<int>(dst.info.width),
                static_cast<int>(dst.info.height),
                stride,
                intensity);
    AndroidBitmap_unlockPixels(env, dstBitmap);
    return JNI_TRUE;
}

#include "pigmenta/decorrelation.h"
#include "pigmenta/color_spaces.h"
#include "pigmenta/matrix3x3.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace pigmenta {

// ── ARGB pixel packing/unpacking ─────────────────────────────────────────────

static inline Vec3 unpack_rgb(uint32_t argb) {
    return {
        ((argb >> 16) & 0xFF) / 255.0f,
        ((argb >>  8) & 0xFF) / 255.0f,
        ( argb        & 0xFF) / 255.0f
    };
}

static inline uint32_t pack_rgb(uint8_t a, Vec3 rgb) {
    auto r = static_cast<uint8_t>(clamp(rgb.x, 0.0f, 1.0f) * 255.0f);
    auto g = static_cast<uint8_t>(clamp(rgb.y, 0.0f, 1.0f) * 255.0f);
    auto b = static_cast<uint8_t>(clamp(rgb.z, 0.0f, 1.0f) * 255.0f);
    return (static_cast<uint32_t>(a) << 24) |
           (static_cast<uint32_t>(r) << 16) |
           (static_cast<uint32_t>(g) <<  8) |
            static_cast<uint32_t>(b);
}

// ── Convert all pixels to working colour space ────────────────────────────────

using ToCS   = Vec3(*)(Vec3);
using FromCS = Vec3(*)(Vec3);

static ToCS   get_to_cs(ColorSpace cs) {
    switch (cs) {
        case ColorSpace::YCBCR: return rgb_to_ycbcr;
        case ColorSpace::LAB:   return rgb_to_lab;
        case ColorSpace::YRD:   return rgb_to_yrd;
        case ColorSpace::VDS:   return rgb_to_vds;
        default:                return [](Vec3 v) { return v; }; // RGB identity
    }
}

static FromCS get_from_cs(ColorSpace cs) {
    switch (cs) {
        case ColorSpace::YCBCR: return ycbcr_to_rgb;
        case ColorSpace::LAB:   return lab_to_rgb;
        case ColorSpace::YRD:   return yrd_to_rgb;
        case ColorSpace::VDS:   return vds_to_rgb;
        default:                return [](Vec3 v) { return Vec3{clamp01(v.x), clamp01(v.y), clamp01(v.z)}; };
    }
}

// ── Covariance + mean ─────────────────────────────────────────────────────────

struct Stats {
    Vec3    mean;
    Mat3x3  cov;
};

static Stats compute_stats(const std::vector<Vec3>& pts) {
    const int N = static_cast<int>(pts.size());
    Stats s;

    // Mean
    for (const auto& p : pts) s.mean += p;
    s.mean = s.mean * (1.0f / N);

    // Covariance (upper triangle, symmetric)
    for (const auto& p : pts) {
        float dx = p.x - s.mean.x;
        float dy = p.y - s.mean.y;
        float dz = p.z - s.mean.z;
        s.cov.m[0][0] += dx * dx;
        s.cov.m[1][1] += dy * dy;
        s.cov.m[2][2] += dz * dz;
        s.cov.m[0][1] += dx * dy;
        s.cov.m[0][2] += dx * dz;
        s.cov.m[1][2] += dy * dz;
    }
    float inv = 1.0f / std::max(N - 1, 1);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            s.cov.m[i][j] *= inv;
    // Symmetrise
    s.cov.m[1][0] = s.cov.m[0][1];
    s.cov.m[2][0] = s.cov.m[0][2];
    s.cov.m[2][1] = s.cov.m[1][2];

    return s;
}

// ── Core decorrelation stretch ────────────────────────────────────────────────

void decorrelation_stretch(
    uint32_t*  pixels,
    int        width,
    int        height,
    int        stride_bytes,
    ColorSpace cs,
    float      target_std,
    float      intensity
) {
    const int N = width * height;
    if (N == 0) return;

    ToCS   to_cs   = get_to_cs(cs);
    FromCS from_cs = get_from_cs(cs);

    // 1. Convert all pixels to working colour space
    std::vector<Vec3> working(N);
    std::vector<uint8_t> alphas(N);

    const int stride_px = stride_bytes / 4;
    for (int y = 0; y < height; ++y) {
        const uint32_t* row = pixels + y * stride_px;
        for (int x = 0; x < width; ++x) {
            uint32_t argb = row[x];
            alphas[y * width + x] = static_cast<uint8_t>((argb >> 24) & 0xFF);
            working[y * width + x] = to_cs(unpack_rgb(argb));
        }
    }

    // 2. Compute mean and covariance
    Stats stats = compute_stats(working);

    // 3. Eigen decompose covariance
    Mat3x3 V;
    float  eigenvalues[3];
    jacobi_eigen(stats.cov, V, eigenvalues);

    // V columns are eigenvectors sorted by ascending eigenvalue.
    // We want V as a 3×3 where columns correspond to eigenvectors.
    // V.m[i][j] = component i of eigenvector j.

    // 4. Compute per-component scale: target_std / sqrt(eigenvalue)
    constexpr float eps = 1e-8f;
    float scales[3];
    for (int i = 0; i < 3; ++i)
        scales[i] = target_std / std::sqrt(std::max(eigenvalues[i], eps));

    // 5. Build combined transform: out = V * diag(scales) * V^T * (in - mean) + out_mean
    //    out_mean: centre of RGB space in the working colour space
    Vec3 out_mean = stats.mean;  // keep the same centre

    // 6. Apply transform pixel by pixel
    for (int i = 0; i < N; ++i) {
        Vec3 p = working[i];
        // Centre
        Vec3 c = {p.x - stats.mean.x, p.y - stats.mean.y, p.z - stats.mean.z};

        // Project onto eigenvectors (V^T * c)
        float proj[3];
        for (int j = 0; j < 3; ++j)
            proj[j] = V.m[0][j]*c.x + V.m[1][j]*c.y + V.m[2][j]*c.z;

        // Scale
        for (int j = 0; j < 3; ++j) proj[j] *= scales[j];

        // Back-project (V * proj)
        Vec3 stretched = {
            V.m[0][0]*proj[0] + V.m[0][1]*proj[1] + V.m[0][2]*proj[2],
            V.m[1][0]*proj[0] + V.m[1][1]*proj[1] + V.m[1][2]*proj[2],
            V.m[2][0]*proj[0] + V.m[2][1]*proj[1] + V.m[2][2]*proj[2],
        };

        // Re-centre and convert back to RGB
        Vec3 out_cs = {
            stretched.x + out_mean.x,
            stretched.y + out_mean.y,
            stretched.z + out_mean.z
        };
        Vec3 out_rgb = from_cs(out_cs);

        // Blend with original
        if (intensity < 1.0f) {
            Vec3 orig_rgb = unpack_rgb(
                pixels[(i / width) * stride_px + (i % width)]
            );
            float inv = 1.0f - intensity;
            out_rgb.x = orig_rgb.x * inv + out_rgb.x * intensity;
            out_rgb.y = orig_rgb.y * inv + out_rgb.y * intensity;
            out_rgb.z = orig_rgb.z * inv + out_rgb.z * intensity;
        }

        working[i] = out_rgb;
    }

    // 7. Write back to bitmap buffer
    for (int y = 0; y < height; ++y) {
        uint32_t* row = pixels + y * stride_px;
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            row[x] = pack_rgb(alphas[idx], working[idx]);
        }
    }
}

} // namespace pigmenta

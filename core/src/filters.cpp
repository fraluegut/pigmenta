#include "pigmenta/filters.h"
#include "pigmenta/color_spaces.h"
#include "pigmenta/matrix3x3.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace pigmenta {

// ── Shared pixel helpers ──────────────────────────────────────────────────────

static int stride_px_of(int stride_bytes) { return stride_bytes / 4; }

static Vec3 read_rgb(const uint32_t* pixels, int stride_px, int x, int y) {
    uint32_t argb = pixels[y * stride_px + x];
    return {((argb >> 16) & 0xFF) / 255.0f,
            ((argb >>  8) & 0xFF) / 255.0f,
            ( argb        & 0xFF) / 255.0f};
}

static void write_rgb(uint32_t* pixels, int stride_px, int x, int y,
                      Vec3 rgb, uint8_t alpha = 0xFF) {
    auto r = static_cast<uint8_t>(clamp(rgb.x, 0.0f, 1.0f) * 255.0f);
    auto g = static_cast<uint8_t>(clamp(rgb.y, 0.0f, 1.0f) * 255.0f);
    auto b = static_cast<uint8_t>(clamp(rgb.z, 0.0f, 1.0f) * 255.0f);
    pixels[y * stride_px + x] =
        (static_cast<uint32_t>(alpha) << 24) |
        (static_cast<uint32_t>(r)     << 16) |
        (static_cast<uint32_t>(g)     <<  8) |
         static_cast<uint32_t>(b);
}

static uint8_t read_alpha(const uint32_t* pixels, int stride_px, int x, int y) {
    return static_cast<uint8_t>((pixels[y * stride_px + x] >> 24) & 0xFF);
}

// ── CLAHE ─────────────────────────────────────────────────────────────────────

static std::vector<uint8_t> build_tile_map(
    const std::vector<uint8_t>& data, float clip_limit, int n_bins = 256
) {
    std::vector<int> hist(n_bins, 0);
    for (uint8_t v : data) hist[v]++;

    // Clip and redistribute
    int threshold = static_cast<int>(clip_limit * data.size() / n_bins);
    threshold = std::max(threshold, 1);
    int excess = 0;
    for (int& h : hist) {
        if (h > threshold) { excess += h - threshold; h = threshold; }
    }
    int add = excess / n_bins;
    for (int& h : hist) h += add;

    // CDF → LUT
    std::vector<int> cdf(n_bins);
    std::partial_sum(hist.begin(), hist.end(), cdf.begin());
    int cdf_min = *std::min_element(cdf.begin(), cdf.end());
    int total   = static_cast<int>(data.size());

    std::vector<uint8_t> lut(n_bins);
    for (int i = 0; i < n_bins; ++i)
        lut[i] = static_cast<uint8_t>(
            std::round((cdf[i] - cdf_min) * 255.0f / std::max(total - cdf_min, 1))
        );
    return lut;
}

void clahe_lab(uint32_t* pixels, int width, int height, int stride_bytes,
               int tile_size, float clip_limit) {
    int sp = stride_px_of(stride_bytes);

    // Extract L* channel as uint8 (0–255 mapped from 0–100)
    std::vector<uint8_t> L_ch(width * height);
    std::vector<uint8_t> alphas(width * height);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            alphas[y * width + x] = read_alpha(pixels, sp, x, y);
            Vec3 lab = rgb_to_lab(read_rgb(pixels, sp, x, y));
            L_ch[y * width + x] = static_cast<uint8_t>(
                std::round(clamp(lab.x, 0.0f, 100.0f) * 2.55f)
            );
        }

    // Build tile LUTs
    int nx = (width  + tile_size - 1) / tile_size;
    int ny = (height + tile_size - 1) / tile_size;
    std::vector<std::vector<uint8_t>> luts(nx * ny);

    for (int ty = 0; ty < ny; ++ty)
        for (int tx = 0; tx < nx; ++tx) {
            std::vector<uint8_t> tile_data;
            for (int y = ty * tile_size; y < std::min((ty+1)*tile_size, height); ++y)
                for (int x = tx * tile_size; x < std::min((tx+1)*tile_size, width); ++x)
                    tile_data.push_back(L_ch[y * width + x]);
            luts[ty * nx + tx] = build_tile_map(tile_data, clip_limit);
        }

    // Apply with bilinear interpolation between tile LUTs
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t lv = L_ch[y * width + x];

            float ty_f = static_cast<float>(y) / tile_size - 0.5f;
            float tx_f = static_cast<float>(x) / tile_size - 0.5f;
            int ty0 = static_cast<int>(std::floor(ty_f));
            int tx0 = static_cast<int>(std::floor(tx_f));
            int ty1 = ty0 + 1, tx1 = tx0 + 1;
            float wy = ty_f - ty0, wx = tx_f - tx0;

            auto lut_at = [&](int tya, int txa) -> const std::vector<uint8_t>& {
                int cty = std::max(0, std::min(ny-1, tya));
                int ctx = std::max(0, std::min(nx-1, txa));
                return luts[cty * nx + ctx];
            };

            float v = (1-wy)*(1-wx)* lut_at(ty0,tx0)[lv]
                    + (1-wy)*(  wx)* lut_at(ty0,tx1)[lv]
                    + (  wy)*(1-wx)* lut_at(ty1,tx0)[lv]
                    + (  wy)*(  wx)* lut_at(ty1,tx1)[lv];

            // Write back: change only L*, keep a*, b*
            Vec3 rgb_orig = read_rgb(pixels, sp, x, y);
            Vec3 lab      = rgb_to_lab(rgb_orig);
            lab.x         = clamp(v / 2.55f, 0.0f, 100.0f);
            write_rgb(pixels, sp, x, y, lab_to_rgb(lab), alphas[y * width + x]);
        }
    }
}

// ── Gamma ─────────────────────────────────────────────────────────────────────

void gamma_correct(uint32_t* pixels, int width, int height, int stride_bytes, float gamma) {
    int sp = stride_px_of(stride_bytes);
    // Pre-build LUT for speed
    uint8_t lut[256];
    for (int i = 0; i < 256; ++i)
        lut[i] = static_cast<uint8_t>(std::round(std::pow(i / 255.0f, gamma) * 255.0f));

    for (int y = 0; y < height; ++y) {
        uint32_t* row = pixels + y * sp;
        for (int x = 0; x < width; ++x) {
            uint32_t p = row[x];
            uint8_t a = (p >> 24) & 0xFF;
            uint8_t r = lut[(p >> 16) & 0xFF];
            uint8_t g = lut[(p >>  8) & 0xFF];
            uint8_t b = lut[ p        & 0xFF];
            row[x] = (static_cast<uint32_t>(a) << 24) |
                     (static_cast<uint32_t>(r) << 16) |
                     (static_cast<uint32_t>(g) <<  8) |
                      static_cast<uint32_t>(b);
        }
    }
}

// ── Saturation ────────────────────────────────────────────────────────────────

static Vec3 rgb_to_hsv_native(Vec3 rgb) {
    float r = rgb.x, g = rgb.y, b = rgb.z;
    float cmax = std::max({r, g, b});
    float cmin = std::min({r, g, b});
    float delta = cmax - cmin;

    float h = 0.0f, s = 0.0f, v = cmax;
    if (delta > 1e-6f) {
        s = delta / cmax;
        if      (cmax == r) h = 60.0f * std::fmod((g - b) / delta, 6.0f);
        else if (cmax == g) h = 60.0f * ((b - r) / delta + 2.0f);
        else                h = 60.0f * ((r - g) / delta + 4.0f);
        if (h < 0) h += 360.0f;
    }
    return {h, s, v};
}

static Vec3 hsv_to_rgb_native(Vec3 hsv) {
    float h = hsv.x, s = hsv.y, v = hsv.z;
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r1, g1, b1;
    int hi = static_cast<int>(h / 60.0f) % 6;
    switch (hi) {
        case 0: r1=c; g1=x; b1=0; break;
        case 1: r1=x; g1=c; b1=0; break;
        case 2: r1=0; g1=c; b1=x; break;
        case 3: r1=0; g1=x; b1=c; break;
        case 4: r1=x; g1=0; b1=c; break;
        default:r1=c; g1=0; b1=x; break;
    }
    return {clamp01(r1+m), clamp01(g1+m), clamp01(b1+m)};
}

void saturation_boost(uint32_t* pixels, int width, int height, int stride_bytes, float factor) {
    int sp = stride_px_of(stride_bytes);
    for (int y = 0; y < height; ++y) {
        uint32_t* row = pixels + y * sp;
        for (int x = 0; x < width; ++x) {
            uint32_t p  = row[x];
            uint8_t  a  = (p >> 24) & 0xFF;
            Vec3 rgb    = {((p>>16)&0xFF)/255.0f, ((p>>8)&0xFF)/255.0f, (p&0xFF)/255.0f};
            Vec3 hsv    = rgb_to_hsv_native(rgb);
            hsv.y       = clamp01(hsv.y * factor);
            Vec3 out    = hsv_to_rgb_native(hsv);
            write_rgb(pixels, sp, x, y, out, a);
        }
    }
}

// ── Relief shading ────────────────────────────────────────────────────────────

void relief_shade(uint32_t* pixels, int width, int height, int stride_bytes,
                  float angle_deg, float strength) {
    int sp = stride_px_of(stride_bytes);
    float rad = angle_deg * 3.14159265f / 180.0f;
    float kx  = std::cos(rad);
    float ky  = -std::sin(rad);

    // Extract L* as float
    std::vector<float> L(width * height);
    std::vector<uint8_t> alphas(width * height);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            alphas[y*width+x] = read_alpha(pixels, sp, x, y);
            L[y*width+x] = rgb_to_lab(read_rgb(pixels, sp, x, y)).x;
        }

    // Sobel gradient
    std::vector<float> relief(width * height, 0.0f);
    for (int y = 1; y < height - 1; ++y)
        for (int x = 1; x < width - 1; ++x) {
            float dx = -L[(y-1)*width+(x-1)] + L[(y-1)*width+(x+1)]
                       -2*L[y*width+(x-1)]    + 2*L[y*width+(x+1)]
                       -L[(y+1)*width+(x-1)] + L[(y+1)*width+(x+1)];
            float dy = -L[(y-1)*width+(x-1)] - 2*L[(y-1)*width+x] - L[(y-1)*width+(x+1)]
                       +L[(y+1)*width+(x-1)] + 2*L[(y+1)*width+x] + L[(y+1)*width+(x+1)];
            relief[y*width+x] = kx * dx + ky * dy;
        }

    // Normalise
    float rmin = *std::min_element(relief.begin(), relief.end());
    float rmax = *std::max_element(relief.begin(), relief.end());
    float rrange = rmax - rmin + 1e-8f;

    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            float rv  = (relief[y*width+x] - rmin) / rrange;
            Vec3 orig = read_rgb(pixels, sp, x, y);
            Vec3 grey = {rv, rv, rv};
            Vec3 out  = {
                clamp01(orig.x * (1-strength) + grey.x * strength),
                clamp01(orig.y * (1-strength) + grey.y * strength),
                clamp01(orig.z * (1-strength) + grey.z * strength),
            };
            write_rgb(pixels, sp, x, y, out, alphas[y*width+x]);
        }
}

// ── IR simulation ─────────────────────────────────────────────────────────────

void ir_simulate(uint32_t* pixels, int width, int height, int stride_bytes, float intensity) {
    int sp = stride_px_of(stride_bytes);
    for (int y = 0; y < height; ++y) {
        uint32_t* row = pixels + y * sp;
        for (int x = 0; x < width; ++x) {
            uint32_t p = row[x];
            uint8_t a  = (p >> 24) & 0xFF;
            Vec3 rgb   = {((p>>16)&0xFF)/255.0f, ((p>>8)&0xFF)/255.0f, (p&0xFF)/255.0f};
            float ir   = 0.213f*rgb.x + 0.715f*rgb.y + 0.072f*rgb.z;
            Vec3 out   = {
                clamp01(rgb.x + ir * 0.5f * intensity),
                clamp01(rgb.y * (1.0f - 0.2f * intensity)),
                clamp01(rgb.z * (1.0f - 0.8f * intensity) + ir * 0.2f * intensity),
            };
            write_rgb(pixels, sp, x, y, out, a);
        }
    }
}

} // namespace pigmenta

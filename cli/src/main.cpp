// Pigmenta CLI — batch image processing on top of the pigmenta_core engine.

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "pigmenta/decorrelation.h"
#include "pigmenta/filters.h"
#include "args.h"

using namespace pigmenta;

namespace {

void print_usage(const char* argv0) {
    std::fprintf(stderr,
                 "Pigmenta CLI — rock art image enhancement\n\n"
                 "Usage: %s <input> <output> --filter <name> [options]\n\n"
                 "Filters:\n"
                 "  decorrelation --colorspace <ycbcr|lab|rgb|yrd|vds> --std <float> --intensity <float>\n"
                 "  clahe         --tile <int> --clip <float>\n"
                 "  gamma         --gamma <float>\n"
                 "  saturation    --factor <float>\n"
                 "  relief        --angle <float> --strength <float>\n"
                 "  ir            --intensity <float>\n",
                 argv0);
}

ColorSpace parse_colorspace_or_exit(const std::string& s) {
    auto cs = pigmenta::cli::parse_colorspace(s);
    if (!cs) {
        std::fprintf(stderr, "Unknown colorspace '%s'\n", s.c_str());
        std::exit(1);
    }
    return *cs;
}

float next_float(int argc, char** argv, int& i) {
    if (i + 1 >= argc) {
        std::fprintf(stderr, "Missing value for %s\n", argv[i]);
        std::exit(1);
    }
    return std::strtof(argv[++i], nullptr);
}

std::string next_str(int argc, char** argv, int& i) {
    if (i + 1 >= argc) {
        std::fprintf(stderr, "Missing value for %s\n", argv[i]);
        std::exit(1);
    }
    return argv[++i];
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 5) {
        print_usage(argv[0]);
        return 1;
    }

    const std::string input_path = argv[1];
    const std::string output_path = argv[2];

    if (std::strcmp(argv[3], "--filter") != 0) {
        print_usage(argv[0]);
        return 1;
    }
    const std::string filter = argv[4];

    // Defaults.
    std::string colorspace_str = "ycbcr";
    float target_std = 45.0f;
    float intensity = 1.0f;
    int tile_size = 16;
    float clip_limit = 2.0f;
    float gamma = 1.0f;
    float sat_factor = 2.0f;
    float angle_deg = 315.0f;
    float strength = 0.5f;

    for (int i = 5; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--colorspace")
            colorspace_str = next_str(argc, argv, i);
        else if (arg == "--std")
            target_std = next_float(argc, argv, i);
        else if (arg == "--intensity")
            intensity = next_float(argc, argv, i);
        else if (arg == "--tile")
            tile_size = static_cast<int>(next_float(argc, argv, i));
        else if (arg == "--clip")
            clip_limit = next_float(argc, argv, i);
        else if (arg == "--gamma")
            gamma = next_float(argc, argv, i);
        else if (arg == "--factor")
            sat_factor = next_float(argc, argv, i);
        else if (arg == "--angle")
            angle_deg = next_float(argc, argv, i);
        else if (arg == "--strength")
            strength = next_float(argc, argv, i);
        else {
            std::fprintf(stderr, "Unknown option '%s'\n", arg.c_str());
            return 1;
        }
    }

    int width, height, channels;
    unsigned char* data = stbi_load(input_path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        std::fprintf(stderr, "Failed to load '%s': %s\n", input_path.c_str(), stbi_failure_reason());
        return 1;
    }

    auto* pixels = reinterpret_cast<uint32_t*>(data);
    const int stride_bytes = width * 4;

    if (filter == "decorrelation") {
        decorrelation_stretch(pixels,
                              width,
                              height,
                              stride_bytes,
                              parse_colorspace_or_exit(colorspace_str),
                              target_std,
                              intensity);
    } else if (filter == "clahe") {
        clahe_lab(pixels, width, height, stride_bytes, tile_size, clip_limit);
    } else if (filter == "gamma") {
        gamma_correct(pixels, width, height, stride_bytes, gamma);
    } else if (filter == "saturation") {
        saturation_boost(pixels, width, height, stride_bytes, sat_factor);
    } else if (filter == "relief") {
        relief_shade(pixels, width, height, stride_bytes, angle_deg, strength);
    } else if (filter == "ir") {
        ir_simulate(pixels, width, height, stride_bytes, intensity);
    } else {
        std::fprintf(stderr, "Unknown filter '%s'\n", filter.c_str());
        stbi_image_free(data);
        return 1;
    }

    int ok;
    if (pigmenta::cli::has_jpg_extension(output_path)) {
        ok = stbi_write_jpg(output_path.c_str(), width, height, 4, data, 90);
    } else {
        ok = stbi_write_png(output_path.c_str(), width, height, 4, data, stride_bytes);
    }

    stbi_image_free(data);

    if (!ok) {
        std::fprintf(stderr, "Failed to write '%s'\n", output_path.c_str());
        return 1;
    }

    std::printf("Wrote %s (%dx%d)\n", output_path.c_str(), width, height);
    return 0;
}

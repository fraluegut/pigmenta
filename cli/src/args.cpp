#include "args.h"

namespace pigmenta::cli {

std::optional<::pigmenta::ColorSpace> parse_colorspace(const std::string& s) {
    if (s == "ycbcr")
        return ::pigmenta::ColorSpace::YCBCR;
    if (s == "lab")
        return ::pigmenta::ColorSpace::LAB;
    if (s == "rgb")
        return ::pigmenta::ColorSpace::RGB;
    if (s == "yrd")
        return ::pigmenta::ColorSpace::YRD;
    if (s == "vds")
        return ::pigmenta::ColorSpace::VDS;
    return std::nullopt;
}

bool has_jpg_extension(const std::string& path) {
    return path.size() >= 4 && path.rfind(".jpg") == path.size() - 4;
}

}  // namespace pigmenta::cli

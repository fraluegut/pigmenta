#pragma once
// Pure, testable argument-parsing helpers for the Pigmenta CLI — kept free
// of std::exit() / stderr side effects so they can be unit tested directly.

#include <optional>
#include <string>
#include "pigmenta/decorrelation.h"

namespace pigmenta::cli {

// Maps a --colorspace string to a pigmenta::ColorSpace. Returns nullopt for
// anything unrecognised (the caller decides how to report that).
std::optional<::pigmenta::ColorSpace> parse_colorspace(const std::string& s);

// True if `path` ends in ".jpg" (case-sensitive, matching the original
// behaviour) — used to decide between the PNG and JPEG writers.
bool has_jpg_extension(const std::string& path);

}  // namespace pigmenta::cli

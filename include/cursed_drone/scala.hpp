// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "cursed_drone/session.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace cursed_drone {

struct ParsedScale {
    std::string name{};
    std::vector<float> cents{};
    float period_cents{1200.0F};
};

[[nodiscard]] ParsedScale equal_temperament_scale();
[[nodiscard]] bool parse_scala_file(
    const std::filesystem::path& path,
    ParsedScale& scale,
    std::string& error);
[[nodiscard]] std::vector<ParsedScale> load_scala_scales(
    const std::vector<std::filesystem::path>& directories);
void apply_scale(ScalaTuning& tuning, const ParsedScale& scale) noexcept;
void set_equal_temperament(ScalaTuning& tuning) noexcept;
[[nodiscard]] float quantize_frequency(float frequency_hz, const ScalaTuning& tuning) noexcept;

} // namespace cursed_drone

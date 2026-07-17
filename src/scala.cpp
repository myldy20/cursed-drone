// SPDX-License-Identifier: GPL-3.0-or-later
#include "cursed_drone/scala.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <sstream>

namespace cursed_drone {
namespace {

std::string trim(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1U);
}

bool parse_interval(const std::string& text, float& cents) {
    try {
        if (const auto slash = text.find('/'); slash != std::string::npos) {
            const double numerator = std::stod(text.substr(0U, slash));
            const double denominator = std::stod(text.substr(slash + 1U));
            if (numerator <= 0.0 || denominator <= 0.0) return false;
            cents = static_cast<float>(1200.0 * std::log2(numerator / denominator));
            return std::isfinite(cents);
        }
        if (text.find('.') == std::string::npos) {
            const double ratio = std::stod(text);
            if (ratio <= 0.0) return false;
            cents = static_cast<float>(1200.0 * std::log2(ratio));
            return std::isfinite(cents);
        }
        cents = std::stof(text);
        return std::isfinite(cents) && cents > 0.0F;
    } catch (...) {
        return false;
    }
}

void copy_name(std::array<char, kScaleNameLength>& target, const std::string& name) noexcept {
    target.fill('\0');
    const std::size_t count = std::min(target.size() - 1U, name.size());
    std::copy_n(name.data(), count, target.data());
}

} // namespace

ParsedScale equal_temperament_scale() {
    ParsedScale scale{};
    scale.name = "12-TET";
    scale.period_cents = 1200.0F;
    for (int degree = 1; degree <= 12; ++degree) {
        scale.cents.push_back(static_cast<float>(degree) * 100.0F);
    }
    return scale;
}

bool parse_scala_file(const std::filesystem::path& path, ParsedScale& scale, std::string& error) {
    std::ifstream input(path);
    if (!input) {
        error = "cannot open Scala file: " + path.string();
        return false;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line.front() == '!') continue;
        lines.push_back(line);
    }
    if (lines.size() < 3U) {
        error = "Scala file has too few data lines: " + path.string();
        return false;
    }

    int declared = 0;
    try {
        declared = std::stoi(lines[1]);
    } catch (...) {
        error = "invalid Scala degree count: " + path.string();
        return false;
    }
    if (declared <= 0 || declared > 512) {
        error = "unsupported Scala degree count: " + path.string();
        return false;
    }

    ParsedScale parsed{};
    parsed.name = lines[0].empty() ? path.stem().string() : lines[0];
    const int available = std::min(declared, static_cast<int>(lines.size() - 2U));
    for (int index = 0; index < available && index < static_cast<int>(kScaleDegreeCount); ++index) {
        float cents = 0.0F;
        if (!parse_interval(lines[static_cast<std::size_t>(index + 2)], cents)) {
            error = "invalid Scala interval in " + path.string();
            return false;
        }
        parsed.cents.push_back(cents);
    }
    if (parsed.cents.empty()) {
        error = "Scala file has no usable intervals: " + path.string();
        return false;
    }
    parsed.period_cents = parsed.cents.back();
    if (parsed.period_cents < 50.0F || parsed.period_cents > 4800.0F) {
        error = "Scala period is outside the experimental range: " + path.string();
        return false;
    }
    scale = std::move(parsed);
    return true;
}

std::vector<ParsedScale> load_scala_scales(const std::vector<std::filesystem::path>& directories) {
    std::vector<ParsedScale> scales;
    scales.push_back(equal_temperament_scale());
    for (const auto& directory : directories) {
        std::error_code error;
        if (!std::filesystem::is_directory(directory, error)) continue;
        for (const auto& entry : std::filesystem::directory_iterator(directory, error)) {
            if (error) break;
            if (!entry.is_regular_file() || entry.path().extension() != ".scl") continue;
            ParsedScale scale{};
            std::string parse_error;
            if (parse_scala_file(entry.path(), scale, parse_error)) {
                const bool duplicate = std::any_of(scales.begin(), scales.end(), [&](const ParsedScale& existing) {
                    return existing.name == scale.name;
                });
                if (!duplicate) scales.push_back(std::move(scale));
            }
        }
    }
    return scales;
}

void apply_scale(ScalaTuning& tuning, const ParsedScale& scale) noexcept {
    tuning.enabled = true;
    tuning.degree_count = std::clamp(static_cast<int>(scale.cents.size()), 1, static_cast<int>(kScaleDegreeCount));
    tuning.period_cents = std::clamp(scale.period_cents, 50.0F, 4800.0F);
    tuning.cents.fill(0.0F);
    for (int index = 0; index < tuning.degree_count; ++index) {
        tuning.cents[static_cast<std::size_t>(index)] = scale.cents[static_cast<std::size_t>(index)];
    }
    copy_name(tuning.name, scale.name);
}

void set_equal_temperament(ScalaTuning& tuning) noexcept {
    apply_scale(tuning, equal_temperament_scale());
}

float quantize_frequency(float frequency_hz, const ScalaTuning& tuning) noexcept {
    frequency_hz = std::clamp(frequency_hz, 4.0F, 20'000.0F);
    if (!tuning.enabled || tuning.degree_count <= 0 || tuning.period_cents <= 0.0F) {
        return frequency_hz;
    }

    const float root = 440.0F * std::pow(2.0F, (static_cast<float>(tuning.root_midi) - 69.0F) / 12.0F);
    const float total_cents = 1200.0F * std::log2(frequency_hz / root);
    const float period = tuning.period_cents;
    const float period_index = std::floor(total_cents / period);
    const float local = total_cents - period_index * period;

    float best = 0.0F;
    float best_distance = std::abs(local);
    for (int index = 0; index < tuning.degree_count; ++index) {
        const float candidate = tuning.cents[static_cast<std::size_t>(index)];
        const float distance = std::abs(candidate - local);
        if (distance < best_distance) {
            best = candidate;
            best_distance = distance;
        }
    }
    if (std::abs(period - local) < best_distance) best = period;
    const float quantized_cents = period_index * period + best;
    return root * std::pow(2.0F, quantized_cents / 1200.0F);
}

} // namespace cursed_drone

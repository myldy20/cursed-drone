// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.
#pragma once

#include <algorithm>
#include <cmath>

namespace cursed_drone::mapping {

constexpr float kMinimumFrequencyHz = 20.0F;
constexpr float kMaximumFrequencyHz = 2'000.0F;
constexpr float kMinimumModRateHz = 0.001F;
constexpr float kMaximumModRateHz = 40.0F;
constexpr float kMinimumFadeSeconds = 0.25F;
constexpr float kMaximumFadeSeconds = 30.0F;

inline float normalized(float value) noexcept {
    return std::clamp(value, 0.0F, 1.0F);
}

inline float normalize_log(float value, float minimum, float maximum) noexcept {
    value = std::clamp(value, minimum, maximum);
    return std::log(value / minimum) / std::log(maximum / minimum);
}

inline float denormalize_log(float value, float minimum, float maximum) noexcept {
    return minimum * std::pow(maximum / minimum, normalized(value));
}

inline float normalized_frequency(float value) noexcept {
    return normalize_log(value, kMinimumFrequencyHz, kMaximumFrequencyHz);
}

inline float frequency_from_normalized(float value) noexcept {
    return denormalize_log(value, kMinimumFrequencyHz, kMaximumFrequencyHz);
}

inline float normalized_mod_rate(float value) noexcept {
    return normalize_log(value, kMinimumModRateHz, kMaximumModRateHz);
}

inline float mod_rate_from_normalized(float value) noexcept {
    return denormalize_log(value, kMinimumModRateHz, kMaximumModRateHz);
}

inline float normalized_fade_seconds(float value) noexcept {
    return std::clamp(
        (value - kMinimumFadeSeconds) /
            (kMaximumFadeSeconds - kMinimumFadeSeconds),
        0.0F, 1.0F);
}

inline float fade_seconds_from_normalized(float value) noexcept {
    return kMinimumFadeSeconds +
        normalized(value) * (kMaximumFadeSeconds - kMinimumFadeSeconds);
}

inline float normalized_tuning_root(int value) noexcept {
    return static_cast<float>(std::clamp(value, 0, 127)) / 127.0F;
}

inline int tuning_root_from_normalized(float value) noexcept {
    return static_cast<int>(std::lround(normalized(value) * 127.0F));
}

inline float bipolar_from_normalized(float value) noexcept {
    return normalized(value) * 2.0F - 1.0F;
}

inline int next_rate_mod_source(int current, int modulator_index) noexcept {
    modulator_index = std::clamp(modulator_index, 0, 3);
    if (modulator_index == 0) {
        return -1;
    }
    if (current < -1 || current >= modulator_index) {
        current = -1;
    }
    return current + 1 >= modulator_index ? -1 : current + 1;
}

inline int picker_max_page(int item_count, int page_size) noexcept {
    if (item_count <= 0 || page_size <= 0) {
        return 0;
    }
    return (item_count - 1) / page_size;
}

inline int picker_next_page(int current, int item_count, int page_size) noexcept {
    return std::min(std::max(0, current) + 1,
        picker_max_page(item_count, page_size));
}

inline int picker_previous_page(int current) noexcept {
    return std::max(0, current - 1);
}

} // namespace cursed_drone::mapping

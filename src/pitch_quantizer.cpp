// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.
#include "cursed_drone/scala.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace cursed_drone {

// src/scala.cpp is compiled with a renamed symbol. It remains the canonical
// precise Scala implementation; this realtime wrapper only controls cadence.
[[nodiscard]] float quantize_frequency_precise(
    float frequency_hz,
    const ScalaTuning& tuning) noexcept;

namespace {

constexpr unsigned kQuantizerControlPeriod = 16U;
constexpr std::size_t kQuantizerCacheEntries = 8U;

struct QuantizerCacheEntry {
    const ScalaTuning* tuning{nullptr};
    unsigned remaining{0U};
    float output{55.0F};
};

thread_local std::array<QuantizerCacheEntry, kQuantizerCacheEntries>
    g_quantizer_cache{};
thread_local std::size_t g_quantizer_replacement{0U};

QuantizerCacheEntry& cache_for(const ScalaTuning& tuning) noexcept {
    for (auto& entry : g_quantizer_cache) {
        if (entry.tuning == &tuning) return entry;
    }
    for (auto& entry : g_quantizer_cache) {
        if (entry.tuning == nullptr) {
            entry.tuning = &tuning;
            return entry;
        }
    }
    auto& entry = g_quantizer_cache[g_quantizer_replacement];
    g_quantizer_replacement =
        (g_quantizer_replacement + 1U) % g_quantizer_cache.size();
    entry = {};
    entry.tuning = &tuning;
    return entry;
}

} // namespace

float quantize_frequency(
    float frequency_hz,
    const ScalaTuning& tuning) noexcept {
    if (!std::isfinite(frequency_hz)) frequency_hz = 55.0F;
    frequency_hz = std::clamp(frequency_hz, 4.0F, 20'000.0F);
    if (!tuning.enabled || tuning.degree_count <= 0 ||
        !std::isfinite(tuning.period_cents) || tuning.period_cents <= 0.0F) {
        return frequency_hz;
    }

    auto& cache = cache_for(tuning);
    if (cache.remaining == 0U) {
        cache.output = quantize_frequency_precise(frequency_hz, tuning);
        cache.remaining = kQuantizerControlPeriod - 1U;
    } else {
        --cache.remaining;
    }
    return cache.output;
}

} // namespace cursed_drone

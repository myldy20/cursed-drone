// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.
#include "plaits_actor.hpp"

#include "cursed_drone/scala.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#ifndef CURSED_DRONE_HAS_PLAITS
#define CURSED_DRONE_HAS_PLAITS 0
#endif

#if CURSED_DRONE_HAS_PLAITS
#include "plaits/dsp/voice.h"
#include "stmlib/utils/buffer_allocator.h"
#endif

namespace cursed_drone::detail {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kTwoPi = kPi * 2.0F;

int engine_index(PlaitsModel model) noexcept {
    switch (model) {
    case PlaitsModel::virtual_analog_vcf: return 0;
    case PlaitsModel::phase_distortion: return 1;
    case PlaitsModel::wave_terrain: return 5;
    case PlaitsModel::string_machine: return 6;
    case PlaitsModel::chiptune: return 7;
    case PlaitsModel::virtual_analog: return 8;
    case PlaitsModel::waveshaping: return 9;
    case PlaitsModel::fm: return 10;
    case PlaitsModel::grain: return 11;
    case PlaitsModel::additive: return 12;
    case PlaitsModel::wavetable: return 13;
    case PlaitsModel::chord: return 14;
    case PlaitsModel::swarm: return 16;
    case PlaitsModel::noise: return 17;
    case PlaitsModel::string: return 19;
    case PlaitsModel::modal: return 20;
    }
    return 8;
}

StereoFrame route(float main, float aux, PlaitsOutputMode mode) noexcept {
    switch (mode) {
    case PlaitsOutputMode::main: return {main, main};
    case PlaitsOutputMode::aux: return {aux, aux};
    case PlaitsOutputMode::mix: {
        const float mixed = (main + aux) * 0.5F;
        return {mixed, mixed};
    }
    case PlaitsOutputMode::stereo: return {main, aux};
    }
    return {main, main};
}

} // namespace

struct PlaitsActor::Impl {
    float sample_rate{48'000.0F};
    float phase_a{0.0F};
    float phase_b{0.0F};
#if CURSED_DRONE_HAS_PLAITS
    std::array<std::uint8_t, 16'384> allocator_memory{};
    stmlib::BufferAllocator allocator{};
    plaits::Voice voice{};
    std::array<plaits::Voice::Frame, plaits::kBlockSize> frames{};
    std::size_t frame_index{plaits::kBlockSize};
    bool pending_trigger{false};
#endif

    void fallback_reset() noexcept {
        phase_a = 0.0F;
        phase_b = 0.0F;
    }
};

PlaitsActor::PlaitsActor() : impl_(std::make_unique<Impl>()) { }
PlaitsActor::~PlaitsActor() = default;

void PlaitsActor::prepare(float sample_rate) {
    impl_->sample_rate = std::max(8'000.0F, sample_rate);
    impl_->fallback_reset();
#if CURSED_DRONE_HAS_PLAITS
    impl_->allocator.Init(impl_->allocator_memory.data(), impl_->allocator_memory.size());
    impl_->voice.Init(&impl_->allocator);
    impl_->frame_index = plaits::kBlockSize;
    impl_->pending_trigger = false;
#endif
}

void PlaitsActor::reset() noexcept {
    impl_->fallback_reset();
#if CURSED_DRONE_HAS_PLAITS
    impl_->allocator.Init(impl_->allocator_memory.data(), impl_->allocator_memory.size());
    impl_->voice.Init(&impl_->allocator);
    impl_->frame_index = plaits::kBlockSize;
    impl_->pending_trigger = false;
#endif
}

StereoFrame PlaitsActor::next(
    PlaitsModel model,
    PlaitsOutputMode output_mode,
    const ScalaTuning& tuning,
    float frequency_hz,
    float harmonics,
    float timbre,
    float morph,
    float decay,
    bool trigger,
    bool trigger_patched) noexcept {
    harmonics = std::clamp(harmonics, 0.0F, 1.0F);
    timbre = std::clamp(timbre, 0.0F, 1.0F);
    morph = std::clamp(morph, 0.0F, 1.0F);
    decay = std::clamp(decay, 0.0F, 1.0F);

#if CURSED_DRONE_HAS_PLAITS
    impl_->pending_trigger = impl_->pending_trigger || trigger;
    if (impl_->frame_index >= plaits::kBlockSize) {
        plaits::Patch patch{};
        const float quantized_frequency = quantize_frequency(frequency_hz, tuning);
        const float corrected_frequency = quantized_frequency * (48'000.0F / impl_->sample_rate);
        patch.note = 69.0F + 12.0F * std::log2(std::max(1.0F, corrected_frequency) / 440.0F);
        patch.harmonics = harmonics;
        patch.timbre = timbre;
        patch.morph = morph;
        patch.frequency_modulation_amount = 0.0F;
        patch.timbre_modulation_amount = 0.0F;
        patch.morph_modulation_amount = 0.0F;
        patch.engine = engine_index(model);
        patch.decay = decay;
        patch.lpg_colour = harmonics;

        plaits::Modulations modulations{};
        modulations.engine = 0.0F;
        modulations.note = 0.0F;
        modulations.frequency = 0.0F;
        modulations.harmonics = 0.0F;
        modulations.timbre = 0.0F;
        modulations.morph = 0.0F;
        modulations.trigger = impl_->pending_trigger ? 1.0F : 0.0F;
        modulations.level = 1.0F;
        modulations.frequency_patched = false;
        modulations.timbre_patched = false;
        modulations.morph_patched = false;
        modulations.trigger_patched = trigger_patched;
        modulations.level_patched = false;

        impl_->voice.Render(patch, modulations, impl_->frames.data(), plaits::kBlockSize);
        impl_->frame_index = 0U;
        impl_->pending_trigger = false;
    }
    const auto frame = impl_->frames[impl_->frame_index++];
    const float main = -static_cast<float>(frame.out) / 32768.0F;
    const float aux = -static_cast<float>(frame.aux) / 32768.0F;
    return route(main, aux, output_mode);
#else
    frequency_hz = quantize_frequency(frequency_hz, tuning);
    const float ratio = 1.0F + static_cast<float>(engine_index(model) % 7) * 0.125F;
    impl_->phase_a += frequency_hz / impl_->sample_rate;
    impl_->phase_b += frequency_hz * ratio / impl_->sample_rate;
    impl_->phase_a -= std::floor(impl_->phase_a);
    impl_->phase_b -= std::floor(impl_->phase_b);
    const float sine = std::sin(impl_->phase_a * kTwoPi);
    const float saw = impl_->phase_b * 2.0F - 1.0F;
    const float shaped = std::tanh((sine * (1.0F - harmonics) + saw * harmonics) * (1.0F + timbre * 5.0F));
    const float aux = std::sin((impl_->phase_a + std::sin(impl_->phase_b * kTwoPi) * morph * 0.18F) * kTwoPi);
    return route(shaped * (0.45F + decay * 0.55F), aux * 0.75F, output_mode);
#endif
}

} // namespace cursed_drone::detail

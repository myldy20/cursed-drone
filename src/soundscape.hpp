// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.
#pragma once

#include "cursed_drone/session.hpp"

#include "Filters/svf.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace cursed_drone::detail {

// One instance renders one scene actor. The algorithms deliberately model
// gestures and materials rather than notes: event timing, excitation and the
// resonating body all evolve independently.
class SoundscapeVoice {
public:
    void prepare(float sample_rate, std::size_t voice_index) noexcept;
    void reset() noexcept;

    float next(
        EngineKind kind,
        float frequency,
        float timbre,
        float color,
        float motion,
        float texture,
        float tempo_bpm,
        float activity,
        float tension,
        float evolution,
        bool external_trigger) noexcept;

    [[nodiscard]] bool consume_event_fired() noexcept;

private:
    float derelict_bed(float frequency, float timbre, float color, float motion,
        float texture, float tension, float evolution) noexcept;
    float footsteps(float frequency, float timbre, float color, float motion,
        float texture, float tempo_bpm, float activity, float tension, float evolution) noexcept;
    float door(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float pipe(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float motor(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float machinery(float frequency, float timbre, float color, float motion,
        float texture, float tempo_bpm, float activity, float tension, float evolution) noexcept;
    float crowd(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float metal(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float wind(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float birds(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float insects(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float signal(float frequency, float timbre, float color, float motion,
        float texture, float tempo_bpm, float activity, float tension, float evolution) noexcept;
    float cave_air(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float water_flow(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float stone(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float metro_traction(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float rail_joint(float frequency, float timbre, float color, float motion,
        float texture, float tempo_bpm, float activity, float tension, float evolution) noexcept;
    float brake(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float carriage(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float music_box(float frequency, float timbre, float color, float motion,
        float texture, float tempo_bpm, float activity, float tension, float evolution) noexcept;
    float toy_voice(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float toy_gears(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float lullaby(float frequency, float timbre, float color, float motion,
        float texture, float tempo_bpm, float activity, float tension, float evolution) noexcept;
    float sub_drone(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float tape_drone(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float bowed_metal(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;
    float earth_rumble(float frequency, float timbre, float color, float motion,
        float texture, float activity, float tension, float evolution) noexcept;

    float noise() noexcept;
    float sine(std::size_t index, float frequency) noexcept;
    float modal_sum(float base_frequency, float damping, float brightness) noexcept;
    void excite_modes(float amount, float irregularity) noexcept;
    bool tick_event(float rate_hz, float irregularity) noexcept;
    bool take_external_trigger() noexcept;
    void mark_event_fired() noexcept;
    bool control_tick() noexcept;
    void configure_filter(std::size_t index, float frequency, float resonance, float drive = 0.0F) noexcept;

    float sample_rate_{48'000.0F};
    float inverse_sample_rate_{1.0F / 48'000.0F};
    std::size_t voice_index_{0U};
    EngineKind last_kind_{EngineKind::diagnostic};
    std::uint32_t random_state_{0x3141'5926U};
    unsigned control_counter_{0U};
    float time_{0.0F};
    float event_countdown_{0.0F};
    float event_age_{99.0F};
    float event_duration_{1.0F};
    float secondary_countdown_{99.0F};
    float secondary_envelope_{0.0F};
    float primary_envelope_{0.0F};
    float slip_phase_{0.0F};
    float step_phase_{0.0F};
    float approach_phase_{0.0F};
    float slow_value_{0.0F};
    float slow_target_{0.0F};
    float slow_countdown_{0.0F};
    float event_pan_{0.0F};
    int sequence_index_{0};
    int sequence_length_{0};
    int chirps_remaining_{0};
    bool external_trigger_pending_{false};
    bool event_fired_{false};
    std::array<float, 8> phases_{};
    std::array<float, 6> mode_phases_{};
    std::array<float, 6> mode_amplitudes_{};
    std::array<daisysp::Svf, 6> filters_{};
};

} // namespace cursed_drone::detail

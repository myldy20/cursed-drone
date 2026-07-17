// SPDX-License-Identifier: GPL-3.0-or-later
#include "soundscape.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace cursed_drone::detail {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kTwoPi = 2.0F * kPi;

float clamp01(float value) noexcept {
    return std::clamp(value, 0.0F, 1.0F);
}

float smoothstep(float value) noexcept {
    value = clamp01(value);
    return value * value * (3.0F - 2.0F * value);
}

float triangle(float phase) noexcept {
    return 1.0F - std::abs(phase * 2.0F - 1.0F);
}

float decay_coefficient(float sample_rate, float seconds) noexcept {
    return std::exp(-1.0F / std::max(1.0F, sample_rate * seconds));
}

bool is_soundscape_engine(EngineKind kind) noexcept {
    return static_cast<int>(kind) >= static_cast<int>(EngineKind::derelict_bed);
}

} // namespace

void SoundscapeVoice::prepare(float sample_rate, std::size_t voice_index) noexcept {
    sample_rate_ = std::max(8'000.0F, sample_rate);
    inverse_sample_rate_ = 1.0F / sample_rate_;
    voice_index_ = voice_index;
    random_state_ = 0x3141'5926U ^ (0x9E37'79B9U * static_cast<std::uint32_t>(voice_index + 1U));
    reset();
}

void SoundscapeVoice::reset() noexcept {
    control_counter_ = 0U;
    time_ = 0.0F;
    event_countdown_ = 0.05F + static_cast<float>(voice_index_) * 0.07F;
    event_age_ = 99.0F;
    event_duration_ = 1.0F;
    secondary_countdown_ = 99.0F;
    secondary_envelope_ = 0.0F;
    primary_envelope_ = 0.0F;
    slip_phase_ = 0.0F;
    step_phase_ = 0.86F;
    approach_phase_ = 0.17F * static_cast<float>(voice_index_);
    slow_value_ = 0.0F;
    slow_target_ = 0.0F;
    slow_countdown_ = 0.0F;
    event_pan_ = 0.0F;
    sequence_index_ = 0;
    sequence_length_ = 0;
    chirps_remaining_ = 0;
    phases_.fill(0.0F);
    mode_phases_.fill(0.0F);
    mode_amplitudes_.fill(0.0F);
    for (auto& filter : filters_) {
        filter.Init(sample_rate_);
        filter.SetRes(0.2F);
        filter.SetDrive(0.0F);
    }
}

float SoundscapeVoice::noise() noexcept {
    random_state_ ^= random_state_ << 13U;
    random_state_ ^= random_state_ >> 17U;
    random_state_ ^= random_state_ << 5U;
    return static_cast<float>(random_state_) * (2.0F / 4'294'967'295.0F) - 1.0F;
}

float SoundscapeVoice::sine(std::size_t index, float frequency) noexcept {
    auto& phase = phases_[index % phases_.size()];
    phase += std::clamp(frequency, 0.01F, sample_rate_ * 0.45F) * inverse_sample_rate_;
    phase -= std::floor(phase);
    return std::sin(phase * kTwoPi);
}

bool SoundscapeVoice::control_tick() noexcept {
    ++control_counter_;
    if (control_counter_ < 64U) {
        return false;
    }
    control_counter_ = 0U;
    return true;
}

void SoundscapeVoice::configure_filter(
    std::size_t index, float frequency, float resonance, float drive) noexcept {
    auto& filter = filters_[index % filters_.size()];
    filter.SetFreq(std::clamp(frequency, 1.0F, sample_rate_ * 0.30F));
    filter.SetRes(clamp01(resonance));
    filter.SetDrive(clamp01(drive));
}

bool SoundscapeVoice::tick_event(float rate_hz, float irregularity) noexcept {
    event_countdown_ -= inverse_sample_rate_;
    if (event_countdown_ > 0.0F) {
        return false;
    }
    const float random_unit = noise() * 0.5F + 0.5F;
    const float base = 1.0F / std::max(0.005F, rate_hz);
    event_countdown_ = base * std::clamp(
        1.0F + (random_unit * 2.0F - 1.0F) * irregularity, 0.18F, 2.7F);
    return true;
}

void SoundscapeVoice::excite_modes(float amount, float irregularity) noexcept {
    for (std::size_t index = 0; index < mode_amplitudes_.size(); ++index) {
        const float weight = 1.0F / (1.0F + static_cast<float>(index) * 0.72F);
        mode_amplitudes_[index] += amount * weight *
            std::clamp(1.0F + noise() * irregularity, 0.08F, 1.8F);
    }
}

float SoundscapeVoice::modal_sum(float base_frequency, float damping, float brightness) noexcept {
    constexpr std::array<float, 6> ratios{1.0F, 1.47F, 2.18F, 3.11F, 4.37F, 6.02F};
    float result = 0.0F;
    for (std::size_t index = 0; index < ratios.size(); ++index) {
        const float bright = std::pow(0.35F + brightness * 0.65F, static_cast<float>(index));
        mode_phases_[index] += base_frequency * ratios[index] * inverse_sample_rate_;
        mode_phases_[index] -= std::floor(mode_phases_[index]);
        result += std::sin(mode_phases_[index] * kTwoPi) * mode_amplitudes_[index] * bright;
        const float seconds = damping * (0.62F + 0.16F * static_cast<float>(index));
        mode_amplitudes_[index] *= decay_coefficient(sample_rate_, std::max(0.008F, seconds));
    }
    return result * 0.26F;
}

float SoundscapeVoice::next(
    EngineKind kind,
    float frequency,
    float timbre,
    float color,
    float motion,
    float texture,
    float tempo_bpm,
    float activity,
    float tension,
    float evolution) noexcept {
    if (!is_soundscape_engine(kind)) {
        return 0.0F;
    }
    if (kind != last_kind_) {
        reset();
        last_kind_ = kind;
        if (kind == EngineKind::signal) {
            secondary_countdown_ = 0.0F;
        }
    }
    time_ += inverse_sample_rate_;
    timbre = clamp01(timbre);
    color = clamp01(color);
    motion = clamp01(motion);
    texture = clamp01(texture);
    activity = clamp01(activity);
    tension = clamp01(tension);
    evolution = clamp01(evolution);

    slow_countdown_ -= inverse_sample_rate_;
    if (slow_countdown_ <= 0.0F) {
        slow_target_ = noise();
        slow_countdown_ = 0.35F + (noise() * 0.5F + 0.5F) * (2.8F - evolution * 2.1F);
    }
    const float slow_slew = 1.0F - std::exp(-inverse_sample_rate_ / (0.28F + (1.0F - motion) * 1.8F));
    slow_value_ += (slow_target_ - slow_value_) * slow_slew;

    switch (kind) {
    case EngineKind::derelict_bed:
        return derelict_bed(frequency, timbre, color, motion, texture, tension, evolution);
    case EngineKind::footsteps:
        return footsteps(frequency, timbre, color, motion, texture, tempo_bpm, activity, tension, evolution);
    case EngineKind::door:
        return door(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::pipe:
        return pipe(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::motor:
        return motor(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::machinery:
        return machinery(frequency, timbre, color, motion, texture, tempo_bpm, activity, tension, evolution);
    case EngineKind::crowd:
        return crowd(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::metal:
        return metal(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::wind:
        return wind(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::birds:
        return birds(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::insects:
        return insects(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::signal:
        return signal(frequency, timbre, color, motion, texture, tempo_bpm, activity, tension, evolution);
    case EngineKind::cave_air:
        return cave_air(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::water_flow:
        return water_flow(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::stone:
        return stone(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::metro_traction:
        return metro_traction(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::rail_joint:
        return rail_joint(frequency, timbre, color, motion, texture, tempo_bpm, activity, tension, evolution);
    case EngineKind::brake:
        return brake(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::carriage:
        return carriage(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::music_box:
        return music_box(frequency, timbre, color, motion, texture, tempo_bpm, activity, tension, evolution);
    case EngineKind::toy_voice:
        return toy_voice(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::toy_gears:
        return toy_gears(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::lullaby:
        return lullaby(frequency, timbre, color, motion, texture, tempo_bpm, activity, tension, evolution);
    case EngineKind::sub_drone:
        return sub_drone(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::tape_drone:
        return tape_drone(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::bowed_metal:
        return bowed_metal(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::earth_rumble:
        return earth_rumble(frequency, timbre, color, motion, texture, activity, tension, evolution);
    case EngineKind::diagnostic:
    case EngineKind::macro:
    case EngineKind::body:
    case EngineKind::grain:
    case EngineKind::particle:
    case EngineKind::water_drip:
        break;
    }
    return 0.0F;
}

float SoundscapeVoice::derelict_bed(float frequency, float timbre, float color, float motion,
    float texture, float tension, float evolution) noexcept {
    const float long_cycle = std::fmod(time_ * (0.006F + evolution * 0.018F), 1.0F);
    const float climb = smoothstep(long_cycle) - 0.38F;
    const float base = std::clamp(frequency, 18.0F, 180.0F) *
        std::pow(2.0F, climb * tension * 0.48F + slow_value_ * motion * 0.035F);
    const float breath = 0.52F + 0.30F * sine(6U, 0.017F + evolution * 0.041F) + 0.18F * slow_value_;
    const float fundamental = sine(0U, base);
    const float beating = sine(1U, base * (1.002F + timbre * 0.014F));
    const float overtone = sine(2U, base * (2.01F + color * 1.93F));
    const float raw = noise();
    if (control_tick()) {
        configure_filter(0U, 120.0F + color * 920.0F + slow_value_ * 55.0F, 0.58F + texture * 0.30F);
        configure_filter(1U, 1'100.0F + color * 2'600.0F, 0.10F + texture * 0.25F);
    }
    filters_[0].Process(raw);
    filters_[1].Process(raw);
    const float air = filters_[0].Band() * (0.10F + texture * 0.34F) + filters_[1].Band() * texture * 0.06F;
    const float hum = fundamental * (0.72F - timbre * 0.24F) + beating * (0.16F + timbre * 0.20F) +
        overtone * (0.02F + color * 0.08F);
    return hum * std::clamp(breath, 0.08F, 1.0F) * 0.62F + air;
}

float SoundscapeVoice::footsteps(float frequency, float timbre, float color, float motion,
    float texture, float tempo_bpm, float activity, float tension, float evolution) noexcept {
    const float tempo = std::clamp(tempo_bpm, 10.0F, 300.0F) / 60.0F;
    const float rate = 0.24F + motion * 0.85F + activity * activity * 1.55F + tempo * activity * 0.18F;
    step_phase_ += rate * inverse_sample_rate_;
    if (step_phase_ >= 1.0F) {
        step_phase_ -= 1.0F;
        primary_envelope_ = 1.0F;
        secondary_countdown_ = 0.065F + (noise() * 0.5F + 0.5F) * (0.10F + timbre * 0.07F);
        event_pan_ = -event_pan_;
        if (std::abs(event_pan_) < 0.2F) {
            event_pan_ = 0.65F;
        }
    }
    secondary_countdown_ -= inverse_sample_rate_;
    if (secondary_countdown_ <= 0.0F && secondary_countdown_ > -1.0F) {
        secondary_envelope_ = 1.0F;
        secondary_countdown_ = -2.0F;
    }
    approach_phase_ += (0.012F + evolution * 0.045F) * inverse_sample_rate_;
    approach_phase_ -= std::floor(approach_phase_);
    const float approach = 0.10F + 0.90F * std::pow(triangle(approach_phase_), 1.4F);
    const float heel_decay = decay_coefficient(sample_rate_, 0.045F + (1.0F - color) * 0.08F);
    const float toe_decay = decay_coefficient(sample_rate_, 0.075F + timbre * 0.11F);
    primary_envelope_ *= heel_decay;
    secondary_envelope_ *= toe_decay;
    const float thump_frequency = std::clamp(frequency, 28.0F, 130.0F) *
        (1.0F + primary_envelope_ * (0.8F + tension * 1.7F));
    const float thump = sine(0U, thump_frequency) * primary_envelope_ * primary_envelope_;
    const float toe = sine(1U, thump_frequency * (1.42F + timbre * 0.65F)) * secondary_envelope_;
    const float raw = noise();
    if (control_tick()) {
        configure_filter(0U, 170.0F + color * 1'450.0F, 0.12F + timbre * 0.38F);
        configure_filter(1U, 1'600.0F + color * 4'800.0F, 0.08F + texture * 0.20F);
    }
    filters_[0].Process(raw);
    filters_[1].Process(raw);
    const float weight = primary_envelope_ * 0.75F + secondary_envelope_ * 0.52F;
    const float surface = filters_[0].Band() * (0.18F + texture * 0.62F) +
        filters_[1].High() * texture * 0.08F;
    return (thump * 0.62F + toe * 0.24F + surface * weight) * approach;
}

float SoundscapeVoice::door(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    const float rate = 0.010F + activity * activity * 0.085F + motion * 0.035F;
    if (tick_event(rate, 0.82F)) {
        event_age_ = 0.0F;
        event_duration_ = 0.55F + (noise() * 0.5F + 0.5F) * (1.4F + evolution * 2.2F);
        slip_phase_ = 1.0F;
    }
    event_age_ += inverse_sample_rate_;
    const float progress = event_age_ / std::max(0.05F, event_duration_);
    if (progress >= 1.0F) {
        return modal_sum(std::clamp(frequency, 45.0F, 220.0F), 0.028F + timbre * 0.055F, color) * 0.15F;
    }
    const float gesture = std::sin(progress * kPi);
    const float slip_rate = 5.0F + motion * 19.0F + texture * 23.0F + slow_value_ * 2.0F;
    slip_phase_ += slip_rate * inverse_sample_rate_;
    if (slip_phase_ >= 1.0F) {
        slip_phase_ -= 1.0F;
        primary_envelope_ = 1.0F;
        excite_modes(gesture * (0.15F + texture * 0.32F), 0.65F);
    }
    primary_envelope_ *= decay_coefficient(sample_rate_, 0.012F + (1.0F - texture) * 0.028F);
    const float glide = 0.55F + progress * (0.7F + tension * 2.8F) + slow_value_ * 0.08F;
    const float squeal_frequency = std::clamp(frequency, 45.0F, 220.0F) * (2.4F + color * 5.4F) * glide;
    const float squeal = sine(0U, squeal_frequency) * primary_envelope_ * gesture;
    const float raw = noise();
    if (control_tick()) {
        configure_filter(0U, squeal_frequency * (0.72F + timbre * 0.42F), 0.72F + color * 0.24F);
    }
    filters_[0].Process(raw * primary_envelope_);
    const float wood = modal_sum(std::clamp(frequency, 45.0F, 220.0F), 0.025F + timbre * 0.065F, color);
    return squeal * (0.18F + tension * 0.20F) + filters_[0].Band() * texture * 0.28F + wood * 0.74F;
}

float SoundscapeVoice::pipe(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    const float raw = noise();
    const float gust = std::clamp(0.42F + slow_value_ * 0.48F + sine(6U, 0.025F + motion * 0.08F) * 0.20F, 0.0F, 1.0F);
    const float pipe_base = 160.0F + frequency * (1.3F + color * 3.5F);
    if (control_tick()) {
        configure_filter(0U, pipe_base * (0.82F + slow_value_ * 0.10F), 0.80F + timbre * 0.18F);
        configure_filter(1U, pipe_base * (1.83F + color * 0.55F), 0.70F + timbre * 0.25F);
    }
    filters_[0].Process(raw * gust);
    filters_[1].Process(raw * gust);
    if (tick_event(0.025F + activity * activity * 0.34F, 0.86F)) {
        excite_modes(0.32F + activity * 0.34F, 0.55F);
    }
    const float howl = sine(0U, pipe_base * (0.48F + slow_value_ * motion * 0.05F)) *
        std::pow(gust, 2.4F) * (0.04F + tension * 0.22F);
    const float knock = modal_sum(std::clamp(frequency, 42.0F, 160.0F),
        0.035F + (1.0F - texture) * 0.09F, 0.35F + color * 0.55F);
    const float drift = 0.65F + 0.35F * sine(7U, 0.007F + evolution * 0.026F);
    return (filters_[0].Band() * 0.42F + filters_[1].Band() * texture * 0.22F + howl + knock * 0.55F) * drift;
}

float SoundscapeVoice::motor(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    const float load_cycle = 0.5F + 0.5F * sine(6U, 0.018F + evolution * 0.055F);
    // A loaded motor changes spectrum and amplitude much more than pitch. The
    // previous broad speed sweep produced an accidental human "wah" gesture.
    const float nominal_speed = std::clamp(frequency, 16.0F, 130.0F) * (0.78F + activity * 0.48F);
    const float speed = nominal_speed * (1.0F + (load_cycle - 0.5F) * motion * 0.045F +
        slow_value_ * tension * 0.012F);
    const float load_harmonics = 0.72F + load_cycle * 0.38F;
    const float rotor = sine(0U, speed) * 0.55F +
        sine(1U, speed * 2.0F) * (0.13F + timbre * 0.20F) * load_harmonics +
        sine(2U, speed * (3.0F + color * 2.0F)) * texture * 0.12F * load_harmonics;
    const float commutator = sine(3U, speed * (6.0F + timbre * 10.0F));
    const float raw = noise();
    if (control_tick()) {
        configure_filter(0U, 1'100.0F + color * 4'800.0F + speed * 8.0F, 0.16F + texture * 0.42F);
    }
    filters_[0].Process(raw);
    const float brush = filters_[0].Band() * (0.08F + texture * 0.42F) * (0.6F + 0.4F * commutator);
    const float torque = 0.72F + load_cycle * 0.20F + slow_value_ * tension * 0.025F;
    return (rotor * torque + brush) * (0.62F + activity * 0.22F);
}

float SoundscapeVoice::machinery(float frequency, float timbre, float color, float motion,
    float texture, float tempo_bpm, float activity, float tension, float evolution) noexcept {
    const float tempo = std::clamp(tempo_bpm, 10.0F, 300.0F) / 60.0F;
    const float rate = tempo * (0.18F + activity * 1.55F + motion * 0.65F);
    if (tick_event(rate, 0.08F + evolution * 0.50F)) {
        primary_envelope_ = 1.0F;
        secondary_countdown_ = 0.045F + (noise() * 0.5F + 0.5F) * 0.16F;
        excite_modes(0.16F + texture * 0.30F, 0.48F);
    }
    secondary_countdown_ -= inverse_sample_rate_;
    if (secondary_countdown_ <= 0.0F && secondary_countdown_ > -1.0F) {
        secondary_envelope_ = 1.0F;
        secondary_countdown_ = -2.0F;
    }
    primary_envelope_ *= decay_coefficient(sample_rate_, 0.035F + timbre * 0.075F);
    secondary_envelope_ *= decay_coefficient(sample_rate_, 0.10F + texture * 0.28F);
    const float impact_frequency = std::clamp(frequency, 25.0F, 180.0F) *
        (1.0F + primary_envelope_ * (1.5F + tension * 3.0F));
    const float impact = sine(0U, impact_frequency) * primary_envelope_;
    const float raw = noise();
    if (control_tick()) {
        configure_filter(0U, 520.0F + color * 3'900.0F, 0.24F + timbre * 0.48F);
    }
    filters_[0].Process(raw);
    const float hiss = filters_[0].Band() * secondary_envelope_ * (0.12F + texture * 0.52F);
    const float body = modal_sum(std::clamp(frequency, 25.0F, 180.0F), 0.018F + timbre * 0.055F, color);
    return impact * 0.72F + hiss + body * 0.48F;
}

float SoundscapeVoice::crowd(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    const float raw = noise();
    const float phrase_a = 0.5F + 0.5F * sine(5U, 0.08F + motion * 0.17F);
    const float phrase_b = 0.5F + 0.5F * sine(6U, 0.053F + evolution * 0.13F);
    const float phrase_c = 0.5F + 0.5F * sine(7U, 0.031F + activity * 0.11F);
    const float base = std::clamp(frequency, 70.0F, 260.0F) * (0.9F + slow_value_ * tension * 0.08F);
    if (control_tick()) {
        configure_filter(0U, base * (1.3F + timbre * 1.6F), 0.42F + color * 0.30F);
        configure_filter(1U, base * (3.2F + color * 3.4F), 0.34F + timbre * 0.34F);
        configure_filter(2U, base * (7.0F + texture * 9.0F), 0.18F + color * 0.24F);
    }
    filters_[0].Process(raw * phrase_a);
    filters_[1].Process(raw * phrase_b);
    filters_[2].Process(raw * phrase_c);
    const float murmur = filters_[0].Band() * 0.72F + filters_[1].Band() * (0.34F + timbre * 0.34F) +
        filters_[2].Band() * texture * 0.24F;
    const float mass = 0.24F + activity * 0.54F + phrase_a * phrase_b * 0.22F;
    return murmur * mass;
}

float SoundscapeVoice::metal(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    const float rate = 0.012F + activity * activity * 0.12F + motion * 0.04F;
    if (tick_event(rate, 0.80F)) {
        event_age_ = 0.0F;
        event_duration_ = 0.28F + (noise() * 0.5F + 0.5F) * (0.8F + evolution * 2.6F);
        excite_modes(0.16F + texture * 0.28F, 0.82F);
    }
    event_age_ += inverse_sample_rate_;
    const float progress = event_age_ / std::max(0.05F, event_duration_);
    const float gesture = progress < 1.0F ? std::sin(progress * kPi) : 0.0F;
    const float raw = noise();
    const float sweep = 0.35F + progress * (1.3F + tension * 3.4F) + slow_value_ * 0.12F;
    const float base = std::clamp(frequency, 45.0F, 220.0F);
    if (control_tick()) {
        configure_filter(0U, base * (3.0F + color * 5.0F) * sweep, 0.82F + timbre * 0.16F, texture * 0.20F);
        configure_filter(1U, base * (7.0F + texture * 12.0F) / std::max(0.5F, sweep), 0.72F + color * 0.24F);
        configure_filter(2U, base * (12.0F + color * 18.0F), 0.45F + timbre * 0.42F);
    }
    filters_[0].Process(raw * gesture);
    filters_[1].Process(raw * gesture);
    filters_[2].Process(raw * gesture);
    const float scrape = filters_[0].Band() * 0.62F + filters_[1].Band() * 0.38F +
        filters_[2].Band() * texture * 0.18F;
    const float clank = modal_sum(base, 0.012F + (1.0F - timbre) * 0.045F, color);
    return scrape * (0.35F + texture * 0.65F) + clank * 0.34F;
}

float SoundscapeVoice::wind(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    const float raw = noise();
    if (control_tick()) {
        configure_filter(0U, 0.35F + evolution * 2.2F, 0.05F);
        configure_filter(1U, 180.0F + color * 1'450.0F + slow_value_ * 100.0F, 0.42F + timbre * 0.42F);
        configure_filter(2U, 1'100.0F + texture * 5'600.0F, 0.14F + color * 0.36F);
    }
    filters_[0].Process(raw);
    const float gust = std::clamp(0.28F + filters_[0].Low() * 8.0F + activity * 0.28F +
        sine(7U, 0.021F + motion * 0.11F) * 0.18F, 0.0F, 1.0F);
    filters_[1].Process(raw * gust);
    filters_[2].Process(raw * gust * gust);
    const float howl_frequency = 170.0F + frequency * (1.2F + color * 4.2F) + slow_value_ * 80.0F;
    const float howl = sine(0U, howl_frequency) * std::pow(gust, 3.2F) * (0.006F + tension * 0.055F);
    return filters_[1].Band() * (0.48F + timbre * 0.24F) + filters_[2].Band() * texture * 0.25F + howl;
}

float SoundscapeVoice::birds(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    if (chirps_remaining_ <= 0 && tick_event(
            0.018F + activity * activity * 0.20F + evolution * 0.03F, 0.88F)) {
        chirps_remaining_ = 2 + static_cast<int>((noise() * 0.5F + 0.5F) * (2.0F + activity * 5.0F));
        secondary_countdown_ = 0.0F;
    }
    secondary_countdown_ -= inverse_sample_rate_;
    if (chirps_remaining_ > 0 && secondary_countdown_ <= 0.0F && event_age_ >= event_duration_) {
        --chirps_remaining_;
        event_age_ = 0.0F;
        event_duration_ = 0.045F + (noise() * 0.5F + 0.5F) * (0.09F + (1.0F - motion) * 0.16F);
        secondary_countdown_ = event_duration_ + 0.035F + (noise() * 0.5F + 0.5F) * 0.16F;
        event_pan_ = noise();
    }
    event_age_ += inverse_sample_rate_;
    const float progress = event_age_ / std::max(0.02F, event_duration_);
    if (progress >= 1.0F) {
        return 0.0F;
    }
    const float envelope = std::pow(std::sin(progress * kPi), 1.3F + texture * 2.0F);
    const float base = std::clamp(frequency, 220.0F, 1'600.0F) * (0.72F + color * 1.15F);
    const float contour = 0.62F + progress * (1.1F + tension * 1.7F) +
        std::sin(progress * kTwoPi * (1.0F + timbre * 3.0F)) * (0.08F + motion * 0.17F);
    const float carrier = sine(0U, base * contour);
    const float syrinx = sine(1U, base * contour * (1.98F + timbre * 0.06F));
    const float grain = noise() * texture * 0.08F;
    return (carrier * 0.54F + syrinx * (0.08F + color * 0.12F) + grain) * envelope * 0.40F;
}

float SoundscapeVoice::insects(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    const float base = std::clamp(frequency, 260.0F, 2'800.0F) * (0.72F + tension * 0.48F);
    const float burst_rate = 7.0F + motion * 28.0F + activity * 22.0F;
    const float gate_a = std::pow(0.5F + 0.5F * sine(4U, burst_rate), 4.0F + texture * 12.0F);
    const float gate_b = std::pow(0.5F + 0.5F * sine(5U, burst_rate * (0.47F + timbre * 0.18F)), 6.0F);
    const float colony = 0.28F + 0.36F * sine(7U, 0.025F + evolution * 0.12F) + 0.24F * slow_value_;
    const float insect_a = sine(0U, base * (1.0F + slow_value_ * 0.012F));
    const float insect_b = sine(1U, base * (1.37F + color * 0.82F));
    const float raw = noise();
    if (control_tick()) {
        configure_filter(0U, base * (0.7F + color * 0.55F), 0.58F + timbre * 0.38F);
    }
    filters_[0].Process(raw);
    const float scratch = filters_[0].Band() * texture * gate_b * 0.12F;
    return (insect_a * gate_a * 0.18F + insect_b * gate_b * 0.09F + scratch) *
        std::clamp(colony + activity * 0.28F, 0.0F, 1.0F);
}

float SoundscapeVoice::signal(float frequency, float timbre, float color, float motion,
    float texture, float tempo_bpm, float activity, float tension, float evolution) noexcept {
    secondary_countdown_ -= inverse_sample_rate_;
    if (sequence_index_ >= sequence_length_ && secondary_countdown_ <= 0.0F) {
        sequence_length_ = 3 + static_cast<int>((noise() * 0.5F + 0.5F) * (3.0F + activity * 6.0F));
        sequence_index_ = 0;
        secondary_countdown_ = 0.0F;
    }
    if (sequence_index_ < sequence_length_ && secondary_countdown_ <= 0.0F) {
        primary_envelope_ = 1.0F;
        ++sequence_index_;
        const float tempo = std::clamp(tempo_bpm, 10.0F, 300.0F) / 60.0F;
        secondary_countdown_ = (0.16F + (1.0F - activity) * 0.46F) / std::max(0.35F, tempo);
        if (sequence_index_ >= sequence_length_) {
            secondary_countdown_ += 1.2F + (noise() * 0.5F + 0.5F) * (3.0F + (1.0F - evolution) * 8.0F);
        }
    }
    primary_envelope_ *= decay_coefficient(sample_rate_, 0.06F + timbre * 0.18F);
    const float rise = sequence_length_ > 1
        ? static_cast<float>(std::max(0, sequence_index_ - 1)) / static_cast<float>(sequence_length_ - 1)
        : 0.0F;
    const float pitch = std::clamp(frequency, 35.0F, 420.0F) *
        std::pow(2.0F, rise * (0.42F + tension * 1.05F));
    const float tone = sine(0U, pitch * (1.0F + primary_envelope_ * motion * 0.30F));
    const float overtone = sine(1U, pitch * (2.0F + color * 2.0F));
    const float grit = noise() * texture * 0.12F;
    return (tone * 0.50F + overtone * (0.035F + color * 0.09F) + grit) * primary_envelope_;
}

float SoundscapeVoice::cave_air(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    const float raw = noise();
    const float breath = std::clamp(0.48F + slow_value_ * 0.32F +
        sine(7U, 0.013F + evolution * 0.052F) * (0.12F + motion * 0.16F), 0.04F, 1.0F);
    if (control_tick()) {
        configure_filter(0U, 34.0F + color * 170.0F, 0.45F + timbre * 0.42F);
        configure_filter(1U, 240.0F + color * 1'600.0F + slow_value_ * 90.0F,
            0.28F + timbre * 0.55F);
        configure_filter(2U, 1'400.0F + texture * 4'800.0F, 0.10F + color * 0.24F);
    }
    filters_[0].Process(raw * breath);
    filters_[1].Process(raw * breath);
    filters_[2].Process(raw * breath);
    const float sub = sine(0U, std::clamp(frequency, 18.0F, 70.0F) *
        (1.0F + slow_value_ * tension * 0.012F)) * (0.025F + tension * 0.07F);
    const float hollow = filters_[1].Band() * (0.30F + timbre * 0.26F);
    const float mist = filters_[2].Band() * texture * (0.04F + activity * 0.12F);
    return filters_[0].Low() * 0.75F + hollow + mist + sub;
}

float SoundscapeVoice::water_flow(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    const float raw = noise();
    const float surge = std::clamp(0.46F + slow_value_ * 0.34F +
        sine(6U, 0.08F + motion * 0.34F) * 0.20F, 0.06F, 1.0F);
    if (control_tick()) {
        configure_filter(0U, 90.0F + color * 720.0F, 0.20F + timbre * 0.32F);
        configure_filter(1U, 760.0F + color * 3'800.0F, 0.16F + texture * 0.28F);
        configure_filter(2U, 2'400.0F + texture * 5'200.0F, 0.08F + color * 0.18F);
    }
    filters_[0].Process(raw * surge);
    filters_[1].Process(raw * surge * surge);
    filters_[2].Process(raw);
    if (tick_event(0.18F + activity * 1.4F + evolution * 0.5F, 0.82F)) {
        primary_envelope_ = 1.0F;
        event_pan_ = noise();
    }
    primary_envelope_ *= decay_coefficient(sample_rate_, 0.025F + timbre * 0.10F);
    const float bubble_pitch = std::clamp(frequency, 55.0F, 420.0F) *
        (2.0F + (event_pan_ * 0.5F + 0.5F) * (3.0F + color * 8.0F));
    const float bubble = sine(0U, bubble_pitch * (1.0F + primary_envelope_ * tension * 0.52F)) *
        primary_envelope_ * 0.08F;
    return filters_[0].Band() * 0.56F + filters_[1].Band() * (0.24F + texture * 0.28F) +
        filters_[2].High() * texture * 0.035F + bubble;
}

float SoundscapeVoice::stone(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    if (tick_event(0.012F + activity * activity * 0.18F + motion * 0.03F, 0.94F)) {
        event_pan_ = noise();
        excite_modes(0.30F + activity * 0.46F, 0.24F + texture * 0.62F);
        primary_envelope_ = 1.0F;
    }
    primary_envelope_ *= decay_coefficient(sample_rate_, 0.012F + color * 0.032F);
    const float impact = noise() * primary_envelope_ * (0.18F + texture * 0.36F);
    const float base = std::clamp(frequency, 28.0F, 220.0F) *
        std::pow(2.0F, event_pan_ * tension * 0.28F);
    const float ring = modal_sum(base, 0.045F + timbre * (0.20F + evolution * 0.22F), color);
    return impact + ring * (0.72F + texture * 0.34F);
}

float SoundscapeVoice::metro_traction(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    approach_phase_ += (0.004F + motion * 0.018F + evolution * 0.012F) * inverse_sample_rate_;
    approach_phase_ -= std::floor(approach_phase_);
    const float run = smoothstep(triangle(approach_phase_));
    const float base = std::clamp(frequency, 18.0F, 120.0F) *
        (0.55F + run * (0.62F + activity * 1.55F));
    const float inverter = 75.0F + run * run * (160.0F + tension * 1'400.0F);
    const float rotor = sine(0U, base) * 0.52F + sine(1U, base * 2.0F) * (0.12F + timbre * 0.20F) +
        sine(2U, base * 6.0F) * texture * 0.08F;
    const float whine = sine(3U, inverter) * (0.010F + color * 0.045F) * std::pow(run, 1.7F);
    const float raw = noise();
    if (control_tick()) configure_filter(0U, 480.0F + run * 2'600.0F, 0.24F + texture * 0.48F);
    filters_[0].Process(raw);
    const float bearings = filters_[0].Band() * texture * (0.04F + run * 0.18F);
    return (rotor + whine + bearings) * (0.42F + run * 0.46F);
}

float SoundscapeVoice::rail_joint(float frequency, float timbre, float color, float motion,
    float texture, float tempo_bpm, float activity, float tension, float evolution) noexcept {
    const float tempo = std::clamp(tempo_bpm, 10.0F, 300.0F) / 60.0F;
    const float rate = 0.42F + activity * 2.8F + motion * 2.0F + tempo * 0.18F;
    step_phase_ += rate * inverse_sample_rate_;
    if (step_phase_ >= 1.0F) {
        step_phase_ -= 1.0F;
        primary_envelope_ = 1.0F;
        secondary_countdown_ = 0.055F + (1.0F - tension) * 0.035F;
        excite_modes(0.14F + texture * 0.20F, 0.55F);
    }
    secondary_countdown_ -= inverse_sample_rate_;
    if (secondary_countdown_ <= 0.0F && secondary_countdown_ > -1.0F) {
        secondary_countdown_ = -2.0F;
        secondary_envelope_ = 0.72F;
        excite_modes(0.08F + texture * 0.15F, 0.70F);
    }
    primary_envelope_ *= decay_coefficient(sample_rate_, 0.018F + timbre * 0.035F);
    secondary_envelope_ *= decay_coefficient(sample_rate_, 0.026F + timbre * 0.045F);
    const float hit = noise() * (primary_envelope_ + secondary_envelope_) * (0.18F + color * 0.28F);
    const float ring = modal_sum(std::clamp(frequency, 38.0F, 190.0F),
        0.028F + timbre * 0.095F + evolution * 0.08F, color);
    return hit + ring * 0.68F;
}

float SoundscapeVoice::brake(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    if (tick_event(0.006F + activity * activity * 0.085F + evolution * 0.014F, 0.88F)) {
        event_age_ = 0.0F;
        event_duration_ = 0.65F + (noise() * 0.5F + 0.5F) * (1.2F + motion * 2.8F);
        event_pan_ = noise();
    }
    event_age_ += inverse_sample_rate_;
    const float progress = event_age_ / std::max(0.1F, event_duration_);
    if (progress >= 1.0F) return 0.0F;
    const float envelope = std::pow(std::sin(progress * kPi), 0.7F + timbre * 1.2F);
    const float sweep = 0.45F + (1.0F - progress) * (1.0F + tension * 5.0F) + event_pan_ * 0.08F;
    const float center = std::clamp(frequency, 80.0F, 650.0F) * (2.0F + color * 6.0F) * sweep;
    const float raw = noise();
    if (control_tick()) {
        configure_filter(0U, center, 0.72F + timbre * 0.24F, texture * 0.22F);
        configure_filter(1U, center * (1.62F + color * 0.7F), 0.58F + texture * 0.34F);
    }
    filters_[0].Process(raw * envelope);
    filters_[1].Process(raw * envelope);
    return filters_[0].Band() * 0.64F + filters_[1].Band() * (0.22F + texture * 0.35F);
}

float SoundscapeVoice::carriage(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    const float raw = noise();
    const float speed = 0.8F + activity * 6.0F + motion * 3.0F;
    const float sway = 0.68F + 0.22F * sine(6U, 0.21F + evolution * 0.34F) +
        0.10F * sine(7U, 0.73F + motion * 1.2F);
    if (control_tick()) {
        configure_filter(0U, 45.0F + color * 180.0F, 0.38F + timbre * 0.36F);
        configure_filter(1U, 420.0F + color * 2'400.0F, 0.28F + texture * 0.46F);
    }
    filters_[0].Process(raw * sway);
    filters_[1].Process(raw);
    const float rattle_gate = std::pow(0.5F + 0.5F * sine(5U, speed), 8.0F + texture * 12.0F);
    const float chassis = sine(0U, std::clamp(frequency, 22.0F, 110.0F) *
        (1.0F + slow_value_ * tension * 0.025F)) * 0.10F;
    return filters_[0].Low() * 0.60F + filters_[1].Band() * rattle_gate *
        (0.10F + texture * 0.34F) + chassis;
}

float SoundscapeVoice::music_box(float frequency, float timbre, float color, float motion,
    float texture, float tempo_bpm, float activity, float tension, float evolution) noexcept {
    const float tempo = std::clamp(tempo_bpm, 10.0F, 300.0F) / 60.0F;
    if (tick_event(0.14F + activity * 0.8F + tempo * 0.12F + motion * 0.18F,
            0.42F + evolution * 0.46F)) {
        constexpr std::array<float, 8> semitones{0.0F, 3.0F, 5.0F, 7.0F, 10.0F, 12.0F, 15.0F, 19.0F};
        const auto index = static_cast<std::size_t>(std::clamp(
            static_cast<int>((noise() * 0.5F + 0.5F) * 8.0F), 0, 7));
        event_pan_ = semitones[index];
        primary_envelope_ = 1.0F;
        excite_modes(0.16F + texture * 0.24F, 0.18F + tension * 0.48F);
    }
    primary_envelope_ *= decay_coefficient(sample_rate_, 0.018F + timbre * 0.055F);
    const float pitch = std::clamp(frequency, 110.0F, 880.0F) * std::pow(2.0F, event_pan_ / 12.0F);
    const float tine = sine(0U, pitch) * primary_envelope_ * 0.46F +
        sine(1U, pitch * (2.01F + color * 1.93F)) * primary_envelope_ * (0.08F + color * 0.20F);
    const float body = modal_sum(pitch * 0.25F, 0.06F + timbre * 0.34F, 0.28F + color * 0.62F);
    return tine + body * (0.48F + texture * 0.28F) + noise() * primary_envelope_ * texture * 0.025F;
}

float SoundscapeVoice::toy_voice(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    if (tick_event(0.015F + activity * activity * 0.11F, 0.84F)) {
        event_age_ = 0.0F;
        event_duration_ = 0.38F + (noise() * 0.5F + 0.5F) * (0.7F + evolution * 1.8F);
        event_pan_ = noise();
    }
    event_age_ += inverse_sample_rate_;
    const float progress = event_age_ / std::max(0.08F, event_duration_);
    if (progress >= 1.0F) return 0.0F;
    const float envelope = std::pow(std::sin(progress * kPi), 0.65F);
    const float syllables = 1.0F + std::floor(progress * (2.0F + motion * 8.0F));
    const float bent = std::sin(progress * kTwoPi * syllables) * (0.04F + tension * 0.38F);
    const float pitch = std::clamp(frequency, 55.0F, 380.0F) *
        std::pow(2.0F, event_pan_ * 0.18F + bent);
    const float pulse = sine(0U, pitch) > (timbre * 1.6F - 0.8F) ? 1.0F : -1.0F;
    const float carrier = sine(1U, pitch * (1.0F + color * 0.08F));
    const float dropout = sine(6U, 3.0F + motion * 17.0F) > (-0.82F + texture * 0.58F) ? 1.0F : 0.18F;
    return (pulse * (0.18F + timbre * 0.14F) + carrier * 0.26F + noise() * texture * 0.07F) *
        envelope * dropout;
}

float SoundscapeVoice::toy_gears(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    const float speed = std::clamp(frequency, 12.0F, 100.0F) * (0.42F + activity * 1.15F);
    step_phase_ += speed * inverse_sample_rate_;
    if (step_phase_ >= 1.0F) {
        step_phase_ -= 1.0F;
        primary_envelope_ = 1.0F;
        if (noise() > 0.72F - evolution * 0.34F) slow_target_ = -1.0F;
    }
    primary_envelope_ *= decay_coefficient(sample_rate_, 0.003F + timbre * 0.018F);
    const float motor = sine(0U, speed) * 0.22F + sine(1U, speed * (3.0F + color * 5.0F)) * 0.09F;
    const float tooth = noise() * primary_envelope_ * (0.14F + texture * 0.38F);
    const float battery = std::clamp(0.72F + slow_value_ * (0.18F + tension * 0.48F) +
        sine(7U, 0.09F + motion * 0.36F) * 0.12F, 0.0F, 1.0F);
    return (motor + tooth) * battery;
}

float SoundscapeVoice::lullaby(float frequency, float timbre, float color, float motion,
    float texture, float tempo_bpm, float activity, float tension, float evolution) noexcept {
    const float tempo = std::clamp(tempo_bpm, 10.0F, 300.0F) / 60.0F;
    if (tick_event(0.055F + activity * 0.34F + tempo * 0.045F, 0.66F + evolution * 0.28F)) {
        constexpr std::array<float, 7> semitones{0.0F, 2.0F, 3.0F, 7.0F, 9.0F, 12.0F, 15.0F};
        const auto index = static_cast<std::size_t>(std::clamp(
            static_cast<int>((noise() * 0.5F + 0.5F) * 7.0F), 0, 6));
        event_pan_ = semitones[index];
        secondary_envelope_ = 1.0F;
        excite_modes(0.10F + texture * 0.16F, 0.24F + tension * 0.48F);
    }
    secondary_envelope_ *= decay_coefficient(sample_rate_, 0.16F + timbre * 0.72F);
    const float pitch = std::clamp(frequency, 130.0F, 720.0F) * std::pow(2.0F, event_pan_ / 12.0F);
    const float glass = sine(0U, pitch * (1.0F + slow_value_ * motion * 0.008F)) * secondary_envelope_ * 0.26F +
        sine(1U, pitch * (2.97F + color * 0.08F)) * secondary_envelope_ * (0.04F + color * 0.11F);
    const float haze = modal_sum(pitch * 0.21F, 0.18F + timbre * 0.58F, color) * 0.32F;
    return glass + haze + noise() * secondary_envelope_ * texture * 0.018F;
}


float SoundscapeVoice::sub_drone(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    const float base = std::clamp(frequency, 16.0F, 92.0F);
    const float drift = slow_value_ * motion * 0.010F +
        sine(7U, 0.012F + evolution * 0.035F) * (0.0015F + tension * 0.006F);
    const float sub = sine(0U, base * 0.5F * (1.0F + drift));
    const float fundamental = sine(1U, base * (1.0F - drift * 0.4F));
    const float detuned = sine(2U, base * (1.002F + timbre * 0.012F + drift));
    const float fifth = sine(3U, base * 1.498F) * (0.015F + color * 0.055F);
    const float raw = noise();
    if (control_tick()) {
        configure_filter(0U, 24.0F + color * 150.0F, 0.18F + timbre * 0.24F);
    }
    filters_[0].Process(raw);
    const float air = filters_[0].Low() * texture * (0.05F + activity * 0.11F);
    const float breathing = std::clamp(0.72F + slow_value_ * 0.14F +
        sine(6U, 0.021F + motion * 0.09F) * 0.12F, 0.34F, 1.0F);
    const float body = sub * (0.48F + timbre * 0.12F) + fundamental * 0.36F +
        detuned * (0.10F + timbre * 0.16F) + fifth + air;
    return std::tanh(body * (1.15F + texture * 1.4F)) * breathing * 0.64F;
}

float SoundscapeVoice::tape_drone(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    const float base = std::clamp(frequency, 20.0F, 170.0F);
    const float wow = sine(6U, 0.09F + motion * 0.38F) * (0.001F + motion * 0.012F) +
        slow_value_ * tension * 0.006F;
    const float flutter = sine(7U, 2.1F + color * 5.8F) * texture * 0.0018F;
    const float a = sine(0U, base * (1.0F + wow + flutter));
    const float b = sine(1U, base * (0.997F - wow * 0.35F));
    const float c = sine(2U, base * (2.0F + timbre * 0.018F)) * (0.04F + color * 0.12F);
    const float raw = noise();
    if (control_tick()) {
        configure_filter(0U, 90.0F + color * 520.0F, 0.12F + timbre * 0.18F);
        configure_filter(1U, 1'200.0F + color * 2'200.0F, 0.08F + texture * 0.16F);
    }
    filters_[0].Process(raw);
    filters_[1].Process(raw);
    const float hiss = filters_[1].Band() * texture * 0.035F;
    const float head_bump = filters_[0].Low() * (0.06F + texture * 0.12F);
    const float dropout = std::clamp(0.84F + slow_value_ * (0.05F + evolution * 0.11F) +
        sine(5U, 0.031F + activity * 0.08F) * 0.06F, 0.52F, 1.0F);
    return std::tanh((a * 0.46F + b * (0.28F + timbre * 0.12F) + c + head_bump + hiss) *
        (1.05F + texture * 1.8F)) * dropout * 0.62F;
}

float SoundscapeVoice::bowed_metal(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    const float base = std::clamp(frequency, 24.0F, 180.0F) *
        (1.0F + slow_value_ * motion * 0.018F);
    const float raw = noise();
    const float bow = raw * (0.22F + texture * 0.52F) +
        sine(7U, 0.18F + activity * 0.65F) * (0.04F + motion * 0.09F);
    if (control_tick()) {
        configure_filter(0U, base, 0.78F + timbre * 0.18F, texture * 0.08F);
        configure_filter(1U, base * (1.43F + color * 0.18F), 0.72F + timbre * 0.22F);
        configure_filter(2U, base * (2.05F + color * 0.38F), 0.58F + timbre * 0.28F);
        configure_filter(3U, 180.0F + color * 520.0F, 0.18F + texture * 0.25F);
    }
    filters_[0].Process(bow);
    filters_[1].Process(bow);
    filters_[2].Process(bow);
    filters_[3].Process(raw);
    const float body = sine(0U, base * 0.5F) * (0.10F + tension * 0.12F);
    const float resonances = filters_[0].Band() * 0.42F + filters_[1].Band() * 0.27F +
        filters_[2].Band() * (0.10F + color * 0.10F);
    const float scrape = filters_[3].Low() * texture * 0.12F;
    const float swell = std::clamp(0.66F + slow_value_ * 0.18F +
        sine(6U, 0.024F + evolution * 0.07F) * 0.16F, 0.25F, 1.0F);
    return std::tanh((body + resonances + scrape) * (1.0F + texture * 1.1F)) * swell * 0.78F;
}

float SoundscapeVoice::earth_rumble(float frequency, float timbre, float color, float motion,
    float texture, float activity, float tension, float evolution) noexcept {
    if (tick_event(0.006F + activity * activity * 0.045F + evolution * 0.012F, 0.96F)) {
        primary_envelope_ = 1.0F;
    }
    primary_envelope_ *= decay_coefficient(sample_rate_, 0.32F + timbre * 1.8F);
    const float raw = noise();
    if (control_tick()) {
        configure_filter(0U, 18.0F + color * 92.0F, 0.30F + timbre * 0.32F);
        configure_filter(1U, 55.0F + color * 210.0F, 0.36F + texture * 0.30F);
    }
    filters_[0].Process(raw);
    filters_[1].Process(raw);
    const float base = std::clamp(frequency, 12.0F, 68.0F) *
        (1.0F + slow_value_ * tension * 0.014F);
    const float sub = sine(0U, base) * (0.18F + tension * 0.18F) +
        sine(1U, base * 0.5F) * 0.22F;
    const float ground = filters_[0].Low() * (0.62F + texture * 0.24F) +
        filters_[1].Band() * (0.08F + color * 0.16F);
    const float impact = raw * primary_envelope_ * (0.05F + activity * 0.16F);
    const float heave = std::clamp(0.70F + slow_value_ * 0.17F +
        sine(6U, 0.009F + motion * 0.035F) * 0.13F, 0.32F, 1.0F);
    return std::tanh((sub + ground + impact) * (1.0F + texture * 0.9F)) * heave * 0.72F;
}

} // namespace cursed_drone::detail

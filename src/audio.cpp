// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.
#include "cursed_drone/audio.hpp"
#include "plaits_actor.hpp"
#include "soundscape.hpp"

#include "Noise/clockednoise.h"
#include "Noise/grainlet.h"
#include "Noise/particle.h"
#include "PhysicalModeling/drip.h"
#include "PhysicalModeling/modalvoice.h"
#include "Synthesis/oscillator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace cursed_drone {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kTwoPi = kPi * 2.0F;

float clamp01(float value) noexcept {
    return std::clamp(value, 0.0F, 1.0F);
}

float next_noise(std::uint32_t& state) noexcept {
    state ^= state << 13U;
    state ^= state >> 17U;
    state ^= state << 5U;
    return static_cast<float>(state) * (2.0F / 4'294'967'295.0F) - 1.0F;
}

// Low-frequency modulation does not need libm-grade sine accuracy. This
// approximation stays smooth and bounded while avoiding hundreds of thousands
// of transcendental calls per second on the handheld CPU.
float fast_sine_lfo(float phase) noexcept {
    phase -= std::floor(phase);
    float x = phase * kTwoPi;
    if (x > kPi) x -= kTwoPi;
    constexpr float b = 4.0F / kPi;
    constexpr float c = -4.0F / (kPi * kPi);
    float y = b * x + c * x * std::abs(x);
    constexpr float correction = 0.225F;
    y = correction * (y * std::abs(y) - y) + y;
    return y;
}

struct SmoothedValue {
    float current{0.0F};
    float target{0.0F};
    float coefficient{1.0F};

    void prepare(float initial, float sample_rate, float milliseconds = 20.0F) noexcept {
        current = initial;
        target = initial;
        const float samples = std::max(1.0F, sample_rate * milliseconds * 0.001F);
        coefficient = 1.0F - std::exp(-1.0F / samples);
    }

    void set(float value) noexcept { target = value; }

    float next() noexcept {
        current += (target - current) * coefficient;
        return current;
    }

    void snap() noexcept { current = target; }
};

class ModulatorRuntime {
public:
    void reset() noexcept {
        phase_ = 0.0F;
        held_ = 0.0F;
        walk_ = 0.0F;
    }

    float next(const ModSettings& settings, float sample_rate, float rate_modulation = 0.0F) noexcept {
        if (!settings.enabled) {
            return 0.0F;
        }

        float value = 0.0F;
        switch (settings.wave) {
        case ModWave::sine:
            value = fast_sine_lfo(phase_);
            break;
        case ModWave::triangle:
            value = 1.0F - 4.0F * std::abs(phase_ - 0.5F);
            break;
        case ModWave::sample_hold:
            value = held_;
            break;
        case ModWave::random_walk:
            value = walk_;
            break;
        }

        const float rate = std::clamp(settings.rate_hz, 0.001F, 40.0F) *
            std::pow(2.0F, std::clamp(rate_modulation, -1.0F, 1.0F) * 4.0F);
        phase_ += std::clamp(rate, 0.0002F, 80.0F) / sample_rate;
        if (phase_ >= 1.0F) {
            phase_ -= std::floor(phase_);
            held_ = next_noise(random_state_);
            walk_ = std::clamp(walk_ + next_noise(random_state_) * 0.22F, -1.0F, 1.0F);
        }
        return settings.offset + value * settings.depth;
    }

private:
    float phase_{0.0F};
    float held_{0.0F};
    float walk_{0.0F};
    std::uint32_t random_state_{0xA341'316CU};
};

class EuclideanRuntime {
public:
    void reset() noexcept {
        phase_ = 1.0F;
        step_ = -1;
        random_state_ = 0xE0C1'1DEA;
    }

    bool next(const EuclideanSettings& settings, float tempo_bpm, float sample_rate) noexcept {
        if (!settings.enabled) return false;
        const int steps = std::clamp(settings.steps, 1, 32);
        const int pulses = std::clamp(settings.pulses, 0, steps);
        const float rate = std::clamp(tempo_bpm, 10.0F, 300.0F) / 60.0F * 4.0F;
        phase_ += rate / sample_rate;
        if (phase_ < 1.0F) return false;
        phase_ -= std::floor(phase_);
        step_ = (step_ + 1) % steps;
        const int rotated = (step_ + std::clamp(settings.rotation, 0, steps - 1)) % steps;
        const bool hit = pulses > 0 && ((rotated * pulses) % steps) < pulses;
        if (!hit) return false;
        const float random_unit = next_noise(random_state_) * 0.5F + 0.5F;
        return random_unit <= clamp01(settings.probability);
    }

private:
    float phase_{1.0F};
    int step_{-1};
    std::uint32_t random_state_{0xE0C1'1DEAU};
};

class DiagnosticEngine {
public:
    void prepare(std::size_t style) noexcept {
        style_ = style % kSlotCount;
        random_state_ = 0xC801'3EA4U ^ (0x9E37'79B9U * static_cast<std::uint32_t>(style_ + 1U));
        reset();
    }

    void reset() noexcept {
        phase_a_ = 0.0F;
        phase_b_ = 0.0F;
        noise_filter_ = 0.0F;
        particle_ = 0.0F;
    }

    float next(float frequency, float timbre, float color, float motion, float texture, float sample_rate) noexcept {
        frequency = std::clamp(frequency, 20.0F, sample_rate * 0.2F);
        phase_a_ += frequency / sample_rate;
        phase_b_ += frequency * (1.001F + color * 0.127F) / sample_rate;
        phase_a_ -= std::floor(phase_a_);
        phase_b_ -= std::floor(phase_b_);

        const float fundamental = std::sin(phase_a_ * kTwoPi);
        const float overtone = std::sin(phase_b_ * kTwoPi * (1.0F + timbre * 3.0F));
        const float raw_noise = next_noise(random_state_);
        const float noise_coefficient = 0.0007F + motion * motion * 0.07F;
        noise_filter_ += (raw_noise - noise_filter_) * noise_coefficient;
        switch (style_) {
        case 0U: {
            const float body = fundamental * (0.82F - timbre * 0.32F) + overtone * (0.08F + timbre * 0.30F);
            return body * (1.0F - texture * 0.45F) + noise_filter_ * texture * 1.7F;
        }
        case 1U: {
            const float threshold = 0.08F + timbre * 0.84F;
            const float pulse = phase_b_ < threshold ? 1.0F : -1.0F;
            return fundamental * (0.48F - texture * 0.12F) + pulse * (0.10F + timbre * 0.20F) +
                noise_filter_ * texture * 1.35F;
        }
        case 2U: {
            const float modulator = std::sin(phase_b_ * kTwoPi) * (0.5F + timbre * 7.0F);
            const float fm = std::sin(phase_a_ * kTwoPi + modulator);
            return fm * (0.58F - texture * 0.16F) + overtone * 0.16F + noise_filter_ * texture * 1.1F;
        }
        default: {
            particle_ *= 0.992F - motion * 0.018F;
            const float trigger = 0.985F - motion * motion * 0.25F;
            if (raw_noise > trigger) {
                particle_ = 0.35F + 0.65F * std::abs(next_noise(random_state_));
            }
            return fundamental * (0.22F - texture * 0.10F) + noise_filter_ * (0.35F + texture * 1.6F) +
                particle_ * overtone * (0.25F + timbre * 0.55F);
        }
        }
    }

private:
    float phase_a_{0.0F};
    float phase_b_{0.0F};
    float noise_filter_{0.0F};
    float particle_{0.0F};
    std::size_t style_{0U};
    std::uint32_t random_state_{0xC801'3EA4U};
};

class ProductEngine {
public:
    void prepare(float sample_rate, std::size_t slot_index) noexcept {
        sample_rate_ = sample_rate;
        slot_index_ = slot_index;
        random_state_ = 0x91E1'0DA5U ^ (0x9E37'79B9U * static_cast<std::uint32_t>(slot_index + 1U));
        plaits_.prepare(sample_rate_);
        reset();
    }

    void reset() noexcept {
        diagnostic_.prepare(slot_index_);
        fundamental_.Init(sample_rate_);
        overtone_.Init(sample_rate_);
        fundamental_.SetAmp(1.0F);
        overtone_.SetAmp(1.0F);
        body_.Init(sample_rate_);
        grain_.Init(sample_rate_);
        particle_.Init(sample_rate_);
        drip_.Init(sample_rate_, 0.72F);
        clocked_noise_.Init(sample_rate_);
        soundscape_.prepare(sample_rate_, slot_index_);
        event_phase_ = 1.0F;
        event_period_ = 1.0F;
        cached_event_rate_ = 0.0F;
        event_control_counter_ = 0U;
        cached_event_kind_ = EngineKind::diagnostic;
        cached_triggerable_ = false;
        grain_envelope_ = 0.0F;
        grain_decay_ = 0.999F;
        grain_control_counter_ = 0U;
        drift_phase_ = 0.0F;
        soundscape_.reset();
        plaits_.reset();
        event_fired_ = false;
    }

    float next_mono(
        EngineKind kind,
        float frequency,
        float timbre,
        float color,
        float motion,
        float texture,
        float tempo_bpm,
        float pulse,
        float chaos,
        float events,
        bool external_trigger) noexcept {
        frequency = std::clamp(frequency, 20.0F, sample_rate_ * 0.2F);
        timbre = clamp01(timbre);
        color = clamp01(color);
        motion = clamp01(motion);
        texture = clamp01(texture);
        events = clamp01(events);

        if (event_control_counter_ == 0U || kind != cached_event_kind_) {
            cached_event_kind_ = kind;
            cached_triggerable_ = supports_event_rate(kind);
            cached_event_rate_ = cached_triggerable_
                ? event_rate_hz(tempo_bpm, events, motion)
                : 0.0F;
            event_control_counter_ = 31U;
        } else {
            --event_control_counter_;
        }
        const bool triggerable = cached_triggerable_;
        if (triggerable) event_phase_ += cached_event_rate_ / sample_rate_;
        bool trigger = triggerable && external_trigger;
        if (triggerable && event_phase_ >= event_period_) {
            event_phase_ -= event_period_;
            const float random_unit = next_noise(random_state_) * 0.5F + 0.5F;
            const float irregularity = (0.18F + chaos * 0.72F) * (1.0F - pulse * 0.72F);
            event_period_ = std::clamp(1.0F + (random_unit * 2.0F - 1.0F) * irregularity, 0.35F, 1.8F);
            trigger = true;
            grain_envelope_ = 1.0F;
        }
        event_fired_ = trigger;

        drift_phase_ += (0.007F + motion * motion * 0.19F) / sample_rate_;
        drift_phase_ -= std::floor(drift_phase_);
        const float drift = fast_sine_lfo(drift_phase_) * (0.0005F + motion * 0.018F);

        switch (kind) {
        case EngineKind::plaits:
            return 0.0F;
        case EngineKind::diagnostic:
            return diagnostic_.next(frequency, timbre, color, motion, texture, sample_rate_);
        case EngineKind::macro: {
            fundamental_.SetFreq(frequency * (1.0F + drift));
            fundamental_.SetWaveform(daisysp::Oscillator::WAVE_SIN);
            overtone_.SetFreq(frequency * (1.001F + color * 0.021F - drift * 0.5F));
            overtone_.SetPw(0.08F + color * 0.84F);
            overtone_.SetWaveform(color < 0.55F
                    ? daisysp::Oscillator::WAVE_POLYBLEP_SAW
                    : daisysp::Oscillator::WAVE_POLYBLEP_SQUARE);
            clocked_noise_.SetFreq(frequency * (2.0F + texture * 38.0F));
            const float clean = fundamental_.Process();
            const float edged = overtone_.Process();
            const float noise = clocked_noise_.Process();
            return clean * (0.92F - timbre * 0.58F) + edged * (0.08F + timbre * 0.50F) +
                noise * texture * texture * 0.16F;
        }
        case EngineKind::body:
            body_.SetFreq(frequency);
            body_.SetAccent(0.28F + texture * 0.72F);
            body_.SetStructure(timbre);
            body_.SetBrightness(color * 0.72F + texture * 0.28F);
            body_.SetDamping(std::clamp(0.28F + (1.0F - motion) * 0.58F, 0.0F, 1.0F));
            body_.SetSustain(texture > 0.92F);
            return body_.Process(trigger) * 2.4F;
        case EngineKind::grain: {
            grain_.SetFreq(frequency * (1.0F + drift * 2.0F));
            grain_.SetFormantFreq(std::min(sample_rate_ * 0.22F, frequency * (1.5F + color * 30.0F)));
            grain_.SetShape(timbre * 2.85F);
            grain_.SetBleed(0.03F + texture * 0.92F);
            clocked_noise_.SetFreq(frequency * (1.0F + texture * 52.0F));
            if (grain_control_counter_ == 0U) {
                grain_decay_ = std::exp(-1.0F /
                    (sample_rate_ * (0.025F + (1.0F - motion) * 0.48F)));
                grain_control_counter_ = 31U;
            } else {
                --grain_control_counter_;
            }
            grain_envelope_ *= grain_decay_;
            const float gate = 0.12F + grain_envelope_ * (0.42F + events * 0.46F);
            return grain_.Process() * gate * 1.7F + clocked_noise_.Process() * texture * 0.12F;
        }
        case EngineKind::particle:
            particle_.SetFreq(frequency * (1.0F + drift * 3.0F));
            particle_.SetResonance(0.52F + timbre * 0.47F);
            particle_.SetDensity(std::clamp(0.015F + texture * texture * 0.78F + events * 0.18F, 0.0F, 1.0F));
            particle_.SetGain(0.72F);
            particle_.SetSpread(1.0F + color * 38.0F);
            particle_.SetRandomFreq(0.15F + motion * motion * 12.0F);
            particle_.SetSync(trigger);
            return particle_.Process() * 2.2F;
        case EngineKind::water_drip:
            drip_.SetFrequency(std::clamp(frequency, 120.0F, 1'800.0F) * (0.78F + color * 0.64F));
            drip_.SetDamping(0.08F + timbre * 0.52F);
            drip_.SetDensity(0.12F + texture * 0.72F);
            return drip_.Process(trigger) * (4.2F + texture * 2.8F);
        case EngineKind::derelict_bed:
        case EngineKind::footsteps:
        case EngineKind::door:
        case EngineKind::pipe:
        case EngineKind::motor:
        case EngineKind::machinery:
        case EngineKind::crowd:
        case EngineKind::metal:
        case EngineKind::wind:
        case EngineKind::birds:
        case EngineKind::insects:
        case EngineKind::signal:
        case EngineKind::cave_air:
        case EngineKind::water_flow:
        case EngineKind::stone:
        case EngineKind::metro_traction:
        case EngineKind::rail_joint:
        case EngineKind::brake:
        case EngineKind::carriage:
        case EngineKind::music_box:
        case EngineKind::toy_voice:
        case EngineKind::toy_gears:
        case EngineKind::lullaby:
        case EngineKind::sub_drone:
        case EngineKind::tape_drone:
        case EngineKind::bowed_metal:
        case EngineKind::earth_rumble:
            {
                const float sample = soundscape_.next(
                    kind, frequency, timbre, color, motion, texture,
                    tempo_bpm, pulse, chaos, events, trigger);
                event_fired_ = soundscape_.consume_event_fired();
                return sample;
            }
        }
        return 0.0F;
    }

    StereoFrame next(
        const SlotSettings& settings,
        float frequency,
        float timbre,
        float color,
        float motion,
        float texture,
        float tempo_bpm,
        float pulse,
        float chaos,
        float events,
        bool external_trigger,
        bool force_trigger_patch = false) noexcept {
        event_fired_ = false;
        if (settings.engine == EngineKind::plaits) {
            event_fired_ = external_trigger;
            return plaits_.next(
                settings.plaits_model,
                settings.plaits_output,
                settings.tuning,
                frequency,
                timbre,
                color,
                motion,
                texture,
                external_trigger,
                settings.euclidean.enabled || force_trigger_patch);
        }
        const float mono = next_mono(
            settings.engine, frequency, timbre, color, motion, texture,
            tempo_bpm, pulse, chaos, events, external_trigger);
        return {mono, mono};
    }

    [[nodiscard]] bool consume_event_fired() noexcept {
        const bool fired = event_fired_;
        event_fired_ = false;
        return fired;
    }

private:
    float sample_rate_{48'000.0F};
    std::size_t slot_index_{0U};
    std::uint32_t random_state_{0x91E1'0DA5U};
    float event_phase_{1.0F};
    float event_period_{1.0F};
    float cached_event_rate_{0.0F};
    unsigned event_control_counter_{0U};
    EngineKind cached_event_kind_{EngineKind::diagnostic};
    bool cached_triggerable_{false};
    float grain_envelope_{0.0F};
    float grain_decay_{0.999F};
    unsigned grain_control_counter_{0U};
    float drift_phase_{0.0F};
    bool event_fired_{false};
    DiagnosticEngine diagnostic_{};
    daisysp::Oscillator fundamental_{};
    daisysp::Oscillator overtone_{};
    daisysp::ModalVoice body_{};
    daisysp::GrainletOscillator grain_{};
    daisysp::Particle particle_{};
    daisysp::Drip drip_{};
    daisysp::ClockedNoise clocked_noise_{};
    detail::SoundscapeVoice soundscape_{};
    detail::PlaitsActor plaits_{};
};

class EffectRuntime {
public:
    void prepare(float sample_rate) {
        sample_rate_ = sample_rate;
        const auto delay_frames = static_cast<std::size_t>(std::ceil(sample_rate_ * 2.4F)) + 2U;
        const auto short_frames = static_cast<std::size_t>(std::ceil(sample_rate_ * 0.080F)) + 2U;
        delay_.assign(delay_frames, {});
        short_delay_.assign(short_frames, {});
        reset();
    }

    void reset() noexcept {
        lowpass_ = {};
        highpass_low_ = {};
        diffuser_damp_ = {};
        phaser_feedback_ = {};
        tremolo_phase_ = 0.0F;
        ring_phase_ = 0.0F;
        chorus_phase_ = 0.0F;
        flanger_phase_ = 0.0F;
        phaser_phase_ = 0.0F;
        envelope_phase_ = 0.0F;
        reverse_phase_a_ = 0.0F;
        reverse_phase_b_ = 0.5F;
        reverse_offset_a_ = 0.25F;
        reverse_offset_b_ = 0.58F;
        delay_write_ = 0U;
        short_write_ = 0U;
        crusher_counter_ = 0U;
        crusher_held_ = {};
        drive_control_counter_ = 0U;
        lowpass_control_counter_ = 0U;
        highpass_control_counter_ = 0U;
        delay_control_counter_ = 0U;
        ring_control_counter_ = 0U;
        comb_control_counter_ = 0U;
        phaser_control_counter_ = 0U;
        drive_normalizer_ = 1.0F;
        lowpass_coefficient_ = 0.1F;
        highpass_coefficient_ = 0.1F;
        delay_frames_ = 1.0F;
        ring_rate_ = 1.0F;
        comb_frames_ = 1.0F;
        phaser_coefficient_ = 0.0F;
        phaser_left_.fill(0.0F);
        phaser_right_.fill(0.0F);
        // Do not clear the large delay buffers here. reset() can run from the
        // audio callback (Kill Silence), so an O(buffer size) memset would miss
        // the realtime deadline. History counters make pre-reset samples
        // logically invalid until they have been overwritten.
        delay_valid_ = 0U;
        short_valid_ = 0U;
    }

    StereoFrame process(StereoFrame input, EffectKind kind, float amount, float tone, float feedback) noexcept {
        amount = clamp01(amount);
        tone = clamp01(tone);
        feedback = clamp01(feedback);

        switch (kind) {
        case EffectKind::bypass: return input;
        case EffectKind::drive: return drive(input, amount);
        case EffectKind::lowpass: return lowpass(input, amount, tone);
        case EffectKind::highpass: return highpass(input, amount, tone);
        case EffectKind::tremolo: return tremolo(input, amount, tone);
        case EffectKind::delay: return delay(input, amount, tone, feedback);
        case EffectKind::crusher: return crusher(input, amount, tone);
        case EffectKind::wavefolder: return wavefolder(input, amount, tone);
        case EffectKind::ringmod: return ringmod(input, amount, tone);
        case EffectKind::comb: return comb(input, amount, tone, feedback);
        case EffectKind::chorus: return chorus(input, amount, tone, feedback);
        case EffectKind::flanger: return flanger(input, amount, tone, feedback);
        case EffectKind::phaser: return phaser(input, amount, tone, feedback);
        case EffectKind::diffuser: return diffuser(input, amount, tone, feedback);
        case EffectKind::ahdr: return ahdr(input, amount, tone, feedback);
        case EffectKind::tape_void: return tape_void(input, amount, tone, feedback);
        case EffectKind::black_hole: return black_hole(input, amount, tone, feedback);
        case EffectKind::ritual_gate: return ritual_gate(input, amount, tone, feedback);
        case EffectKind::rust_cloud: return rust_cloud(input, amount, tone, feedback);
        case EffectKind::deep_sea: return deep_sea(input, amount, tone, feedback);
        case EffectKind::granular_reverse: return granular_reverse(input, amount, tone, feedback);
        }
        return input;
    }

private:
    static StereoFrame mix(StereoFrame dry, StereoFrame wet, float amount) noexcept {
        return {
            dry.left + (wet.left - dry.left) * amount,
            dry.right + (wet.right - dry.right) * amount,
        };
    }

    static float advance(float& phase, float rate, float sample_rate) noexcept {
        phase += rate / sample_rate;
        phase -= std::floor(phase);
        return phase;
    }

    static bool control_due(unsigned& counter) noexcept {
        if (counter == 0U) {
            counter = 15U;
            return true;
        }
        --counter;
        return false;
    }

    StereoFrame read_delay(
        const std::vector<StereoFrame>& buffer,
        std::size_t write,
        float frames,
        std::size_t valid_frames) const noexcept {
        if (buffer.size() < 2U) return {};
        frames = std::clamp(frames, 1.0F, static_cast<float>(buffer.size() - 2U));
        if (valid_frames < static_cast<std::size_t>(std::ceil(frames))) return {};
        const float position = static_cast<float>(write) - frames;
        float wrapped = position;
        while (wrapped < 0.0F) wrapped += static_cast<float>(buffer.size());
        while (wrapped >= static_cast<float>(buffer.size())) wrapped -= static_cast<float>(buffer.size());
        const auto first = static_cast<std::size_t>(wrapped);
        const auto second = (first + 1U) % buffer.size();
        const float fraction = wrapped - static_cast<float>(first);
        return {
            buffer[first].left + (buffer[second].left - buffer[first].left) * fraction,
            buffer[first].right + (buffer[second].right - buffer[first].right) * fraction,
        };
    }

    StereoFrame drive(StereoFrame input, float amount) noexcept {
        const float gain = 1.0F + amount * 48.0F;
        if (control_due(drive_control_counter_)) {
            drive_normalizer_ = 1.0F / std::max(0.001F, std::tanh(gain));
        }
        const StereoFrame wet{
            std::tanh(input.left * gain) * drive_normalizer_,
            std::tanh(input.right * gain) * drive_normalizer_,
        };
        return mix(input, wet, amount);
    }

    StereoFrame lowpass(StereoFrame input, float amount, float tone) noexcept {
        if (control_due(lowpass_control_counter_)) {
            const float cutoff = std::min(sample_rate_ * 0.42F, 18.0F * std::pow(1'000.0F, tone));
            lowpass_coefficient_ = 1.0F - std::exp(-kTwoPi * cutoff / sample_rate_);
        }
        lowpass_.left += (input.left - lowpass_.left) * lowpass_coefficient_;
        lowpass_.right += (input.right - lowpass_.right) * lowpass_coefficient_;
        return mix(input, lowpass_, amount);
    }

    StereoFrame highpass(StereoFrame input, float amount, float tone) noexcept {
        if (control_due(highpass_control_counter_)) {
            const float cutoff = std::min(sample_rate_ * 0.42F, 12.0F * std::pow(1'000.0F, tone));
            highpass_coefficient_ = 1.0F - std::exp(-kTwoPi * cutoff / sample_rate_);
        }
        highpass_low_.left += (input.left - highpass_low_.left) * highpass_coefficient_;
        highpass_low_.right += (input.right - highpass_low_.right) * highpass_coefficient_;
        return mix(input, {input.left - highpass_low_.left, input.right - highpass_low_.right}, amount);
    }

    StereoFrame tremolo(StereoFrame input, float amount, float tone) noexcept {
        const float rate = 0.015F + tone * tone * 18.0F;
        const float phase = advance(tremolo_phase_, rate, sample_rate_);
        const float wave = 0.5F + 0.5F * fast_sine_lfo(phase);
        const float modulation = 1.0F - amount * (0.20F + wave * 0.80F);
        return {input.left * modulation, input.right * modulation};
    }

    StereoFrame delay(StereoFrame input, float amount, float tone, float feedback) noexcept {
        if (delay_.empty()) return input;
        if (control_due(delay_control_counter_)) {
            const float delay_seconds = 0.025F + std::pow(tone, 1.35F) * 1.265F;
            delay_frames_ = delay_seconds * sample_rate_;
        }
        const StereoFrame wet = read_delay(delay_, delay_write_, delay_frames_, delay_valid_);
        const float regeneration = std::min(0.985F, feedback * 0.985F);
        delay_[delay_write_] = {
            input.left + wet.right * regeneration,
            input.right + wet.left * regeneration,
        };
        delay_write_ = (delay_write_ + 1U) % delay_.size();
        delay_valid_ = std::min(delay_valid_ + 1U, delay_.size());
        return mix(input, wet, amount);
    }

    StereoFrame crusher(StereoFrame input, float amount, float tone) noexcept {
        const auto hold_samples = static_cast<unsigned>(1.0F + tone * tone * 255.0F);
        if (crusher_counter_ == 0U) {
            const float levels = std::pow(2.0F, 16.0F - amount * 15.0F);
            crusher_held_ = {
                std::round(input.left * levels) / levels,
                std::round(input.right * levels) / levels,
            };
            crusher_counter_ = hold_samples;
        }
        --crusher_counter_;
        return mix(input, crusher_held_, amount);
    }

    static float fold_sample(float input) noexcept {
        const float wrapped = std::fmod(input + 1.0F, 4.0F);
        const float positive = wrapped < 0.0F ? wrapped + 4.0F : wrapped;
        return 1.0F - std::abs(positive - 2.0F);
    }

    static StereoFrame wavefolder(StereoFrame input, float amount, float tone) noexcept {
        const float gain = 1.0F + amount * (3.0F + tone * 30.0F);
        return mix(input, {fold_sample(input.left * gain), fold_sample(input.right * gain)}, amount);
    }

    StereoFrame ringmod(StereoFrame input, float amount, float tone) noexcept {
        if (control_due(ring_control_counter_)) {
            ring_rate_ = 0.05F * std::pow(160'000.0F, tone);
        }
        const float phase = advance(ring_phase_, ring_rate_, sample_rate_);
        const float carrier = std::sin(phase * kTwoPi);
        return {
            input.left * (1.0F - amount + carrier * amount),
            input.right * (1.0F - amount - carrier * amount),
        };
    }

    StereoFrame comb(StereoFrame input, float amount, float tone, float feedback) noexcept {
        if (delay_.empty()) return input;
        if (control_due(comb_control_counter_)) {
            comb_frames_ = 0.0007F * std::pow(90.0F, tone) * sample_rate_;
        }
        const StereoFrame wet = read_delay(delay_, delay_write_, comb_frames_, delay_valid_);
        const float regeneration = 0.12F + feedback * 0.85F;
        delay_[delay_write_] = {
            input.left + wet.left * regeneration,
            input.right + wet.right * regeneration,
        };
        delay_write_ = (delay_write_ + 1U) % delay_.size();
        delay_valid_ = std::min(delay_valid_ + 1U, delay_.size());
        return mix(input, wet, amount);
    }

    StereoFrame chorus(StereoFrame input, float amount, float tone, float feedback) noexcept {
        if (short_delay_.empty()) return input;
        const float rate = 0.015F + tone * tone * 4.5F;
        const float phase = advance(chorus_phase_, rate, sample_rate_);
        const float depth_ms = 1.0F + feedback * 14.0F;
        const float base_ms = 7.0F + feedback * 17.0F;
        const float left_frames = (base_ms + fast_sine_lfo(phase) * depth_ms) * 0.001F * sample_rate_;
        const float right_frames = (base_ms + fast_sine_lfo(phase + 0.27F) * depth_ms) * 0.001F * sample_rate_;
        const StereoFrame left_tap = read_delay(short_delay_, short_write_, left_frames, short_valid_);
        const StereoFrame right_tap = read_delay(short_delay_, short_write_, right_frames, short_valid_);
        const StereoFrame wet{left_tap.left * 0.72F + right_tap.left * 0.28F,
                              right_tap.right * 0.72F + left_tap.right * 0.28F};
        short_delay_[short_write_] = input;
        short_write_ = (short_write_ + 1U) % short_delay_.size();
        short_valid_ = std::min(short_valid_ + 1U, short_delay_.size());
        return mix(input, wet, amount);
    }

    StereoFrame flanger(StereoFrame input, float amount, float tone, float feedback) noexcept {
        if (short_delay_.empty()) return input;
        const float rate = 0.025F + tone * tone * 7.0F;
        const float phase = advance(flanger_phase_, rate, sample_rate_);
        const float delay_ms = 0.35F + (0.5F + 0.5F * fast_sine_lfo(phase)) * (1.5F + tone * 7.0F);
        const StereoFrame wet = read_delay(short_delay_, short_write_, delay_ms * 0.001F * sample_rate_, short_valid_);
        const float regeneration = feedback * 0.92F;
        short_delay_[short_write_] = {
            input.left + wet.right * regeneration,
            input.right + wet.left * regeneration,
        };
        short_write_ = (short_write_ + 1U) % short_delay_.size();
        short_valid_ = std::min(short_valid_ + 1U, short_delay_.size());
        return mix(input, wet, amount);
    }

    static float allpass(float input, std::array<float, 4>& states, float coefficient) noexcept {
        for (auto& state : states) {
            const float output = -coefficient * input + state;
            state = input + coefficient * output;
            input = output;
        }
        return input;
    }

    StereoFrame phaser(StereoFrame input, float amount, float tone, float feedback) noexcept {
        const float rate = 0.01F + tone * tone * 5.5F;
        const float phase = advance(phaser_phase_, rate, sample_rate_);
        if (control_due(phaser_control_counter_)) {
            const float sweep = clamp01(0.5F + 0.5F * fast_sine_lfo(phase));
            const float frequency = std::min(sample_rate_ * 0.18F, 45.0F * std::pow(120.0F, sweep));
            const float tangent = std::tan(kPi * frequency / sample_rate_);
            phaser_coefficient_ = std::clamp(
                (1.0F - tangent) / (1.0F + tangent), -0.98F, 0.98F);
        }
        const StereoFrame driven{
            input.left + phaser_feedback_.left * feedback * 0.88F,
            input.right + phaser_feedback_.right * feedback * 0.88F,
        };
        const StereoFrame wet{
            allpass(driven.left, phaser_left_, phaser_coefficient_),
            allpass(driven.right, phaser_right_, -phaser_coefficient_),
        };
        phaser_feedback_ = wet;
        return mix(input, wet, amount);
    }

    StereoFrame diffuser(StereoFrame input, float amount, float tone, float feedback) noexcept {
        if (delay_.empty()) return input;
        const float base = (0.012F + tone * 0.105F) * sample_rate_;
        const StereoFrame a = read_delay(delay_, delay_write_, base, delay_valid_);
        const StereoFrame b = read_delay(delay_, delay_write_, base * 1.37F, delay_valid_);
        const StereoFrame c = read_delay(delay_, delay_write_, base * 1.91F, delay_valid_);
        const StereoFrame d = read_delay(delay_, delay_write_, base * 2.53F, delay_valid_);
        const StereoFrame wet{
            (a.left - b.right + c.left + d.right) * 0.38F,
            (a.right + b.left - c.right + d.left) * 0.38F,
        };
        const float damping = 0.04F + (1.0F - tone) * 0.18F;
        diffuser_damp_.left += (wet.left - diffuser_damp_.left) * damping;
        diffuser_damp_.right += (wet.right - diffuser_damp_.right) * damping;
        delay_[delay_write_] = {
            input.left + diffuser_damp_.right * feedback * 0.91F,
            input.right + diffuser_damp_.left * feedback * 0.91F,
        };
        delay_write_ = (delay_write_ + 1U) % delay_.size();
        delay_valid_ = std::min(delay_valid_ + 1U, delay_.size());
        return mix(input, diffuser_damp_, amount);
    }

    float envelope(float tone, float feedback) noexcept {
        const float rate = 0.025F + tone * tone * 3.2F;
        const float phase = advance(envelope_phase_, rate, sample_rate_);
        const float attack_end = 0.05F + feedback * 0.08F;
        const float hold_end = attack_end + 0.05F + feedback * 0.20F;
        const float decay_end = std::min(0.82F, hold_end + 0.18F + (1.0F - tone) * 0.30F);
        const float sustain = 0.08F + feedback * 0.64F;
        if (phase < attack_end) return phase / attack_end;
        if (phase < hold_end) return 1.0F;
        if (phase < decay_end) {
            const float progress = (phase - hold_end) / std::max(0.001F, decay_end - hold_end);
            return 1.0F + (sustain - 1.0F) * progress;
        }
        const float progress = (phase - decay_end) / std::max(0.001F, 1.0F - decay_end);
        return sustain * (1.0F - progress);
    }

    StereoFrame ahdr(StereoFrame input, float amount, float tone, float feedback) noexcept {
        const float env = envelope(tone, feedback);
        const float gain = 1.0F - amount + amount * env;
        return {input.left * gain, input.right * gain};
    }

    StereoFrame tape_void(StereoFrame input, float amount, float tone, float feedback) noexcept {
        StereoFrame wet = chorus(input, 0.45F + amount * 0.45F, tone * 0.55F, 0.35F + feedback * 0.55F);
        wet = drive(wet, amount * (0.18F + tone * 0.32F));
        wet = lowpass(wet, 0.35F + amount * 0.55F, 0.20F + tone * 0.45F);
        wet = delay(wet, 0.20F + amount * 0.65F, 0.25F + tone * 0.60F, 0.45F + feedback * 0.52F);
        return mix(input, wet, amount);
    }

    StereoFrame black_hole(StereoFrame input, float amount, float tone, float feedback) noexcept {
        StereoFrame wet = phaser(input, 0.45F + amount * 0.50F, tone * 0.42F, 0.55F + feedback * 0.40F);
        wet = ringmod(wet, amount * 0.42F, 0.08F + tone * 0.30F);
        wet = comb(wet, 0.30F + amount * 0.62F, 0.18F + tone * 0.52F, 0.70F + feedback * 0.27F);
        wet = lowpass(wet, 0.65F + amount * 0.30F, 0.16F + tone * 0.28F);
        return mix(input, wet, amount);
    }

    StereoFrame ritual_gate(StereoFrame input, float amount, float tone, float feedback) noexcept {
        const float env = envelope(tone, feedback);
        StereoFrame wet{input.left * env, input.right * env};
        wet = lowpass(wet, 0.35F + amount * 0.58F, 0.12F + env * (0.35F + tone * 0.45F));
        wet = drive(wet, amount * 0.35F * env);
        wet = delay(wet, amount * 0.56F, 0.18F + tone * 0.52F, 0.52F + feedback * 0.43F);
        return mix(input, wet, amount);
    }

    StereoFrame rust_cloud(StereoFrame input, float amount, float tone, float feedback) noexcept {
        StereoFrame wet = crusher(input, 0.25F + amount * 0.70F, 0.30F + tone * 0.68F);
        wet = wavefolder(wet, amount * 0.76F, 0.28F + feedback * 0.68F);
        wet = flanger(wet, 0.30F + amount * 0.60F, tone, 0.35F + feedback * 0.60F);
        wet = comb(wet, amount * 0.55F, 0.10F + tone * 0.35F, 0.45F + feedback * 0.48F);
        return mix(input, wet, amount);
    }

    StereoFrame deep_sea(StereoFrame input, float amount, float tone, float feedback) noexcept {
        StereoFrame wet = lowpass(input, 0.70F + amount * 0.28F, 0.08F + tone * 0.32F);
        wet = chorus(wet, 0.28F + amount * 0.62F, 0.06F + tone * 0.25F, 0.55F + feedback * 0.40F);
        wet = tremolo(wet, amount * 0.42F, 0.03F + tone * 0.18F);
        wet = diffuser(wet, 0.28F + amount * 0.64F, 0.20F + tone * 0.45F, 0.58F + feedback * 0.37F);
        return mix(input, wet, amount);
    }

    StereoFrame granular_reverse(StereoFrame input, float amount, float tone, float feedback) noexcept {
        if (delay_.empty()) return input;
        const float grain_seconds = 0.035F + tone * tone * 0.42F;
        const float grain_frames = grain_seconds * sample_rate_;
        const float speed = 1.45F + tone * 1.85F;
        reverse_phase_a_ += speed / std::max(16.0F, grain_frames);
        reverse_phase_b_ += speed / std::max(16.0F, grain_frames);
        if (reverse_phase_a_ >= 1.0F) {
            reverse_phase_a_ -= 1.0F;
            reverse_offset_a_ = 0.08F + (next_noise(reverse_random_state_) * 0.5F + 0.5F) * (0.25F + tone * 1.35F);
        }
        if (reverse_phase_b_ >= 1.0F) {
            reverse_phase_b_ -= 1.0F;
            reverse_offset_b_ = 0.12F + (next_noise(reverse_random_state_) * 0.5F + 0.5F) * (0.30F + tone * 1.60F);
        }
        const float window_a = clamp01(0.5F - 0.5F * fast_sine_lfo(reverse_phase_a_ + 0.25F));
        const float window_b = clamp01(0.5F - 0.5F * fast_sine_lfo(reverse_phase_b_ + 0.25F));
        const float read_a = reverse_offset_a_ * sample_rate_ + reverse_phase_a_ * grain_frames * 2.0F;
        const float read_b = reverse_offset_b_ * sample_rate_ + reverse_phase_b_ * grain_frames * 2.0F;
        const StereoFrame a = read_delay(delay_, delay_write_, read_a, delay_valid_);
        const StereoFrame b = read_delay(delay_, delay_write_, read_b, delay_valid_);
        const float normalization = 1.0F / std::max(0.22F, window_a + window_b);
        const StereoFrame wet{
            (a.left * window_a + b.right * window_b) * normalization,
            (a.right * window_a + b.left * window_b) * normalization,
        };
        const float regeneration = std::min(0.93F, feedback * 0.93F);
        delay_[delay_write_] = {
            input.left + wet.right * regeneration,
            input.right + wet.left * regeneration,
        };
        delay_write_ = (delay_write_ + 1U) % delay_.size();
        delay_valid_ = std::min(delay_valid_ + 1U, delay_.size());
        return mix(input, wet, amount);
    }

    float sample_rate_{48'000.0F};
    StereoFrame lowpass_{};
    StereoFrame highpass_low_{};
    StereoFrame diffuser_damp_{};
    StereoFrame phaser_feedback_{};
    float tremolo_phase_{0.0F};
    float ring_phase_{0.0F};
    float chorus_phase_{0.0F};
    float flanger_phase_{0.0F};
    float phaser_phase_{0.0F};
    float envelope_phase_{0.0F};
    float reverse_phase_a_{0.0F};
    float reverse_phase_b_{0.5F};
    float reverse_offset_a_{0.25F};
    float reverse_offset_b_{0.58F};
    std::uint32_t reverse_random_state_{0x6A09'E667U};
    std::vector<StereoFrame> delay_{};
    std::vector<StereoFrame> short_delay_{};
    std::size_t delay_write_{0U};
    std::size_t short_write_{0U};
    std::size_t delay_valid_{0U};
    std::size_t short_valid_{0U};
    unsigned drive_control_counter_{0U};
    unsigned lowpass_control_counter_{0U};
    unsigned highpass_control_counter_{0U};
    unsigned delay_control_counter_{0U};
    unsigned ring_control_counter_{0U};
    unsigned comb_control_counter_{0U};
    unsigned phaser_control_counter_{0U};
    float drive_normalizer_{1.0F};
    float lowpass_coefficient_{0.1F};
    float highpass_coefficient_{0.1F};
    float delay_frames_{1.0F};
    float ring_rate_{1.0F};
    float comb_frames_{1.0F};
    float phaser_coefficient_{0.0F};
    unsigned crusher_counter_{0U};
    StereoFrame crusher_held_{};
    std::array<float, 4> phaser_left_{};
    std::array<float, 4> phaser_right_{};
};

struct SlotRuntime {
    ProductEngine engine{};
    std::array<EffectRuntime, kEffectsPerSlot> effects{};
    std::array<ModulatorRuntime, kModulatorsPerSlot> modulators{};
    EuclideanRuntime euclidean{};
    SmoothedValue frequency{};
    SmoothedValue timbre{};
    SmoothedValue color{};
    SmoothedValue motion{};
    SmoothedValue texture{};
    SmoothedValue event_density{};
    SmoothedValue level{};
    SmoothedValue pan{};
    std::array<SmoothedValue, kEffectsPerSlot> effect_amount{};
    std::array<SmoothedValue, kEffectsPerSlot> effect_tone{};
    std::array<SmoothedValue, kEffectsPerSlot> effect_feedback{};

    void prepare(float sample_rate, const SlotSettings& settings, std::size_t slot_index) {
        engine.prepare(sample_rate, slot_index);
        frequency.prepare(settings.frequency_hz, sample_rate);
        timbre.prepare(settings.timbre, sample_rate);
        color.prepare(settings.color, sample_rate);
        motion.prepare(settings.motion, sample_rate);
        texture.prepare(settings.texture, sample_rate);
        event_density.prepare(settings.event_density, sample_rate);
        level.prepare(settings.level, sample_rate);
        pan.prepare(settings.pan, sample_rate);
        for (std::size_t index = 0; index < kEffectsPerSlot; ++index) {
            effects[index].prepare(sample_rate);
            effect_amount[index].prepare(settings.effects[index].amount, sample_rate);
            effect_tone[index].prepare(settings.effects[index].tone, sample_rate);
            effect_feedback[index].prepare(settings.effects[index].feedback, sample_rate);
        }
    }

    void update_targets(const SlotSettings& settings) noexcept {
        frequency.set(settings.frequency_hz);
        timbre.set(settings.timbre);
        color.set(settings.color);
        motion.set(settings.motion);
        texture.set(settings.texture);
        event_density.set(settings.event_density);
        level.set(settings.level);
        pan.set(settings.pan);
        for (std::size_t index = 0; index < kEffectsPerSlot; ++index) {
            effect_amount[index].set(settings.effects[index].amount);
            effect_tone[index].set(settings.effects[index].tone);
            effect_feedback[index].set(settings.effects[index].feedback);
        }
    }

    void reset() noexcept {
        engine.reset();
        for (auto& effect : effects) {
            effect.reset();
        }
        for (auto& modulator : modulators) {
            modulator.reset();
        }
        euclidean.reset();
    }
};

struct ModulatedParameters {
    float frequency{55.0F};
    float timbre{0.5F};
    float color{0.5F};
    float motion{0.25F};
    float texture{0.25F};
    float event_density{0.50F};
    float level{0.35F};
    float pan{0.0F};
    std::array<float, kEffectsPerSlot> effect_amount{};
    std::array<float, kEffectsPerSlot> effect_tone{};
    std::array<float, kEffectsPerSlot> effect_feedback{};
};

void apply_modulation(ModulatedParameters& parameters, ModDestination destination, float value) noexcept {
    switch (destination) {
    case ModDestination::pitch:
        parameters.frequency *= std::pow(2.0F, value * 2.0F);
        break;
    case ModDestination::timbre:
        parameters.timbre += value;
        break;
    case ModDestination::color:
        parameters.color += value;
        break;
    case ModDestination::motion:
        parameters.motion += value;
        break;
    case ModDestination::texture:
        parameters.texture += value;
        break;
    case ModDestination::level:
        parameters.level += value;
        break;
    case ModDestination::pan:
        parameters.pan += value;
        break;
    case ModDestination::fx1:
    case ModDestination::fx2:
    case ModDestination::fx3:
    case ModDestination::fx4: {
        const auto index = static_cast<std::size_t>(destination) - static_cast<std::size_t>(ModDestination::fx1);
        parameters.effect_amount[index] += value;
        break;
    }
    }
}

} // namespace

class AudioGraph::Impl {
public:
    void prepare(const AudioConfig& config, const Session& initial_session) {
        config_ = config;
        session_ = initial_session;
        chaos_slew_ = 1.0F - std::exp(-1.0F / (config_.sample_rate * 0.075F));
        event_indicator_decay_ = std::exp(-1.0F / (config_.sample_rate * 0.18F));
        // Preserve the sub-drone fundamentals: the old fixed coefficient placed
        // the DC-blocker corner near 38 Hz at 48 kHz. A sample-rate-derived
        // 12 Hz corner removes DC without hollowing out the intended low end.
        dc_coefficient_ = std::exp(-kTwoPi * 12.0F / config_.sample_rate);
        for (std::size_t index = 0; index < pan_left_.size(); ++index) {
            const float normalized = static_cast<float>(index) /
                static_cast<float>(pan_left_.size() - 1U);
            const float pan = normalized * 2.0F - 1.0F;
            pan_left_[index] = std::sqrt(0.5F * (1.0F - pan));
            pan_right_[index] = std::sqrt(0.5F * (1.0F + pan));
        }
        rebuild_effective_slots();
        master_.prepare(session_.master_level * session_.performance.fade, config_.sample_rate, 100.0F);
        performance_texture_.prepare(session_.performance.texture, config_.sample_rate, 80.0F);
        performance_pulse_.prepare(session_.performance.pulse, config_.sample_rate, 80.0F);
        performance_chaos_.prepare(session_.performance.chaos, config_.sample_rate, 120.0F);
        performance_space_.prepare(session_.performance.space, config_.sample_rate, 100.0F);
        performance_events_.prepare(session_.performance.events, config_.sample_rate, 100.0F);
        for (std::size_t index = 0; index < kSlotCount; ++index) {
            slots_[index].prepare(config_.sample_rate, effective_slots_[index], index);
        }
        for (auto& effect : master_effects_) effect.prepare(config_.sample_rate);
        reset();
        prepared_ = true;
    }

    void reset() noexcept {
        dc_input_ = {};
        dc_output_ = {};
        pulse_phase_ = 0.0F;
        chaos_phase_ = 0.0F;
        scope_publish_counter_ = 0U;
        pulse_phases_.fill(0.0F);
        chaos_current_.fill(0.0F);
        chaos_target_.fill(0.0F);
        slot_event_hold_.fill(0.0F);
        manual_trigger_mask_.store(0U, std::memory_order_relaxed);
        master_.snap();
        performance_texture_.snap();
        performance_pulse_.snap();
        performance_chaos_.snap();
        performance_space_.snap();
        performance_events_.snap();
        for (auto& slot : slots_) slot.reset();
        for (auto& effect : master_effects_) effect.reset();
        clear_telemetry();
    }

    bool submit_session(const Session& session) noexcept { return sessions_.push(session); }

    void trigger_slot(std::size_t slot_index) noexcept {
        if (slot_index >= kSlotCount) return;
        manual_trigger_mask_.fetch_or(std::uint32_t{1} << slot_index, std::memory_order_release);
    }

    void panic() noexcept { panic_requested_.store(true, std::memory_order_release); }

    AudioTelemetry telemetry() const noexcept {
        AudioTelemetry result{};
        for (std::size_t index = 0; index < kSlotCount; ++index) {
            result.slot_rms[index] = slot_rms_[index].load(std::memory_order_relaxed);
            result.slot_peak[index] = slot_peak_[index].load(std::memory_order_relaxed);
            result.slot_event[index] = slot_event_[index].load(std::memory_order_relaxed);
            for (std::size_t point = 0; point < kScopePointCount; ++point) {
                result.slot_scope[index][point] =
                    slot_scope_[index][point].load(std::memory_order_relaxed);
            }
        }
        for (std::size_t point = 0; point < kScopePointCount; ++point) {
            result.master_scope[point] = master_scope_[point].load(std::memory_order_relaxed);
        }
        result.master_rms = master_rms_.load(std::memory_order_relaxed);
        result.master_peak = master_peak_.load(std::memory_order_relaxed);
        result.pulse_phase = pulse_meter_.load(std::memory_order_relaxed);
        result.chaos_activity = chaos_meter_.load(std::memory_order_relaxed);
        return result;
    }

    void process(BufferView<StereoFrame> output) noexcept {
        std::fill(output.begin(), output.end(), StereoFrame{});
        if (!prepared_ || output.empty()) {
            return;
        }

        Session incoming{};
        bool received = false;
        while (sessions_.pop(incoming)) {
            received = true;
        }
        if (received) {
            std::array<bool, kSlotCount> was_enabled{};
            for (std::size_t index = 0; index < kSlotCount; ++index) {
                was_enabled[index] = effective_slots_[index].enabled;
            }
            session_ = incoming;
            master_.set(session_.master_level * clamp01(session_.performance.fade));
            performance_texture_.set(clamp01(session_.performance.texture));
            performance_pulse_.set(clamp01(session_.performance.pulse));
            performance_chaos_.set(clamp01(session_.performance.chaos));
            performance_space_.set(clamp01(session_.performance.space));
            performance_events_.set(clamp01(session_.performance.events));
            rebuild_effective_slots();
            bool any_actor_enabled = false;
            for (std::size_t index = 0; index < kSlotCount; ++index) {
                slots_[index].update_targets(effective_slots_[index]);
                any_actor_enabled = any_actor_enabled || effective_slots_[index].enabled;
                // Explicit mute is a lifecycle boundary: invalidate the actor and
                // its private FX tails once, then keep the dormant slot entirely
                // out of the sample loop until it is enabled again.
                if (was_enabled[index] != effective_slots_[index].enabled) {
                    slots_[index].reset();
                    slot_event_hold_[index] = 0.0F;
                }
            }
            if (!any_actor_enabled) {
                // Do not let the DC blocker itself imitate a source tail after the
                // last actor is muted. Master FX still process their own memory.
                dc_input_ = {};
                dc_output_ = {};
            }
        }

        if (panic_requested_.exchange(false, std::memory_order_acq_rel)) {
            reset();
            return;
        }

        const std::uint32_t manual_trigger_mask =
            manual_trigger_mask_.exchange(0U, std::memory_order_acq_rel);

        std::array<double, kSlotCount> slot_energy{};
        std::array<float, kSlotCount> slot_peak{};
        double master_energy = 0.0;
        float master_peak = 0.0F;
        float last_chaos_activity = 0.0F;
        std::array<std::array<float, kScopePointCount>, kSlotCount> scope_frames{};
        std::array<float, kScopePointCount> master_scope_frame{};
        const bool capture_scope = ++scope_publish_counter_ >= 4U;
        if (capture_scope) scope_publish_counter_ = 0U;

        for (std::size_t frame_index = 0; frame_index < output.size(); ++frame_index) {
            for (auto& hold : slot_event_hold_) hold *= event_indicator_decay_;
            auto& frame = output[frame_index];
            const std::size_t scope_index = capture_scope
                ? std::min(kScopePointCount - 1U, frame_index * kScopePointCount / output.size())
                : 0U;
            const float texture_macro = macro_curve(performance_texture_.next());
            const float pulse_macro = macro_curve(performance_pulse_.next());
            const float chaos_macro = macro_curve(performance_chaos_.next());
            const float space_macro = macro_curve(performance_space_.next());
            const float events_macro = macro_curve(performance_events_.next());

            const float pulse_rate = std::clamp(session_.tempo_bpm, 10.0F, 300.0F) / 60.0F *
                (0.5F + pulse_macro * 3.5F);
            pulse_phase_ += pulse_rate / config_.sample_rate;
            pulse_phase_ -= std::floor(pulse_phase_);

            chaos_phase_ += (0.06F + chaos_macro * chaos_macro * 3.5F) / config_.sample_rate;
            if (chaos_phase_ >= 1.0F) {
                chaos_phase_ -= 1.0F;
                for (auto& target : chaos_target_) {
                    target = next_noise(chaos_random_state_);
                }
            }
            last_chaos_activity = 0.0F;
            for (std::size_t index = 0; index < kSlotCount; ++index) {
                chaos_current_[index] += (chaos_target_[index] - chaos_current_[index]) * chaos_slew_;
                last_chaos_activity = std::max(last_chaos_activity, std::abs(chaos_current_[index]) * chaos_macro);
            }

            StereoFrame mix{};
            for (std::size_t slot_index = 0; slot_index < kSlotCount; ++slot_index) {
                const auto& settings = effective_slots_[slot_index];
                auto& runtime = slots_[slot_index];
                if (!settings.enabled) {
                    if (capture_scope) scope_frames[slot_index][scope_index] = 0.0F;
                    continue;
                }
                ModulatedParameters parameters{
                    runtime.frequency.next(),
                    runtime.timbre.next(),
                    runtime.color.next(),
                    runtime.motion.next(),
                    runtime.texture.next(),
                    runtime.event_density.next(),
                    runtime.level.next(),
                    runtime.pan.next(),
                    {},
                    {},
                    {},
                };
                for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {
                    parameters.effect_amount[effect_index] = runtime.effect_amount[effect_index].next();
                    parameters.effect_tone[effect_index] = runtime.effect_tone[effect_index].next();
                    parameters.effect_feedback[effect_index] = runtime.effect_feedback[effect_index].next();
                }
                std::array<float, kModulatorsPerSlot> modulation_values{};
                for (std::size_t mod_index = 0; mod_index < kModulatorsPerSlot; ++mod_index) {
                    const auto& mod = settings.modulators[mod_index];
                    float rate_modulation = 0.0F;
                    if (mod.rate_mod_source >= 0 &&
                        mod.rate_mod_source < static_cast<int>(mod_index)) {
                        rate_modulation = modulation_values[static_cast<std::size_t>(mod.rate_mod_source)] *
                            mod.rate_mod_amount;
                    }
                    modulation_values[mod_index] = runtime.modulators[mod_index].next(
                        mod, config_.sample_rate, rate_modulation);
                    apply_modulation(parameters, mod.destination, modulation_values[mod_index]);
                }
                const bool euclidean_trigger = runtime.euclidean.next(
                    settings.euclidean, session_.tempo_bpm, config_.sample_rate);
                const bool manual_trigger = frame_index == 0U &&
                    (manual_trigger_mask & (std::uint32_t{1} << slot_index)) != 0U;

                const float chaos_value = chaos_current_[slot_index] * chaos_macro;
                parameters.frequency *= std::pow(2.0F, chaos_value * 0.36F);
                switch (settings.engine) {
                case EngineKind::plaits:
                    parameters.timbre += texture_macro * 0.22F;
                    parameters.color += chaos_macro * 0.16F;
                    parameters.motion += events_macro * 0.24F;
                    parameters.texture += space_macro * 0.18F;
                    break;
                case EngineKind::diagnostic:
                    parameters.timbre += texture_macro * 0.24F;
                    parameters.motion += texture_macro * 0.30F;
                    parameters.texture += texture_macro * 0.68F;
                    break;
                case EngineKind::macro:
                    parameters.timbre += texture_macro * 0.34F;
                    parameters.color += texture_macro * 0.12F;
                    parameters.motion += chaos_macro * 0.18F;
                    parameters.texture += texture_macro * 0.48F;
                    break;
                case EngineKind::body:
                    parameters.timbre += texture_macro * 0.18F;
                    parameters.color += texture_macro * 0.48F;
                    parameters.motion += events_macro * 0.42F;
                    parameters.texture += texture_macro * 0.36F + events_macro * 0.12F;
                    break;
                case EngineKind::grain:
                    parameters.timbre += texture_macro * 0.46F;
                    parameters.color += chaos_macro * 0.15F;
                    parameters.motion += events_macro * 0.38F + chaos_macro * 0.16F;
                    parameters.texture += texture_macro * 0.76F;
                    break;
                case EngineKind::particle:
                    parameters.timbre += texture_macro * 0.20F;
                    parameters.color += chaos_macro * 0.36F;
                    parameters.motion += events_macro * 0.58F + chaos_macro * 0.24F;
                    parameters.texture += texture_macro * 0.62F + events_macro * 0.30F;
                    break;
                case EngineKind::derelict_bed:
                case EngineKind::motor:
                case EngineKind::wind:
                    parameters.timbre += texture_macro * 0.18F;
                    parameters.color += texture_macro * 0.24F;
                    parameters.motion += events_macro * 0.25F + chaos_macro * 0.18F;
                    parameters.texture += texture_macro * 0.46F;
                    break;
                case EngineKind::sub_drone:
                case EngineKind::tape_drone:
                case EngineKind::bowed_metal:
                case EngineKind::earth_rumble:
                    parameters.timbre += texture_macro * 0.12F;
                    parameters.color += texture_macro * 0.10F;
                    parameters.motion += events_macro * 0.12F + chaos_macro * 0.08F;
                    parameters.texture += texture_macro * 0.30F;
                    break;
                case EngineKind::footsteps:
                case EngineKind::machinery:
                case EngineKind::birds:
                case EngineKind::insects:
                case EngineKind::signal:
                case EngineKind::water_drip:
                case EngineKind::water_flow:
                case EngineKind::stone:
                case EngineKind::rail_joint:
                case EngineKind::brake:
                case EngineKind::music_box:
                case EngineKind::toy_voice:
                case EngineKind::lullaby:
                    parameters.color += texture_macro * 0.18F;
                    parameters.motion += pulse_macro * 0.38F + events_macro * 0.22F;
                    parameters.texture += texture_macro * 0.58F + chaos_macro * 0.12F;
                    break;
                case EngineKind::door:
                case EngineKind::pipe:
                case EngineKind::metal:
                case EngineKind::crowd:
                case EngineKind::cave_air:
                case EngineKind::metro_traction:
                case EngineKind::carriage:
                case EngineKind::toy_gears:
                    parameters.timbre += texture_macro * 0.22F;
                    parameters.color += chaos_macro * 0.22F;
                    parameters.motion += events_macro * 0.30F;
                    parameters.texture += texture_macro * 0.64F + chaos_macro * 0.18F;
                    break;
                }
                parameters.motion += std::abs(chaos_value) * 0.28F;
                parameters.texture += std::abs(chaos_value) * 0.32F;
                parameters.pan += chaos_value * 0.38F;

                float pulse_ratio = 1.0F;
                float pulse_depth = 0.72F;
                switch (settings.engine) {
                case EngineKind::macro: pulse_ratio = 0.5F; pulse_depth = 0.38F; break;
                case EngineKind::body: pulse_ratio = 1.0F; pulse_depth = 0.22F; break;
                case EngineKind::grain: pulse_ratio = 2.0F; pulse_depth = 0.88F; break;
                case EngineKind::particle: pulse_ratio = 0.75F; pulse_depth = 0.58F; break;
                case EngineKind::derelict_bed: pulse_ratio = 0.25F; pulse_depth = 0.08F; break;
                case EngineKind::footsteps: pulse_ratio = 1.0F; pulse_depth = 0.0F; break;
                case EngineKind::door: pulse_ratio = 0.37F; pulse_depth = 0.0F; break;
                case EngineKind::pipe: pulse_ratio = 0.31F; pulse_depth = 0.06F; break;
                case EngineKind::motor: pulse_ratio = 0.5F; pulse_depth = 0.09F; break;
                case EngineKind::machinery: pulse_ratio = 1.0F; pulse_depth = 0.0F; break;
                case EngineKind::crowd: pulse_ratio = 0.2F; pulse_depth = 0.05F; break;
                case EngineKind::metal: pulse_ratio = 0.43F; pulse_depth = 0.0F; break;
                case EngineKind::wind: pulse_ratio = 0.17F; pulse_depth = 0.04F; break;
                case EngineKind::birds: pulse_ratio = 0.67F; pulse_depth = 0.0F; break;
                case EngineKind::insects: pulse_ratio = 2.0F; pulse_depth = 0.06F; break;
                case EngineKind::signal: pulse_ratio = 1.0F; pulse_depth = 0.0F; break;
                case EngineKind::cave_air: pulse_ratio = 0.13F; pulse_depth = 0.03F; break;
                case EngineKind::water_drip: pulse_ratio = 0.71F; pulse_depth = 0.0F; break;
                case EngineKind::water_flow: pulse_ratio = 0.29F; pulse_depth = 0.04F; break;
                case EngineKind::stone: pulse_ratio = 0.37F; pulse_depth = 0.0F; break;
                case EngineKind::metro_traction: pulse_ratio = 0.25F; pulse_depth = 0.05F; break;
                case EngineKind::rail_joint: pulse_ratio = 1.0F; pulse_depth = 0.0F; break;
                case EngineKind::brake: pulse_ratio = 0.41F; pulse_depth = 0.0F; break;
                case EngineKind::carriage: pulse_ratio = 0.5F; pulse_depth = 0.04F; break;
                case EngineKind::music_box: pulse_ratio = 0.5F; pulse_depth = 0.0F; break;
                case EngineKind::toy_voice: pulse_ratio = 0.67F; pulse_depth = 0.06F; break;
                case EngineKind::toy_gears: pulse_ratio = 1.0F; pulse_depth = 0.08F; break;
                case EngineKind::lullaby: pulse_ratio = 0.25F; pulse_depth = 0.0F; break;
                case EngineKind::sub_drone: pulse_ratio = 0.125F; pulse_depth = 0.025F; break;
                case EngineKind::tape_drone: pulse_ratio = 0.17F; pulse_depth = 0.035F; break;
                case EngineKind::bowed_metal: pulse_ratio = 0.21F; pulse_depth = 0.04F; break;
                case EngineKind::earth_rumble: pulse_ratio = 0.10F; pulse_depth = 0.02F; break;
                case EngineKind::plaits: pulse_ratio = 1.0F; pulse_depth = 0.14F; break;
                case EngineKind::diagnostic: break;
                }
                pulse_phases_[slot_index] += pulse_rate * pulse_ratio / config_.sample_rate;
                pulse_phases_[slot_index] -= std::floor(pulse_phases_[slot_index]);
                float slot_pulse_phase = pulse_phases_[slot_index] + chaos_value * 0.08F;
                slot_pulse_phase -= std::floor(slot_pulse_phase);
                const float pulse_wave = clamp01(
                    0.5F + 0.5F * fast_sine_lfo(slot_pulse_phase - 0.25F));
                const float pulse_envelope = std::pow(pulse_wave, 1.0F + pulse_macro * 10.0F);
                const float pulse_gain = 1.0F - pulse_macro * pulse_depth * (1.0F - pulse_envelope);
                parameters.level *= pulse_gain * std::clamp(1.0F + chaos_value * 0.72F, 0.16F, 1.65F);

                for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {
                    const auto effect_kind = settings.effects[effect_index].kind;
                    if (!settings.effects[effect_index].enabled ||
                        effect_kind == EffectKind::bypass) continue;
                    switch (effect_kind) {
                    case EffectKind::drive:
                    case EffectKind::crusher:
                    case EffectKind::wavefolder:
                        parameters.effect_amount[effect_index] += texture_macro * 0.42F;
                        break;
                    case EffectKind::lowpass:
                    case EffectKind::highpass:
                        parameters.effect_tone[effect_index] += texture_macro * 0.16F;
                        break;
                    case EffectKind::tremolo:
                        parameters.effect_amount[effect_index] += pulse_macro * 0.20F;
                        break;
                    case EffectKind::delay:
                    case EffectKind::comb:
                        parameters.effect_amount[effect_index] += space_macro * 0.58F;
                        parameters.effect_feedback[effect_index] += space_macro * 0.46F;
                        parameters.effect_tone[effect_index] += space_macro * 0.13F;
                        break;
                    case EffectKind::ringmod:
                        parameters.effect_amount[effect_index] += chaos_macro * 0.34F;
                        parameters.effect_tone[effect_index] += pulse_macro * 0.12F;
                        break;
                    case EffectKind::granular_reverse:
                        parameters.effect_amount[effect_index] += space_macro * 0.74F;
                        parameters.effect_tone[effect_index] += chaos_macro * 0.18F;
                        parameters.effect_feedback[effect_index] += space_macro * 0.40F;
                        break;
                    default:
                        break;
                    }
                    parameters.effect_amount[effect_index] = clamp01(parameters.effect_amount[effect_index]);
                    parameters.effect_tone[effect_index] = clamp01(parameters.effect_tone[effect_index]);
                    parameters.effect_feedback[effect_index] = clamp01(parameters.effect_feedback[effect_index]);
                }

                parameters.timbre = clamp01(parameters.timbre);
                parameters.color = clamp01(parameters.color);
                parameters.motion = clamp01(parameters.motion);
                parameters.texture = clamp01(parameters.texture);
                parameters.event_density = clamp01(parameters.event_density);
                parameters.level = std::clamp(parameters.level, 0.0F, 1.5F);
                parameters.pan = std::clamp(parameters.pan, -1.0F, 1.0F);

                const StereoFrame actor_frame = runtime.engine.next(
                    settings,
                    parameters.frequency,
                    parameters.timbre,
                    parameters.color,
                    parameters.motion,
                    parameters.texture,
                    session_.tempo_bpm,
                    pulse_macro,
                    chaos_macro,
                    effective_event_density(parameters.event_density, events_macro),
                    euclidean_trigger || manual_trigger,
                    manual_trigger);
                if (runtime.engine.consume_event_fired()) {
                    slot_event_hold_[slot_index] = 1.0F;
                }
                const auto [left_pan, right_pan] = pan_gains(parameters.pan);
                StereoFrame slot_frame{
                    actor_frame.left * parameters.level * left_pan,
                    actor_frame.right * parameters.level * right_pan,
                };

                for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {
                    const auto kind = settings.effects[effect_index].kind;
                    if (!settings.effects[effect_index].enabled ||
                        kind == EffectKind::bypass) continue;
                    slot_frame = runtime.effects[effect_index].process(
                        slot_frame,
                        kind,
                        parameters.effect_amount[effect_index],
                        parameters.effect_tone[effect_index],
                        parameters.effect_feedback[effect_index]);
                }
                if (capture_scope) {
                    scope_frames[slot_index][scope_index] = 0.5F * (slot_frame.left + slot_frame.right);
                }
                mix.left += slot_frame.left;
                mix.right += slot_frame.right;
                const float slot_power = 0.5F * (slot_frame.left * slot_frame.left + slot_frame.right * slot_frame.right);
                slot_energy[slot_index] += static_cast<double>(slot_power);
                slot_peak[slot_index] = std::max(
                    slot_peak[slot_index], std::max(std::abs(slot_frame.left), std::abs(slot_frame.right)));
            }

            for (std::size_t effect_index = 0; effect_index < kMasterEffects; ++effect_index) {
                const auto& settings = session_.master_effects[effect_index];
                if (!settings.enabled || settings.kind == EffectKind::bypass) continue;
                mix = master_effects_[effect_index].process(
                    mix, settings.kind, settings.amount, settings.tone, settings.feedback);
            }

            const StereoFrame dc_removed{
                mix.left - dc_input_.left + dc_coefficient_ * dc_output_.left,
                mix.right - dc_input_.right + dc_coefficient_ * dc_output_.right,
            };
            dc_input_ = mix;
            dc_output_ = dc_removed;

            constexpr float limiter_drive = 1.85F;
            const float master = master_.next();
            frame = {
                std::tanh(dc_removed.left * limiter_drive) * master,
                std::tanh(dc_removed.right * limiter_drive) * master,
            };
            if (capture_scope) master_scope_frame[scope_index] = 0.5F * (frame.left + frame.right);
            const float master_power = 0.5F * (frame.left * frame.left + frame.right * frame.right);
            master_energy += static_cast<double>(master_power);
            master_peak = std::max(master_peak, std::max(std::abs(frame.left), std::abs(frame.right)));
        }

        const float frame_count = static_cast<float>(output.size());
        for (std::size_t index = 0; index < kSlotCount; ++index) {
            publish_meter(slot_rms_[index], slot_rms_smooth_[index],
                std::sqrt(static_cast<float>(slot_energy[index]) / frame_count), 0.18F);
            publish_peak(slot_peak_[index], slot_peak_smooth_[index], slot_peak[index]);
            slot_event_[index].store(slot_event_hold_[index], std::memory_order_relaxed);
            if (capture_scope) {
                for (std::size_t point = 0; point < kScopePointCount; ++point) {
                    slot_scope_[index][point].store(scope_frames[index][point], std::memory_order_relaxed);
                }
            }
        }
        if (capture_scope) {
            for (std::size_t point = 0; point < kScopePointCount; ++point) {
                master_scope_[point].store(master_scope_frame[point], std::memory_order_relaxed);
            }
        }
        publish_meter(master_rms_, master_rms_smooth_,
            std::sqrt(static_cast<float>(master_energy) / frame_count), 0.20F);
        publish_peak(master_peak_, master_peak_smooth_, master_peak);
        pulse_meter_.store(pulse_phase_, std::memory_order_relaxed);
        chaos_meter_.store(last_chaos_activity, std::memory_order_relaxed);
    }

    float sample_rate() const noexcept { return config_.sample_rate; }

private:
    static float macro_curve(float value) noexcept {
        value = clamp01(value);
        return value * value * (3.0F - 2.0F * value);
    }

    void rebuild_effective_slots() noexcept {
        Session target = session_;
        apply_scene_recipe(target, session_.performance.morph_target);
        const float morph = clamp01(session_.performance.morph);
        const auto blend = [morph](float a, float b) { return a + (b - a) * morph; };
        for (std::size_t index = 0; index < kSlotCount; ++index) {
            const auto& source = session_.slots[index];
            const auto& destination = target.slots[index];
            auto& result = effective_slots_[index];
            result = source;
            // Actor mute is authoritative. Morph may change the actor recipe, but it
            // must never re-enable an actor that the performer muted.
            result.enabled = source.enabled;
            if (morph >= 0.5F) {
                result.engine = destination.engine;
                result.plaits_model = destination.plaits_model;
                result.plaits_output = destination.plaits_output;
                result.tuning = destination.tuning;
                result.effects = destination.effects;
                result.modulators = destination.modulators;
                result.euclidean = destination.euclidean;
            }
            result.frequency_hz = std::exp(blend(
                std::log(std::max(4.0F, source.frequency_hz)),
                std::log(std::max(4.0F, destination.frequency_hz))));
            result.timbre = blend(source.timbre, destination.timbre);
            result.color = blend(source.color, destination.color);
            result.motion = blend(source.motion, destination.motion);
            result.texture = blend(source.texture, destination.texture);
            result.event_density = blend(source.event_density, destination.event_density);
            result.level = blend(source.level, destination.level);
            result.pan = blend(source.pan, destination.pan);
        }
    }

    std::pair<float, float> pan_gains(float pan) const noexcept {
        constexpr float scale = 0.5F * static_cast<float>(1024U);
        const float position = (std::clamp(pan, -1.0F, 1.0F) + 1.0F) * scale;
        const std::size_t lower = static_cast<std::size_t>(position);
        const std::size_t upper = std::min(lower + 1U, pan_left_.size() - 1U);
        const float fraction = position - static_cast<float>(lower);
        return {
            pan_left_[lower] + (pan_left_[upper] - pan_left_[lower]) * fraction,
            pan_right_[lower] + (pan_right_[upper] - pan_right_[lower]) * fraction,
        };
    }

    static void publish_meter(std::atomic<float>& destination, float& smoothed, float value, float mix) noexcept {
        smoothed += (value - smoothed) * mix;
        destination.store(smoothed, std::memory_order_relaxed);
    }

    static void publish_peak(std::atomic<float>& destination, float& smoothed, float value) noexcept {
        smoothed = std::max(value, smoothed * 0.91F);
        destination.store(smoothed, std::memory_order_relaxed);
    }

    void clear_telemetry() noexcept {
        slot_rms_smooth_.fill(0.0F);
        slot_peak_smooth_.fill(0.0F);
        for (std::size_t index = 0; index < kSlotCount; ++index) {
            slot_rms_[index].store(0.0F, std::memory_order_relaxed);
            slot_peak_[index].store(0.0F, std::memory_order_relaxed);
            slot_event_[index].store(0.0F, std::memory_order_relaxed);
            for (auto& point : slot_scope_[index]) {
                point.store(0.0F, std::memory_order_relaxed);
            }
        }
        for (auto& point : master_scope_) {
            point.store(0.0F, std::memory_order_relaxed);
        }
        master_rms_smooth_ = 0.0F;
        master_peak_smooth_ = 0.0F;
        master_rms_.store(0.0F, std::memory_order_relaxed);
        master_peak_.store(0.0F, std::memory_order_relaxed);
        pulse_meter_.store(0.0F, std::memory_order_relaxed);
        chaos_meter_.store(0.0F, std::memory_order_relaxed);
    }

    AudioConfig config_{};
    Session session_{};
    std::array<SlotSettings, kSlotCount> effective_slots_{};
    std::array<SlotRuntime, kSlotCount> slots_{};
    std::array<EffectRuntime, kMasterEffects> master_effects_{};
    SpscQueue<Session, 32> sessions_{};
    SmoothedValue master_{};
    SmoothedValue performance_texture_{};
    SmoothedValue performance_pulse_{};
    SmoothedValue performance_chaos_{};
    SmoothedValue performance_space_{};
    SmoothedValue performance_events_{};
    StereoFrame dc_input_{};
    StereoFrame dc_output_{};
    float dc_coefficient_{0.998F};
    std::array<float, 1025> pan_left_{};
    std::array<float, 1025> pan_right_{};
    std::array<float, kSlotCount> chaos_current_{};
    std::array<float, kSlotCount> chaos_target_{};
    std::array<float, kSlotCount> pulse_phases_{};
    float pulse_phase_{0.0F};
    float chaos_phase_{0.0F};
    float chaos_slew_{0.0F};
    float event_indicator_decay_{1.0F};
    unsigned scope_publish_counter_{0U};
    std::uint32_t chaos_random_state_{0xD00D'F00DU};
    std::array<std::atomic<float>, kSlotCount> slot_rms_{};
    std::array<std::atomic<float>, kSlotCount> slot_peak_{};
    std::array<std::atomic<float>, kSlotCount> slot_event_{};
    std::array<std::array<std::atomic<float>, kScopePointCount>, kSlotCount> slot_scope_{};
    std::array<std::atomic<float>, kScopePointCount> master_scope_{};
    std::atomic<float> master_rms_{0.0F};
    std::atomic<float> master_peak_{0.0F};
    std::atomic<float> pulse_meter_{0.0F};
    std::atomic<float> chaos_meter_{0.0F};
    std::array<float, kSlotCount> slot_rms_smooth_{};
    std::array<float, kSlotCount> slot_peak_smooth_{};
    std::array<float, kSlotCount> slot_event_hold_{};
    float master_rms_smooth_{0.0F};
    float master_peak_smooth_{0.0F};
    std::atomic<std::uint32_t> manual_trigger_mask_{0U};
    std::atomic<bool> panic_requested_{false};
    bool prepared_{false};
};

AudioGraph::AudioGraph() : impl_(std::make_unique<Impl>()) {}
AudioGraph::~AudioGraph() = default;
AudioGraph::AudioGraph(AudioGraph&&) noexcept = default;
AudioGraph& AudioGraph::operator=(AudioGraph&&) noexcept = default;

void AudioGraph::prepare(const AudioConfig& config, const Session& initial_session) {
    impl_->prepare(config, initial_session);
}

void AudioGraph::reset() noexcept { impl_->reset(); }
bool AudioGraph::submit_session(const Session& session) noexcept { return impl_->submit_session(session); }
void AudioGraph::process(BufferView<StereoFrame> output) noexcept { impl_->process(output); }
AudioTelemetry AudioGraph::telemetry() const noexcept { return impl_->telemetry(); }
void AudioGraph::trigger_slot(std::size_t slot_index) noexcept { impl_->trigger_slot(slot_index); }
void AudioGraph::panic() noexcept { impl_->panic(); }
float AudioGraph::sample_rate() const noexcept { return impl_->sample_rate(); }

} // namespace cursed_drone

#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text(encoding='utf-8')
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f'{path}: expected one match, found {count}: {old[:120]!r}')
    p.write_text(text.replace(old, new), encoding='utf-8')

path = 'src/audio.cpp'
replace_once(path,
'''#include "cursed_drone/audio.hpp"
#include "soundscape.hpp"
''',
'''#include "cursed_drone/audio.hpp"
#include "plaits_actor.hpp"
#include "soundscape.hpp"
''')

replace_once(path,
'''    float next(const ModSettings& settings, float sample_rate) noexcept {
        if (!settings.enabled) {
            return 0.0F;
        }

        float value = 0.0F;
        switch (settings.wave) {
        case ModWave::sine:
            value = std::sin(phase_ * kTwoPi);
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

        phase_ += std::clamp(settings.rate_hz, 0.001F, 40.0F) / sample_rate;
        if (phase_ >= 1.0F) {
            phase_ -= 1.0F;
            held_ = next_noise(random_state_);
            walk_ = std::clamp(walk_ + next_noise(random_state_) * 0.22F, -1.0F, 1.0F);
        }
        return settings.offset + value * settings.depth;
    }''',
'''    float next(const ModSettings& settings, float sample_rate, float rate_modulation = 0.0F) noexcept {
        if (!settings.enabled) {
            return 0.0F;
        }

        float value = 0.0F;
        switch (settings.wave) {
        case ModWave::sine:
            value = std::sin(phase_ * kTwoPi);
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
    }''')

replace_once(path,
'''class DiagnosticEngine {''',
'''class EuclideanRuntime {
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

class DiagnosticEngine {''')

replace_once(path,
'''        random_state_ = 0x91E1'0DA5U ^ (0x9E37'79B9U * static_cast<std::uint32_t>(slot_index + 1U));
        reset();''',
'''        random_state_ = 0x91E1'0DA5U ^ (0x9E37'79B9U * static_cast<std::uint32_t>(slot_index + 1U));
        plaits_.prepare(sample_rate_);
        reset();''')
replace_once(path,
'''        soundscape_.reset();
    }

    float next(
        EngineKind kind,''',
'''        soundscape_.reset();
        plaits_.reset();
    }

    float next_mono(
        EngineKind kind,''')
replace_once(path,
'''        float pulse,
        float chaos,
        float events) noexcept {''',
'''        float pulse,
        float chaos,
        float events,
        bool external_trigger) noexcept {''')
replace_once(path, '        bool trigger = false;', '        bool trigger = external_trigger;')
replace_once(path,
'''        switch (kind) {
        case EngineKind::diagnostic:''',
'''        switch (kind) {
        case EngineKind::plaits:
            return 0.0F;
        case EngineKind::diagnostic:''')
replace_once(path,
'''        return 0.0F;
    }

private:
    float sample_rate_{48'000.0F};''',
'''        return 0.0F;
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
        bool external_trigger) noexcept {
        if (settings.engine == EngineKind::plaits) {
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
                settings.euclidean.enabled);
        }
        const float mono = next_mono(
            settings.engine, frequency, timbre, color, motion, texture,
            tempo_bpm, pulse, chaos, events, external_trigger);
        return {mono, mono};
    }

private:
    float sample_rate_{48'000.0F};''')
replace_once(path,
'''    daisysp::ClockedNoise clocked_noise_{};
    detail::SoundscapeVoice soundscape_{};''',
'''    daisysp::ClockedNoise clocked_noise_{};
    detail::SoundscapeVoice soundscape_{};
    detail::PlaitsActor plaits_{};''')

replace_once(path,
'''        const auto delay_frames = static_cast<std::size_t>(std::ceil(sample_rate_ * 1.3F)) + 2U;''',
'''        const auto delay_frames = static_cast<std::size_t>(std::ceil(sample_rate_ * 2.4F)) + 2U;''')
replace_once(path,
'''        envelope_phase_ = 0.0F;
        delay_write_ = 0U;''',
'''        envelope_phase_ = 0.0F;
        reverse_phase_a_ = 0.0F;
        reverse_phase_b_ = 0.5F;
        reverse_offset_a_ = 0.25F;
        reverse_offset_b_ = 0.58F;
        delay_write_ = 0U;''')
replace_once(path,
'''        case EffectKind::deep_sea: return deep_sea(input, amount, tone, feedback);
        }''',
'''        case EffectKind::deep_sea: return deep_sea(input, amount, tone, feedback);
        case EffectKind::granular_reverse: return granular_reverse(input, amount, tone, feedback);
        }''')
replace_once(path,
'''    StereoFrame deep_sea(StereoFrame input, float amount, float tone, float feedback) noexcept {
        StereoFrame wet = lowpass(input, 0.70F + amount * 0.28F, 0.08F + tone * 0.32F);
        wet = chorus(wet, 0.28F + amount * 0.62F, 0.06F + tone * 0.25F, 0.55F + feedback * 0.40F);
        wet = tremolo(wet, amount * 0.42F, 0.03F + tone * 0.18F);
        wet = diffuser(wet, 0.28F + amount * 0.64F, 0.20F + tone * 0.45F, 0.58F + feedback * 0.37F);
        return mix(input, wet, amount);
    }

    float sample_rate_{48'000.0F};''',
'''    StereoFrame deep_sea(StereoFrame input, float amount, float tone, float feedback) noexcept {
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
        const float window_a = 0.5F - 0.5F * std::cos(reverse_phase_a_ * kTwoPi);
        const float window_b = 0.5F - 0.5F * std::cos(reverse_phase_b_ * kTwoPi);
        const float read_a = reverse_offset_a_ * sample_rate_ + reverse_phase_a_ * grain_frames * 2.0F;
        const float read_b = reverse_offset_b_ * sample_rate_ + reverse_phase_b_ * grain_frames * 2.0F;
        const StereoFrame a = read_delay(delay_, delay_write_, read_a);
        const StereoFrame b = read_delay(delay_, delay_write_, read_b);
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
        return mix(input, wet, amount);
    }

    float sample_rate_{48'000.0F};''')
replace_once(path,
'''    float envelope_phase_{0.0F};
    std::vector<StereoFrame> delay_{};''',
'''    float envelope_phase_{0.0F};
    float reverse_phase_a_{0.0F};
    float reverse_phase_b_{0.5F};
    float reverse_offset_a_{0.25F};
    float reverse_offset_b_{0.58F};
    std::uint32_t reverse_random_state_{0x6A09'E667U};
    std::vector<StereoFrame> delay_{};''')

replace_once(path,
'''    std::array<ModulatorRuntime, kModulatorsPerSlot> modulators{};
    SmoothedValue frequency{};''',
'''    std::array<ModulatorRuntime, kModulatorsPerSlot> modulators{};
    EuclideanRuntime euclidean{};
    SmoothedValue frequency{};''')
replace_once(path,
'''        for (auto& modulator : modulators) {
            modulator.reset();
        }
    }''',
'''        for (auto& modulator : modulators) {
            modulator.reset();
        }
        euclidean.reset();
    }''')

replace_once(path,
'''        session_ = initial_session;
        master_.prepare''',
'''        session_ = initial_session;
        rebuild_effective_slots();
        master_.prepare''')
replace_once(path,
'''            slots_[index].prepare(config_.sample_rate, session_.slots[index], index);''',
'''            slots_[index].prepare(config_.sample_rate, effective_slots_[index], index);''')
replace_once(path,
'''            performance_events_.set(clamp01(session_.performance.events));
            for (std::size_t index = 0; index < kSlotCount; ++index) {
                slots_[index].update_targets(session_.slots[index]);
            }''',
'''            performance_events_.set(clamp01(session_.performance.events));
            rebuild_effective_slots();
            for (std::size_t index = 0; index < kSlotCount; ++index) {
                slots_[index].update_targets(effective_slots_[index]);
            }''')
replace_once(path,
'''                const auto& settings = session_.slots[slot_index];''',
'''                const auto& settings = effective_slots_[slot_index];''')
replace_once(path,
'''                for (std::size_t mod_index = 0; mod_index < kModulatorsPerSlot; ++mod_index) {
                    const auto& mod = settings.modulators[mod_index];
                    apply_modulation(
                        parameters,
                        mod.destination,
                        runtime.modulators[mod_index].next(mod, config_.sample_rate));
                }''',
'''                std::array<float, kModulatorsPerSlot> modulation_values{};
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
                    settings.euclidean, session_.tempo_bpm, config_.sample_rate);''')
replace_once(path,
'''                case EngineKind::diagnostic:
                    parameters.timbre''',
'''                case EngineKind::plaits:
                    parameters.timbre += texture_macro * 0.22F;
                    parameters.color += chaos_macro * 0.16F;
                    parameters.motion += events_macro * 0.24F;
                    parameters.texture += space_macro * 0.18F;
                    break;
                case EngineKind::diagnostic:
                    parameters.timbre''')
replace_once(path,
'''                case EngineKind::earth_rumble: pulse_ratio = 0.10F; pulse_depth = 0.02F; break;
                case EngineKind::diagnostic: break;''',
'''                case EngineKind::earth_rumble: pulse_ratio = 0.10F; pulse_depth = 0.02F; break;
                case EngineKind::plaits: pulse_ratio = 1.0F; pulse_depth = 0.14F; break;
                case EngineKind::diagnostic: break;''')
replace_once(path,
'''                    case EffectKind::bypass:
                        break;''',
'''                    case EffectKind::granular_reverse:
                        parameters.effect_amount[effect_index] += space_macro * 0.74F;
                        parameters.effect_tone[effect_index] += chaos_macro * 0.18F;
                        parameters.effect_feedback[effect_index] += space_macro * 0.40F;
                        break;
                    case EffectKind::bypass:
                        break;''')
replace_once(path,
'''                float mono = 0.0F;
                if (settings.enabled) {
                    mono = runtime.engine.next(
                        settings.engine,
                        parameters.frequency,
                        parameters.timbre,
                        parameters.color,
                        parameters.motion,
                        parameters.texture,
                        session_.tempo_bpm,
                        pulse_macro,
                        chaos_macro,
                        events_macro);
                }
                const float left_pan = std::sqrt(0.5F * (1.0F - parameters.pan));
                const float right_pan = std::sqrt(0.5F * (1.0F + parameters.pan));
                StereoFrame slot_frame{
                    mono * parameters.level * left_pan,
                    mono * parameters.level * right_pan,
                };''',
'''                StereoFrame actor_frame{};
                if (settings.enabled) {
                    actor_frame = runtime.engine.next(
                        settings,
                        parameters.frequency,
                        parameters.timbre,
                        parameters.color,
                        parameters.motion,
                        parameters.texture,
                        session_.tempo_bpm,
                        pulse_macro,
                        chaos_macro,
                        events_macro,
                        euclidean_trigger);
                }
                const float left_pan = std::sqrt(0.5F * (1.0F - parameters.pan));
                const float right_pan = std::sqrt(0.5F * (1.0F + parameters.pan));
                StereoFrame slot_frame{
                    actor_frame.left * parameters.level * left_pan,
                    actor_frame.right * parameters.level * right_pan,
                };''')

replace_once(path,
'''    static void publish_meter(std::atomic<float>& destination, float& smoothed, float value, float mix) noexcept {''',
'''    void rebuild_effective_slots() noexcept {
        Session target = session_;
        apply_scene_recipe(target, session_.performance.morph_target);
        const float morph = clamp01(session_.performance.morph);
        const auto blend = [morph](float a, float b) { return a + (b - a) * morph; };
        for (std::size_t index = 0; index < kSlotCount; ++index) {
            const auto& source = session_.slots[index];
            const auto& destination = target.slots[index];
            auto& result = effective_slots_[index];
            result = source;
            result.enabled = source.enabled || destination.enabled;
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
            result.level = blend(source.level, destination.level);
            result.pan = blend(source.pan, destination.pan);
        }
    }

    static void publish_meter(std::atomic<float>& destination, float& smoothed, float value, float mix) noexcept {''')
replace_once(path,
'''    Session session_{};
    std::array<SlotRuntime, kSlotCount> slots_{};''',
'''    Session session_{};
    std::array<SlotSettings, kSlotCount> effective_slots_{};
    std::array<SlotRuntime, kSlotCount> slots_{};''')

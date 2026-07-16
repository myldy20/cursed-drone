// SPDX-License-Identifier: GPL-3.0-or-later
#include "cursed_drone/audio.hpp"

#include "Noise/clockednoise.h"
#include "Noise/grainlet.h"
#include "Noise/particle.h"
#include "PhysicalModeling/modalvoice.h"
#include "Synthesis/oscillator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <utility>
#include <vector>

namespace cursed_drone {
namespace {

constexpr float kPi = std::numbers::pi_v<float>;
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

    float next(const ModSettings& settings, float sample_rate) noexcept {
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
    }

private:
    float phase_{0.0F};
    float held_{0.0F};
    float walk_{0.0F};
    std::uint32_t random_state_{0xA341'316CU};
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
        frequency = std::clamp(frequency, 8.0F, sample_rate * 0.2F);
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
        clocked_noise_.Init(sample_rate_);
        event_phase_ = 1.0F;
        event_period_ = 1.0F;
        grain_envelope_ = 0.0F;
        drift_phase_ = 0.0F;
    }

    float next(
        EngineKind kind,
        float frequency,
        float timbre,
        float color,
        float motion,
        float texture,
        float tempo_bpm,
        float pulse,
        float chaos,
        float events) noexcept {
        frequency = std::clamp(frequency, 8.0F, sample_rate_ * 0.2F);
        timbre = clamp01(timbre);
        color = clamp01(color);
        motion = clamp01(motion);
        texture = clamp01(texture);
        events = clamp01(events);

        const float beat_hz = std::clamp(tempo_bpm, 10.0F, 300.0F) / 60.0F;
        const float event_rate = beat_hz * (0.08F + events * events * 2.9F) * (0.65F + motion * 1.25F);
        event_phase_ += event_rate / sample_rate_;
        bool trigger = false;
        if (event_phase_ >= event_period_) {
            event_phase_ -= event_period_;
            const float random_unit = next_noise(random_state_) * 0.5F + 0.5F;
            const float irregularity = (0.18F + chaos * 0.72F) * (1.0F - pulse * 0.72F);
            event_period_ = std::clamp(1.0F + (random_unit * 2.0F - 1.0F) * irregularity, 0.35F, 1.8F);
            trigger = true;
            grain_envelope_ = 1.0F;
        }

        drift_phase_ += (0.007F + motion * motion * 0.19F) / sample_rate_;
        drift_phase_ -= std::floor(drift_phase_);
        const float drift = std::sin(drift_phase_ * kTwoPi) * (0.0005F + motion * 0.018F);

        switch (kind) {
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
            const float decay = std::exp(-1.0F / (sample_rate_ * (0.025F + (1.0F - motion) * 0.48F)));
            grain_envelope_ *= decay;
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
        }
        return 0.0F;
    }

private:
    float sample_rate_{48'000.0F};
    std::size_t slot_index_{0U};
    std::uint32_t random_state_{0x91E1'0DA5U};
    float event_phase_{1.0F};
    float event_period_{1.0F};
    float grain_envelope_{0.0F};
    float drift_phase_{0.0F};
    DiagnosticEngine diagnostic_{};
    daisysp::Oscillator fundamental_{};
    daisysp::Oscillator overtone_{};
    daisysp::ModalVoice body_{};
    daisysp::GrainletOscillator grain_{};
    daisysp::Particle particle_{};
    daisysp::ClockedNoise clocked_noise_{};
};

class EffectRuntime {
public:
    void prepare(float sample_rate) {
        sample_rate_ = sample_rate;
        const auto delay_frames = static_cast<std::size_t>(std::ceil(sample_rate_ * 1.3F)) + 1U;
        delay_.assign(delay_frames, {});
        reset();
    }

    void reset() noexcept {
        lowpass_ = {};
        tremolo_phase_ = 0.0F;
        delay_write_ = 0U;
        crusher_counter_ = 0U;
        crusher_held_ = {};
        std::fill(delay_.begin(), delay_.end(), StereoFrame{});
    }

    StereoFrame process(StereoFrame input, EffectKind kind, float amount, float tone, float feedback) noexcept {
        amount = clamp01(amount);
        tone = clamp01(tone);
        feedback = clamp01(feedback);

        switch (kind) {
        case EffectKind::bypass:
            return input;
        case EffectKind::drive:
            return drive(input, amount);
        case EffectKind::lowpass:
            return lowpass(input, amount, tone);
        case EffectKind::tremolo:
            return tremolo(input, amount, tone);
        case EffectKind::delay:
            return delay(input, amount, tone, feedback);
        case EffectKind::crusher:
            return crusher(input, amount, tone);
        }
        return input;
    }

private:
    static StereoFrame drive(StereoFrame input, float amount) noexcept {
        const float gain = 1.0F + amount * 18.0F;
        const float normalizer = 1.0F / std::tanh(gain);
        const StereoFrame wet{
            std::tanh(input.left * gain) * normalizer,
            std::tanh(input.right * gain) * normalizer,
        };
        return {
            input.left + (wet.left - input.left) * amount,
            input.right + (wet.right - input.right) * amount,
        };
    }

    StereoFrame lowpass(StereoFrame input, float amount, float tone) noexcept {
        const float cutoff = 35.0F * std::pow(480.0F, tone);
        const float coefficient = 1.0F - std::exp(-kTwoPi * cutoff / sample_rate_);
        lowpass_.left += (input.left - lowpass_.left) * coefficient;
        lowpass_.right += (input.right - lowpass_.right) * coefficient;
        return {
            input.left + (lowpass_.left - input.left) * amount,
            input.right + (lowpass_.right - input.right) * amount,
        };
    }

    StereoFrame tremolo(StereoFrame input, float amount, float tone) noexcept {
        const float rate = 0.03F + tone * tone * 12.0F;
        tremolo_phase_ += rate / sample_rate_;
        tremolo_phase_ -= std::floor(tremolo_phase_);
        const float modulation = 1.0F - amount * (0.5F + 0.5F * std::sin(tremolo_phase_ * kTwoPi));
        return {input.left * modulation, input.right * modulation};
    }

    StereoFrame delay(StereoFrame input, float amount, float tone, float feedback) noexcept {
        if (delay_.empty()) {
            return input;
        }
        const float delay_seconds = 0.05F + tone * 1.2F;
        const auto delay_frames = std::min(
            delay_.size() - 1U,
            static_cast<std::size_t>(delay_seconds * sample_rate_));
        const auto read = (delay_write_ + delay_.size() - delay_frames) % delay_.size();
        const StereoFrame wet = delay_[read];
        const float regeneration = feedback * 0.92F;
        delay_[delay_write_] = {
            input.left + wet.right * regeneration,
            input.right + wet.left * regeneration,
        };
        delay_write_ = (delay_write_ + 1U) % delay_.size();
        return {
            input.left + wet.left * amount,
            input.right + wet.right * amount,
        };
    }

    StereoFrame crusher(StereoFrame input, float amount, float tone) noexcept {
        const auto hold_samples = static_cast<unsigned>(1.0F + tone * tone * 63.0F);
        if (crusher_counter_ == 0U) {
            const float levels = std::pow(2.0F, 15.0F - amount * 12.0F);
            crusher_held_ = {
                std::round(input.left * levels) / levels,
                std::round(input.right * levels) / levels,
            };
            crusher_counter_ = hold_samples;
        }
        --crusher_counter_;
        return {
            input.left + (crusher_held_.left - input.left) * amount,
            input.right + (crusher_held_.right - input.right) * amount,
        };
    }

    float sample_rate_{48'000.0F};
    StereoFrame lowpass_{};
    float tremolo_phase_{0.0F};
    std::vector<StereoFrame> delay_{};
    std::size_t delay_write_{0U};
    unsigned crusher_counter_{0U};
    StereoFrame crusher_held_{};
};

struct SlotRuntime {
    ProductEngine engine{};
    std::array<EffectRuntime, kEffectsPerSlot> effects{};
    std::array<ModulatorRuntime, kModulatorsPerSlot> modulators{};
    SmoothedValue frequency{};
    SmoothedValue timbre{};
    SmoothedValue color{};
    SmoothedValue motion{};
    SmoothedValue texture{};
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
    }
};

struct ModulatedParameters {
    float frequency{55.0F};
    float timbre{0.5F};
    float color{0.5F};
    float motion{0.25F};
    float texture{0.25F};
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
        master_.prepare(session_.master_level * session_.performance.fade, config_.sample_rate, 100.0F);
        performance_texture_.prepare(session_.performance.texture, config_.sample_rate, 80.0F);
        performance_pulse_.prepare(session_.performance.pulse, config_.sample_rate, 80.0F);
        performance_chaos_.prepare(session_.performance.chaos, config_.sample_rate, 120.0F);
        performance_space_.prepare(session_.performance.space, config_.sample_rate, 100.0F);
        performance_events_.prepare(session_.performance.events, config_.sample_rate, 100.0F);
        for (std::size_t index = 0; index < kSlotCount; ++index) {
            slots_[index].prepare(config_.sample_rate, session_.slots[index], index);
        }
        reset();
        prepared_ = true;
    }

    void reset() noexcept {
        dc_input_ = {};
        dc_output_ = {};
        pulse_phase_ = 0.0F;
        chaos_phase_ = 0.0F;
        pulse_phases_.fill(0.0F);
        chaos_current_.fill(0.0F);
        chaos_target_.fill(0.0F);
        master_.snap();
        performance_texture_.snap();
        performance_pulse_.snap();
        performance_chaos_.snap();
        performance_space_.snap();
        performance_events_.snap();
        for (auto& slot : slots_) {
            slot.reset();
        }
        clear_telemetry();
    }

    bool submit_session(const Session& session) noexcept { return sessions_.push(session); }

    void panic() noexcept { panic_requested_.store(true, std::memory_order_release); }

    AudioTelemetry telemetry() const noexcept {
        AudioTelemetry result{};
        for (std::size_t index = 0; index < kSlotCount; ++index) {
            result.slot_rms[index] = slot_rms_[index].load(std::memory_order_relaxed);
            result.slot_peak[index] = slot_peak_[index].load(std::memory_order_relaxed);
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

    void process(std::span<StereoFrame> output) noexcept {
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
            session_ = incoming;
            master_.set(session_.master_level * clamp01(session_.performance.fade));
            performance_texture_.set(clamp01(session_.performance.texture));
            performance_pulse_.set(clamp01(session_.performance.pulse));
            performance_chaos_.set(clamp01(session_.performance.chaos));
            performance_space_.set(clamp01(session_.performance.space));
            performance_events_.set(clamp01(session_.performance.events));
            for (std::size_t index = 0; index < kSlotCount; ++index) {
                slots_[index].update_targets(session_.slots[index]);
            }
        }

        if (panic_requested_.exchange(false, std::memory_order_acq_rel)) {
            reset();
            return;
        }

        std::array<double, kSlotCount> slot_energy{};
        std::array<float, kSlotCount> slot_peak{};
        double master_energy = 0.0;
        float master_peak = 0.0F;
        float last_chaos_activity = 0.0F;
        std::array<std::array<float, kScopePointCount>, kSlotCount> scope_frames{};
        std::array<float, kScopePointCount> master_scope_frame{};

        for (std::size_t frame_index = 0; frame_index < output.size(); ++frame_index) {
            auto& frame = output[frame_index];
            const std::size_t scope_index = std::min(
                kScopePointCount - 1U, frame_index * kScopePointCount / output.size());
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
            const float chaos_slew = 1.0F - std::exp(-1.0F / (config_.sample_rate * 0.075F));
            last_chaos_activity = 0.0F;
            for (std::size_t index = 0; index < kSlotCount; ++index) {
                chaos_current_[index] += (chaos_target_[index] - chaos_current_[index]) * chaos_slew;
                last_chaos_activity = std::max(last_chaos_activity, std::abs(chaos_current_[index]) * chaos_macro);
            }

            StereoFrame mix{};
            for (std::size_t slot_index = 0; slot_index < kSlotCount; ++slot_index) {
                const auto& settings = session_.slots[slot_index];
                auto& runtime = slots_[slot_index];
                ModulatedParameters parameters{
                    runtime.frequency.next(),
                    runtime.timbre.next(),
                    runtime.color.next(),
                    runtime.motion.next(),
                    runtime.texture.next(),
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
                for (std::size_t mod_index = 0; mod_index < kModulatorsPerSlot; ++mod_index) {
                    const auto& mod = settings.modulators[mod_index];
                    apply_modulation(
                        parameters,
                        mod.destination,
                        runtime.modulators[mod_index].next(mod, config_.sample_rate));
                }

                const float chaos_value = chaos_current_[slot_index] * chaos_macro;
                parameters.frequency *= std::pow(2.0F, chaos_value * 0.36F);
                switch (settings.engine) {
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
                case EngineKind::diagnostic: break;
                }
                pulse_phases_[slot_index] += pulse_rate * pulse_ratio / config_.sample_rate;
                pulse_phases_[slot_index] -= std::floor(pulse_phases_[slot_index]);
                float slot_pulse_phase = pulse_phases_[slot_index] + chaos_value * 0.08F;
                slot_pulse_phase -= std::floor(slot_pulse_phase);
                const float pulse_wave = 0.5F + 0.5F * std::sin(slot_pulse_phase * kTwoPi - kPi * 0.5F);
                const float pulse_envelope = std::pow(pulse_wave, 1.0F + pulse_macro * 10.0F);
                const float pulse_gain = 1.0F - pulse_macro * pulse_depth * (1.0F - pulse_envelope);
                parameters.level *= pulse_gain * std::clamp(1.0F + chaos_value * 0.72F, 0.16F, 1.65F);

                for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {
                    switch (settings.effects[effect_index].kind) {
                    case EffectKind::drive:
                    case EffectKind::crusher:
                        parameters.effect_amount[effect_index] += texture_macro * 0.42F;
                        break;
                    case EffectKind::lowpass:
                        parameters.effect_tone[effect_index] += texture_macro * 0.16F;
                        break;
                    case EffectKind::tremolo:
                        parameters.effect_amount[effect_index] += pulse_macro * 0.20F;
                        break;
                    case EffectKind::delay:
                        parameters.effect_amount[effect_index] += space_macro * 0.58F;
                        parameters.effect_feedback[effect_index] += space_macro * 0.46F;
                        parameters.effect_tone[effect_index] += space_macro * 0.13F;
                        break;
                    case EffectKind::bypass:
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
                parameters.level = std::clamp(parameters.level, 0.0F, 1.5F);
                parameters.pan = std::clamp(parameters.pan, -1.0F, 1.0F);

                float mono = 0.0F;
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
                };

                for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {
                    slot_frame = runtime.effects[effect_index].process(
                        slot_frame,
                        settings.effects[effect_index].kind,
                        parameters.effect_amount[effect_index],
                        parameters.effect_tone[effect_index],
                        parameters.effect_feedback[effect_index]);
                }
                scope_frames[slot_index][scope_index] = 0.5F * (slot_frame.left + slot_frame.right);
                mix.left += slot_frame.left;
                mix.right += slot_frame.right;
                const float slot_power = 0.5F * (slot_frame.left * slot_frame.left + slot_frame.right * slot_frame.right);
                slot_energy[slot_index] += static_cast<double>(slot_power);
                slot_peak[slot_index] = std::max(
                    slot_peak[slot_index], std::max(std::abs(slot_frame.left), std::abs(slot_frame.right)));
            }

            const float master = master_.next();
            mix.left *= master;
            mix.right *= master;

            constexpr float dc_coefficient = 0.995F;
            const StereoFrame dc_removed{
                mix.left - dc_input_.left + dc_coefficient * dc_output_.left,
                mix.right - dc_input_.right + dc_coefficient * dc_output_.right,
            };
            dc_input_ = mix;
            dc_output_ = dc_removed;

            constexpr float limiter_drive = 1.35F;
            frame = {
                std::tanh(dc_removed.left * limiter_drive),
                std::tanh(dc_removed.right * limiter_drive),
            };
            master_scope_frame[scope_index] = 0.5F * (frame.left + frame.right);
            const float master_power = 0.5F * (frame.left * frame.left + frame.right * frame.right);
            master_energy += static_cast<double>(master_power);
            master_peak = std::max(master_peak, std::max(std::abs(frame.left), std::abs(frame.right)));
        }

        const float frame_count = static_cast<float>(output.size());
        for (std::size_t index = 0; index < kSlotCount; ++index) {
            publish_meter(slot_rms_[index], slot_rms_smooth_[index],
                std::sqrt(static_cast<float>(slot_energy[index]) / frame_count), 0.18F);
            publish_peak(slot_peak_[index], slot_peak_smooth_[index], slot_peak[index]);
            for (std::size_t point = 0; point < kScopePointCount; ++point) {
                slot_scope_[index][point].store(scope_frames[index][point], std::memory_order_relaxed);
            }
        }
        for (std::size_t point = 0; point < kScopePointCount; ++point) {
            master_scope_[point].store(master_scope_frame[point], std::memory_order_relaxed);
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
    std::array<SlotRuntime, kSlotCount> slots_{};
    SpscQueue<Session, 8> sessions_{};
    SmoothedValue master_{};
    SmoothedValue performance_texture_{};
    SmoothedValue performance_pulse_{};
    SmoothedValue performance_chaos_{};
    SmoothedValue performance_space_{};
    SmoothedValue performance_events_{};
    StereoFrame dc_input_{};
    StereoFrame dc_output_{};
    std::array<float, kSlotCount> chaos_current_{};
    std::array<float, kSlotCount> chaos_target_{};
    std::array<float, kSlotCount> pulse_phases_{};
    float pulse_phase_{0.0F};
    float chaos_phase_{0.0F};
    std::uint32_t chaos_random_state_{0xD00D'F00DU};
    std::array<std::atomic<float>, kSlotCount> slot_rms_{};
    std::array<std::atomic<float>, kSlotCount> slot_peak_{};
    std::array<std::array<std::atomic<float>, kScopePointCount>, kSlotCount> slot_scope_{};
    std::array<std::atomic<float>, kScopePointCount> master_scope_{};
    std::atomic<float> master_rms_{0.0F};
    std::atomic<float> master_peak_{0.0F};
    std::atomic<float> pulse_meter_{0.0F};
    std::atomic<float> chaos_meter_{0.0F};
    std::array<float, kSlotCount> slot_rms_smooth_{};
    std::array<float, kSlotCount> slot_peak_smooth_{};
    float master_rms_smooth_{0.0F};
    float master_peak_smooth_{0.0F};
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
void AudioGraph::process(std::span<StereoFrame> output) noexcept { impl_->process(output); }
AudioTelemetry AudioGraph::telemetry() const noexcept { return impl_->telemetry(); }
void AudioGraph::panic() noexcept { impl_->panic(); }
float AudioGraph::sample_rate() const noexcept { return impl_->sample_rate(); }

} // namespace cursed_drone

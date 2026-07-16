// SPDX-License-Identifier: GPL-3.0-or-later
#include "cursed_drone/audio.hpp"

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
    void reset() noexcept {
        phase_a_ = 0.0F;
        phase_b_ = 0.0F;
        noise_filter_ = 0.0F;
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
        const float body = fundamental * (0.82F - timbre * 0.32F) + overtone * (0.08F + timbre * 0.30F);
        return body * (1.0F - texture * 0.45F) + noise_filter_ * texture * 1.7F;
    }

private:
    float phase_a_{0.0F};
    float phase_b_{0.0F};
    float noise_filter_{0.0F};
    std::uint32_t random_state_{0xC801'3EA4U};
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
    DiagnosticEngine diagnostic{};
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

    void prepare(float sample_rate, const SlotSettings& settings) {
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
        diagnostic.reset();
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
        master_.prepare(session_.master_level, config_.sample_rate, 30.0F);
        for (std::size_t index = 0; index < kSlotCount; ++index) {
            slots_[index].prepare(config_.sample_rate, session_.slots[index]);
        }
        reset();
        prepared_ = true;
    }

    void reset() noexcept {
        dc_input_ = {};
        dc_output_ = {};
        master_.snap();
        for (auto& slot : slots_) {
            slot.reset();
        }
    }

    bool submit_session(const Session& session) noexcept {
        return sessions_.push(session);
    }

    void process(std::span<StereoFrame> output) noexcept {
        std::fill(output.begin(), output.end(), StereoFrame{});
        if (!prepared_) {
            return;
        }

        Session incoming{};
        bool received = false;
        while (sessions_.pop(incoming)) {
            received = true;
        }
        if (received) {
            session_ = incoming;
            master_.set(session_.master_level);
            for (std::size_t index = 0; index < kSlotCount; ++index) {
                slots_[index].update_targets(session_.slots[index]);
            }
        }

        for (auto& frame : output) {
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
                };
                for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {
                    parameters.effect_amount[effect_index] = runtime.effect_amount[effect_index].next();
                }
                for (std::size_t mod_index = 0; mod_index < kModulatorsPerSlot; ++mod_index) {
                    const auto& mod = settings.modulators[mod_index];
                    apply_modulation(
                        parameters,
                        mod.destination,
                        runtime.modulators[mod_index].next(mod, config_.sample_rate));
                }

                parameters.timbre = clamp01(parameters.timbre);
                parameters.color = clamp01(parameters.color);
                parameters.motion = clamp01(parameters.motion);
                parameters.texture = clamp01(parameters.texture);
                parameters.level = std::clamp(parameters.level, 0.0F, 1.25F);
                parameters.pan = std::clamp(parameters.pan, -1.0F, 1.0F);

                float mono = 0.0F;
                if (settings.enabled && settings.engine == EngineKind::diagnostic) {
                    mono = runtime.diagnostic.next(
                        parameters.frequency,
                        parameters.timbre,
                        parameters.color,
                        parameters.motion,
                        parameters.texture,
                        config_.sample_rate);
                }
                const float left_pan = std::sqrt(0.5F * (1.0F - parameters.pan));
                const float right_pan = std::sqrt(0.5F * (1.0F + parameters.pan));
                StereoFrame slot_frame{
                    mono * parameters.level * left_pan,
                    mono * parameters.level * right_pan,
                };

                for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {
                    const auto& effect = settings.effects[effect_index];
                    slot_frame = runtime.effects[effect_index].process(
                        slot_frame,
                        effect.kind,
                        parameters.effect_amount[effect_index],
                        runtime.effect_tone[effect_index].next(),
                        runtime.effect_feedback[effect_index].next());
                }
                mix.left += slot_frame.left;
                mix.right += slot_frame.right;
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
        }
    }

    float sample_rate() const noexcept { return config_.sample_rate; }

private:
    AudioConfig config_{};
    Session session_{};
    std::array<SlotRuntime, kSlotCount> slots_{};
    SpscQueue<Session, 8> sessions_{};
    SmoothedValue master_{};
    StereoFrame dc_input_{};
    StereoFrame dc_output_{};
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
float AudioGraph::sample_rate() const noexcept { return impl_->sample_rate(); }

} // namespace cursed_drone

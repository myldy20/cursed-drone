#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text(encoding='utf-8')
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f'{path}: expected one match, found {count}: {old[:120]!r}')
    p.write_text(text.replace(old, new), encoding='utf-8')


def replace_between(path: str, start: str, end: str, replacement: str) -> None:
    p = Path(path)
    text = p.read_text(encoding='utf-8')
    a = text.find(start)
    if a < 0:
        raise RuntimeError(f'{path}: start marker missing: {start!r}')
    b = text.find(end, a)
    if b < 0:
        raise RuntimeError(f'{path}: end marker missing: {end!r}')
    p.write_text(text[:a] + replacement + text[b:], encoding='utf-8')

# ---------------------------------------------------------------------------
# Version and session model
# ---------------------------------------------------------------------------
replace_once('CMakeLists.txt',
             'project(cursed_drone VERSION 0.8.0 LANGUAGES CXX)',
             'project(cursed_drone VERSION 0.9.0 LANGUAGES CXX)')

replace_once('include/cursed_drone/session.hpp',
'''enum class EffectKind {
    bypass,
    drive,
    lowpass,
    highpass,
    tremolo,
    delay,
    crusher,
    wavefolder,
    ringmod,
    comb
};''',
'''enum class EffectKind {
    bypass,
    drive,
    lowpass,
    highpass,
    tremolo,
    delay,
    crusher,
    wavefolder,
    ringmod,
    comb,
    chorus,
    flanger,
    phaser,
    diffuser,
    ahdr,
    tape_void,
    black_hole,
    ritual_gate,
    rust_cloud,
    deep_sea
};''')
replace_once('include/cursed_drone/session.hpp',
             'unsigned schema_version{7};',
             'unsigned schema_version{8};')

replace_once('src/session.cpp',
'''    std::pair{EffectKind::ringmod, std::string_view{"ringmod"}},
    std::pair{EffectKind::comb, std::string_view{"comb"}},
};''',
'''    std::pair{EffectKind::ringmod, std::string_view{"ringmod"}},
    std::pair{EffectKind::comb, std::string_view{"comb"}},
    std::pair{EffectKind::chorus, std::string_view{"chorus"}},
    std::pair{EffectKind::flanger, std::string_view{"flanger"}},
    std::pair{EffectKind::phaser, std::string_view{"phaser"}},
    std::pair{EffectKind::diffuser, std::string_view{"diffuser"}},
    std::pair{EffectKind::ahdr, std::string_view{"ahdr"}},
    std::pair{EffectKind::tape_void, std::string_view{"tape_void"}},
    std::pair{EffectKind::black_hole, std::string_view{"black_hole"}},
    std::pair{EffectKind::ritual_gate, std::string_view{"ritual_gate"}},
    std::pair{EffectKind::rust_cloud, std::string_view{"rust_cloud"}},
    std::pair{EffectKind::deep_sea, std::string_view{"deep_sea"}},
};''')
replace_once('src/session.cpp',
'''            schema->second != "4" && schema->second != "5" && schema->second != "6" &&
            schema->second != "7")) {''',
'''            schema->second != "4" && schema->second != "5" && schema->second != "6" &&
            schema->second != "7" && schema->second != "8")) {''')
replace_once('src/session.cpp',
'''    if (schema->second != "4" && schema->second != "5" && schema->second != "6" &&
        schema->second != "7") {''',
'''    if (schema->second != "4" && schema->second != "5" && schema->second != "6" &&
        schema->second != "7" && schema->second != "8") {''')
replace_once('src/session.cpp', '    loaded.schema_version = 7;', '    loaded.schema_version = 8;')

# ---------------------------------------------------------------------------
# Stronger basic effects plus efficient compound ambient/drone processors.
# The topology intentionally reuses the existing per-slot delay memory.
# ---------------------------------------------------------------------------
effect_runtime = r'''class EffectRuntime {
public:
    void prepare(float sample_rate) {
        sample_rate_ = sample_rate;
        const auto delay_frames = static_cast<std::size_t>(std::ceil(sample_rate_ * 1.3F)) + 2U;
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
        delay_write_ = 0U;
        short_write_ = 0U;
        crusher_counter_ = 0U;
        crusher_held_ = {};
        phaser_left_.fill(0.0F);
        phaser_right_.fill(0.0F);
        std::fill(delay_.begin(), delay_.end(), StereoFrame{});
        std::fill(short_delay_.begin(), short_delay_.end(), StereoFrame{});
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

    StereoFrame read_delay(
        const std::vector<StereoFrame>& buffer,
        std::size_t write,
        float frames) const noexcept {
        if (buffer.size() < 2U) return {};
        frames = std::clamp(frames, 1.0F, static_cast<float>(buffer.size() - 2U));
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

    static StereoFrame drive(StereoFrame input, float amount) noexcept {
        const float gain = 1.0F + amount * 48.0F;
        const float normalizer = 1.0F / std::max(0.001F, std::tanh(gain));
        const StereoFrame wet{
            std::tanh(input.left * gain) * normalizer,
            std::tanh(input.right * gain) * normalizer,
        };
        return mix(input, wet, amount);
    }

    StereoFrame lowpass(StereoFrame input, float amount, float tone) noexcept {
        const float cutoff = std::min(sample_rate_ * 0.42F, 18.0F * std::pow(1'000.0F, tone));
        const float coefficient = 1.0F - std::exp(-kTwoPi * cutoff / sample_rate_);
        lowpass_.left += (input.left - lowpass_.left) * coefficient;
        lowpass_.right += (input.right - lowpass_.right) * coefficient;
        return mix(input, lowpass_, amount);
    }

    StereoFrame highpass(StereoFrame input, float amount, float tone) noexcept {
        const float cutoff = std::min(sample_rate_ * 0.42F, 12.0F * std::pow(1'000.0F, tone));
        const float coefficient = 1.0F - std::exp(-kTwoPi * cutoff / sample_rate_);
        highpass_low_.left += (input.left - highpass_low_.left) * coefficient;
        highpass_low_.right += (input.right - highpass_low_.right) * coefficient;
        return mix(input, {input.left - highpass_low_.left, input.right - highpass_low_.right}, amount);
    }

    StereoFrame tremolo(StereoFrame input, float amount, float tone) noexcept {
        const float rate = 0.015F + tone * tone * 18.0F;
        const float phase = advance(tremolo_phase_, rate, sample_rate_);
        const float wave = 0.5F + 0.5F * std::sin(phase * kTwoPi);
        const float modulation = 1.0F - amount * (0.20F + wave * 0.80F);
        return {input.left * modulation, input.right * modulation};
    }

    StereoFrame delay(StereoFrame input, float amount, float tone, float feedback) noexcept {
        if (delay_.empty()) return input;
        const float delay_seconds = 0.025F + std::pow(tone, 1.35F) * 1.265F;
        const StereoFrame wet = read_delay(delay_, delay_write_, delay_seconds * sample_rate_);
        const float regeneration = std::min(0.985F, feedback * 0.985F);
        delay_[delay_write_] = {
            input.left + wet.right * regeneration,
            input.right + wet.left * regeneration,
        };
        delay_write_ = (delay_write_ + 1U) % delay_.size();
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
        const float frequency = 0.05F * std::pow(160'000.0F, tone);
        const float phase = advance(ring_phase_, frequency, sample_rate_);
        const float carrier = std::sin(phase * kTwoPi);
        return {
            input.left * (1.0F - amount + carrier * amount),
            input.right * (1.0F - amount - carrier * amount),
        };
    }

    StereoFrame comb(StereoFrame input, float amount, float tone, float feedback) noexcept {
        if (delay_.empty()) return input;
        const float seconds = 0.0007F * std::pow(90.0F, tone);
        const StereoFrame wet = read_delay(delay_, delay_write_, seconds * sample_rate_);
        const float regeneration = 0.12F + feedback * 0.85F;
        delay_[delay_write_] = {
            input.left + wet.left * regeneration,
            input.right + wet.right * regeneration,
        };
        delay_write_ = (delay_write_ + 1U) % delay_.size();
        return mix(input, wet, amount);
    }

    StereoFrame chorus(StereoFrame input, float amount, float tone, float feedback) noexcept {
        if (short_delay_.empty()) return input;
        const float rate = 0.015F + tone * tone * 4.5F;
        const float phase = advance(chorus_phase_, rate, sample_rate_);
        const float depth_ms = 1.0F + feedback * 14.0F;
        const float base_ms = 7.0F + feedback * 17.0F;
        const float left_frames = (base_ms + std::sin(phase * kTwoPi) * depth_ms) * 0.001F * sample_rate_;
        const float right_frames = (base_ms + std::sin((phase + 0.27F) * kTwoPi) * depth_ms) * 0.001F * sample_rate_;
        const StereoFrame left_tap = read_delay(short_delay_, short_write_, left_frames);
        const StereoFrame right_tap = read_delay(short_delay_, short_write_, right_frames);
        const StereoFrame wet{left_tap.left * 0.72F + right_tap.left * 0.28F,
                              right_tap.right * 0.72F + left_tap.right * 0.28F};
        short_delay_[short_write_] = input;
        short_write_ = (short_write_ + 1U) % short_delay_.size();
        return mix(input, wet, amount);
    }

    StereoFrame flanger(StereoFrame input, float amount, float tone, float feedback) noexcept {
        if (short_delay_.empty()) return input;
        const float rate = 0.025F + tone * tone * 7.0F;
        const float phase = advance(flanger_phase_, rate, sample_rate_);
        const float delay_ms = 0.35F + (0.5F + 0.5F * std::sin(phase * kTwoPi)) * (1.5F + tone * 7.0F);
        const StereoFrame wet = read_delay(short_delay_, short_write_, delay_ms * 0.001F * sample_rate_);
        const float regeneration = feedback * 0.92F;
        short_delay_[short_write_] = {
            input.left + wet.right * regeneration,
            input.right + wet.left * regeneration,
        };
        short_write_ = (short_write_ + 1U) % short_delay_.size();
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
        const float sweep = 0.5F + 0.5F * std::sin(phase * kTwoPi);
        const float frequency = std::min(sample_rate_ * 0.18F, 45.0F * std::pow(120.0F, sweep));
        const float tangent = std::tan(kPi * frequency / sample_rate_);
        const float coefficient = std::clamp((1.0F - tangent) / (1.0F + tangent), -0.98F, 0.98F);
        const StereoFrame driven{
            input.left + phaser_feedback_.left * feedback * 0.88F,
            input.right + phaser_feedback_.right * feedback * 0.88F,
        };
        const StereoFrame wet{
            allpass(driven.left, phaser_left_, coefficient),
            allpass(driven.right, phaser_right_, -coefficient),
        };
        phaser_feedback_ = wet;
        return mix(input, wet, amount);
    }

    StereoFrame diffuser(StereoFrame input, float amount, float tone, float feedback) noexcept {
        if (delay_.empty()) return input;
        const float base = (0.012F + tone * 0.105F) * sample_rate_;
        const StereoFrame a = read_delay(delay_, delay_write_, base);
        const StereoFrame b = read_delay(delay_, delay_write_, base * 1.37F);
        const StereoFrame c = read_delay(delay_, delay_write_, base * 1.91F);
        const StereoFrame d = read_delay(delay_, delay_write_, base * 2.53F);
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
    std::vector<StereoFrame> delay_{};
    std::vector<StereoFrame> short_delay_{};
    std::size_t delay_write_{0U};
    std::size_t short_write_{0U};
    unsigned crusher_counter_{0U};
    StereoFrame crusher_held_{};
    std::array<float, 4> phaser_left_{};
    std::array<float, 4> phaser_right_{};
};

'''
replace_between('src/audio.cpp', 'class EffectRuntime {', 'struct SlotRuntime {', effect_runtime)

# ---------------------------------------------------------------------------
# SDL UI: proper branding, consistent physical-key hints and grouped FX picker.
# ---------------------------------------------------------------------------
replace_between('src/sdl_main.cpp',
                'std::string_view effect_name(cd::EffectKind kind, bool russian) noexcept {',
                'constexpr std::array<std::array<cd::EngineKind, 4>, 8> kEngineGroups',
r'''std::string_view effect_name(cd::EffectKind kind, bool russian) noexcept {
    switch (kind) {
    case cd::EffectKind::bypass: return russian ? "ПУСТО" : "EMPTY";
    case cd::EffectKind::drive: return russian ? "ДРАЙВ" : "DRIVE";
    case cd::EffectKind::lowpass: return russian ? "ФИЛЬТР" : "LOWPASS";
    case cd::EffectKind::highpass: return russian ? "ВЧ-ФИЛЬТР" : "HIGHPASS";
    case cd::EffectKind::tremolo: return russian ? "ТРЕМОЛО" : "TREMOLO";
    case cd::EffectKind::delay: return russian ? "ДЕЛЕЙ" : "DELAY";
    case cd::EffectKind::crusher: return russian ? "КРАШЕР" : "CRUSHER";
    case cd::EffectKind::wavefolder: return russian ? "ФОЛДЕР" : "FOLDER";
    case cd::EffectKind::ringmod: return russian ? "КОЛЬЦО" : "RING MOD";
    case cd::EffectKind::comb: return russian ? "ГРЕБЕНКА" : "COMB";
    case cd::EffectKind::chorus: return russian ? "ХОРУС" : "CHORUS";
    case cd::EffectKind::flanger: return russian ? "ФЛЭНЖЕР" : "FLANGER";
    case cd::EffectKind::phaser: return russian ? "ФЕЙЗЕР" : "PHASER";
    case cd::EffectKind::diffuser: return russian ? "ДИФФУЗОР" : "DIFFUSER";
    case cd::EffectKind::ahdr: return "AHDR";
    case cd::EffectKind::tape_void: return russian ? "ЛЕНТОПУСТОТА" : "TAPE VOID";
    case cd::EffectKind::black_hole: return russian ? "ЧЕРНАЯ ДЫРА" : "BLACK HOLE";
    case cd::EffectKind::ritual_gate: return russian ? "РИТУАЛЬНЫЙ ГЕЙТ" : "RITUAL GATE";
    case cd::EffectKind::rust_cloud: return russian ? "ОБЛАКО РЖАВЧИНЫ" : "RUST CLOUD";
    case cd::EffectKind::deep_sea: return russian ? "ГЛУБИНА" : "DEEP SEA";
    }
    return {};
}

int effect_field_count(cd::EffectKind kind) noexcept {
    switch (kind) {
    case cd::EffectKind::bypass: return 0;
    case cd::EffectKind::drive: return 1;
    case cd::EffectKind::lowpass:
    case cd::EffectKind::highpass:
    case cd::EffectKind::tremolo:
    case cd::EffectKind::crusher:
    case cd::EffectKind::wavefolder:
    case cd::EffectKind::ringmod: return 2;
    case cd::EffectKind::delay:
    case cd::EffectKind::comb:
    case cd::EffectKind::chorus:
    case cd::EffectKind::flanger:
    case cd::EffectKind::phaser:
    case cd::EffectKind::diffuser:
    case cd::EffectKind::ahdr:
    case cd::EffectKind::tape_void:
    case cd::EffectKind::black_hole:
    case cd::EffectKind::ritual_gate:
    case cd::EffectKind::rust_cloud:
    case cd::EffectKind::deep_sea: return 3;
    }
    return 0;
}

std::string_view effect_field(cd::EffectKind kind, int index, bool russian) noexcept {
    switch (kind) {
    case cd::EffectKind::bypass: return russian ? "НЕТ ПАРАМЕТРОВ" : "NO PARAMETERS";
    case cd::EffectKind::drive: return russian ? "ПЕРЕГРУЗ" : "DRIVE";
    case cd::EffectKind::lowpass:
    case cd::EffectKind::highpass:
        return index == 0 ? (russian ? "ПОДМЕСЬ" : "MIX") : (russian ? "СРЕЗ" : "CUTOFF");
    case cd::EffectKind::tremolo:
        return index == 0 ? (russian ? "ГЛУБИНА" : "DEPTH") : (russian ? "СКОРОСТЬ" : "RATE");
    case cd::EffectKind::delay:
        if (index == 0) return russian ? "ПОДМЕСЬ" : "MIX";
        if (index == 1) return russian ? "ВРЕМЯ" : "TIME";
        return russian ? "ПОВТОРЫ" : "FEEDBACK";
    case cd::EffectKind::crusher:
        return index == 0 ? (russian ? "ДРОБЛЕНИЕ" : "CRUSH") : (russian ? "ЧАСТОТА" : "RATE");
    case cd::EffectKind::wavefolder:
        return index == 0 ? (russian ? "СКЛАДКИ" : "FOLD") : (russian ? "ГЛУБИНА" : "DEPTH");
    case cd::EffectKind::ringmod:
        return index == 0 ? (russian ? "ПОДМЕСЬ" : "MIX") : (russian ? "ЧАСТОТА" : "RATE");
    case cd::EffectKind::comb:
        if (index == 0) return russian ? "ПОДМЕСЬ" : "MIX";
        if (index == 1) return russian ? "РАЗМЕР" : "SIZE";
        return russian ? "РЕЗОНАНС" : "RESONANCE";
    case cd::EffectKind::chorus:
        if (index == 0) return russian ? "ПОДМЕСЬ" : "MIX";
        if (index == 1) return russian ? "СКОРОСТЬ" : "RATE";
        return russian ? "ГЛУБИНА" : "DEPTH";
    case cd::EffectKind::flanger:
    case cd::EffectKind::phaser:
        if (index == 0) return russian ? "ПОДМЕСЬ" : "MIX";
        if (index == 1) return russian ? "СКОРОСТЬ" : "RATE";
        return russian ? "ОБР. СВЯЗЬ" : "FEEDBACK";
    case cd::EffectKind::diffuser:
        if (index == 0) return russian ? "ПОДМЕСЬ" : "MIX";
        if (index == 1) return russian ? "РАЗМЕР" : "SIZE";
        return russian ? "РАСПАД" : "DECAY";
    case cd::EffectKind::ahdr:
        if (index == 0) return russian ? "ГЛУБИНА" : "DEPTH";
        if (index == 1) return russian ? "СКОРОСТЬ" : "RATE";
        return russian ? "ФОРМА" : "SHAPE";
    case cd::EffectKind::tape_void:
    case cd::EffectKind::black_hole:
    case cd::EffectKind::ritual_gate:
    case cd::EffectKind::rust_cloud:
    case cd::EffectKind::deep_sea:
        if (index == 0) return russian ? "ИНТЕНСИВН." : "INTENSITY";
        if (index == 1) return russian ? "ХАРАКТЕР" : "CHARACTER";
        return russian ? "ДВИЖЕНИЕ" : "MOTION";
    }
    return {};
}

constexpr std::array<std::array<cd::EngineKind, 4>, 8> kEngineGroups''')

replace_once('src/sdl_main.cpp',
'''constexpr std::array<cd::EffectKind, 10> kEffectKinds{
    cd::EffectKind::bypass, cd::EffectKind::drive, cd::EffectKind::lowpass, cd::EffectKind::highpass,
    cd::EffectKind::tremolo, cd::EffectKind::delay, cd::EffectKind::crusher, cd::EffectKind::wavefolder,
    cd::EffectKind::ringmod, cd::EffectKind::comb};''',
'''constexpr std::array<cd::EffectKind, 15> kBasicEffects{
    cd::EffectKind::bypass, cd::EffectKind::drive, cd::EffectKind::lowpass,
    cd::EffectKind::highpass, cd::EffectKind::tremolo, cd::EffectKind::delay,
    cd::EffectKind::crusher, cd::EffectKind::wavefolder, cd::EffectKind::ringmod,
    cd::EffectKind::comb, cd::EffectKind::chorus, cd::EffectKind::flanger,
    cd::EffectKind::phaser, cd::EffectKind::diffuser, cd::EffectKind::ahdr};

constexpr std::array<cd::EffectKind, 5> kCompoundEffects{
    cd::EffectKind::tape_void, cd::EffectKind::black_hole,
    cd::EffectKind::ritual_gate, cd::EffectKind::rust_cloud, cd::EffectKind::deep_sea};

int effect_group_size(int group) noexcept {
    return group == 0 ? static_cast<int>(kBasicEffects.size())
                      : static_cast<int>(kCompoundEffects.size());
}

cd::EffectKind effect_at(int group, int item) noexcept {
    if (group == 0) return kBasicEffects[static_cast<std::size_t>(item)];
    return kCompoundEffects[static_cast<std::size_t>(item)];
}

std::string_view effect_group_name(int group, bool russian) noexcept {
    if (group == 0) return russian ? "БАЗОВЫЕ" : "BASIC";
    return russian ? "СОСТАВНЫЕ" : "COMPOUND";
}''')

replace_once('src/sdl_main.cpp',
'''    for (int item = 0; item < static_cast<int>(kEffectKinds.size()); ++item) {
        if (kEffectKinds[static_cast<std::size_t>(item)] == current) {
            state.picker_item = item;
        }
    }''',
'''    for (int group = 0; group < 2; ++group) {
        for (int item = 0; item < effect_group_size(group); ++item) {
            if (effect_at(group, item) == current) {
                state.picker_group = group;
                state.picker_item = item;
            }
        }
    }''')
replace_once('src/sdl_main.cpp',
'''    } else if (state.picker == Picker::effect) {
        const int effects = static_cast<int>(kEffectKinds.size());
        state.picker_item = (state.picker_item + horizontal * 5 + vertical + effects) % effects;
    }''',
'''    } else if (state.picker == Picker::effect) {
        if (horizontal != 0) {
            state.picker_group = (state.picker_group + horizontal + 2) % 2;
            state.picker_item = std::min(state.picker_item, effect_group_size(state.picker_group) - 1);
        }
        if (vertical != 0) {
            const int effects = effect_group_size(state.picker_group);
            state.picker_item = (state.picker_item + vertical + effects) % effects;
        }
    }''')
replace_once('src/sdl_main.cpp',
'''        session.slots[static_cast<std::size_t>(state.slot)]
            .effects[static_cast<std::size_t>(parameter(state))].kind =
            kEffectKinds[static_cast<std::size_t>(state.picker_item)];''',
'''        session.slots[static_cast<std::size_t>(state.slot)]
            .effects[static_cast<std::size_t>(parameter(state))].kind =
            effect_at(state.picker_group, state.picker_item);''')

replace_between('src/sdl_main.cpp',
                'void draw_myldy_mark(SDL_Renderer* renderer, int x, int y, int scale, SDL_Color color) {',
                'SDL_Color react(SDL_Color color, float activity) noexcept {',
r'''void draw_myldy_mark(SDL_Renderer* renderer, int x, int y, int scale, SDL_Color color) {
    const int s = std::max(1, scale);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    fill(renderer, {x, y, 4 * s, 15 * s}, color);
    fill(renderer, {x + 7 * s, y, 3 * s, 15 * s}, color);
    fill(renderer, {x + 18 * s, y, 3 * s, 15 * s}, color);
    fill(renderer, {x + 24 * s, y, 4 * s, 15 * s}, color);
    for (int offset = 0; offset < 3 * s; ++offset) {
        SDL_RenderDrawLine(renderer,
            x + 9 * s + offset, y,
            x + 14 * s + offset, y + 9 * s);
        SDL_RenderDrawLine(renderer,
            x + 14 * s + offset, y + 9 * s,
            x + 19 * s + offset, y);
    }
}

SDL_Color react(SDL_Color color, float activity) noexcept {''')

replace_once('src/sdl_main.cpp',
'''    SDL_SetRenderDrawColor(renderer, 75, 67, 86, 255);
    SDL_RenderDrawLine(renderer, 32, 267, 468, 267);
    draw_myldy_mark(renderer, 32, 276, 2, kInk);
    cd::ui::draw_text(renderer, 82, 277, "DEVELOPED BY MYLDY DESIGN", kInk);
    cd::ui::draw_text(renderer, 82, 294, "@MYLDY20", kDim);
    constexpr std::string_view version{"V0.8.0"};
    cd::ui::draw_text(renderer, 468 - cd::ui::text_width(version), 277, version, kDim);
    char cpu[48]{};
    std::snprintf(cpu, sizeof(cpu), "%s: %d%%", ru(session) ? "ЗАГРУЗКА DSP" : "DSP LOAD",
        state.displayed_cpu_percent);
    cd::ui::draw_text(renderer, 468 - cd::ui::text_width(cpu), 294, cpu,
        state.displayed_cpu_percent < 75 ? kDim : kFxColors[0]);
    cd::ui::draw_text(renderer, 32, 328,''',
'''    SDL_SetRenderDrawColor(renderer, 75, 67, 86, 255);
    SDL_RenderDrawLine(renderer, 32, 267, 468, 267);
    draw_myldy_mark(renderer, 32, 275, 1, kInk);
    cd::ui::draw_text(renderer, 68, 276, "MYLDY DESIGN  @MYLDY20", kInk);
    cd::ui::draw_text(renderer, 68, 292, "DSP: DAISYSP  UI: FONT512  PORTMASTER", kDim);
    constexpr std::string_view version{"V0.9.0"};
    cd::ui::draw_text(renderer, 468 - cd::ui::text_width(version), 276, version, kDim);
    char cpu[48]{};
    std::snprintf(cpu, sizeof(cpu), "DSP %d%%", state.displayed_cpu_percent);
    cd::ui::draw_text(renderer, 468 - cd::ui::text_width(cpu), 292, cpu,
        state.displayed_cpu_percent < 75 ? kDim : kFxColors[0]);
    cd::ui::draw_text(renderer, 32, 328,''')

p = Path('src/sdl_main.cpp')
text = p.read_text(encoding='utf-8')
for old, new in [
    ('^/V', 'UP/DN'),
    ('</>', 'LT/RT'),
    ('СТРЕЛКИ ВЫБОР', 'D-PAD ВЫБОР'),
    ('ARROWS SELECT', 'D-PAD SELECT'),
]:
    text = text.replace(old, new)
p.write_text(text, encoding='utf-8')

old_effect_picker = r'''    } else if (state.picker == Picker::effect) {
        cd::ui::draw_text(renderer, 92, 66,
            ru(session) ? "ВЫБОР ЭФФЕКТА" : "CHOOSE EFFECT", kInk, 2);
        for (int item = 0; item < static_cast<int>(kEffectKinds.size()); ++item) {
            const int column = item / 5;
            const int row = item % 5;
            const int x = 92 + column * 164;
            const int y = 110 + row * 42;
            const bool selected = item == state.picker_item;
            if (selected) fill(renderer, {x, y - 5, 154, 24}, {73, 46, 104, 255});
            const std::string label = std::to_string(item + 1) + "  " +
                std::string{effect_name(kEffectKinds[static_cast<std::size_t>(item)], ru(session))};
            cd::ui::draw_text(renderer, x + 8, y, label, selected ? kInk : kDim);
        }
        cd::ui::draw_text(renderer, 92, 334,
            handheld()
                ? (ru(session) ? "СТРЕЛКИ ВЫБОР   B OK   A НАЗАД"
                               : "ARROWS SELECT   B OK   A BACK")
                : (ru(session) ? "СТРЕЛКИ ВЫБОР   E OK   ESC НАЗАД"
                               : "ARROWS SELECT   E OK   ESC BACK"), kDim);
    }'''
new_effect_picker = r'''    } else if (state.picker == Picker::effect) {
        cd::ui::draw_text(renderer, 92, 66,
            ru(session) ? "ВЫБОР ЭФФЕКТА" : "CHOOSE EFFECT", kInk, 2);
        for (int group = 0; group < 2; ++group) {
            const int x = 92 + group * 164;
            const bool selected_group = group == state.picker_group;
            if (selected_group) fill(renderer, {x - 4, 92, 154, 20}, kPurple);
            cd::ui::draw_text(renderer, x + 4, 97,
                effect_group_name(group, ru(session)), selected_group ? kInk : kDim);
        }
        const int count = effect_group_size(state.picker_group);
        const int rows = state.picker_group == 0 ? 8 : 5;
        for (int item = 0; item < count; ++item) {
            const int column = item / rows;
            const int row = item % rows;
            const int x = 92 + column * 164;
            const int y = 124 + row * 25;
            const bool selected = item == state.picker_item;
            if (selected) fill(renderer, {x, y - 4, 154, 20}, {73, 46, 104, 255});
            cd::ui::draw_text(renderer, x + 7, y,
                effect_name(effect_at(state.picker_group, item), ru(session)),
                selected ? kInk : kDim);
        }
        cd::ui::draw_text(renderer, 92, 334,
            handheld()
                ? (ru(session) ? "LT/RT ГРУППА  UP/DN ЭФФЕКТ  B OK  A НАЗАД"
                               : "LT/RT GROUP  UP/DN EFFECT  B OK  A BACK")
                : (ru(session) ? "LT/RT ГРУППА  UP/DN ЭФФЕКТ  E OK  ESC НАЗАД"
                               : "LT/RT GROUP  UP/DN EFFECT  E OK  ESC BACK"), kDim);
    }'''
replace_once('src/sdl_main.cpp', old_effect_picker, new_effect_picker)

replace_once('src/sdl_main.cpp',
'''        case cd::EffectKind::comb:
            value = std::sin(t * kTwoPi * (1.0F + effect.tone * 12.0F)) * effect.amount;
            break;
        case cd::EffectKind::bypass:
        case cd::EffectKind::delay:
            break;''',
'''        case cd::EffectKind::comb:
            value = std::sin(t * kTwoPi * (1.0F + effect.tone * 12.0F)) * effect.amount;
            break;
        case cd::EffectKind::chorus:
        case cd::EffectKind::flanger:
        case cd::EffectKind::phaser:
            value = std::sin(t * kTwoPi * (1.0F + effect.tone * 7.0F) +
                std::sin(t * kTwoPi * 2.0F) * effect.feedback * 2.0F) * effect.amount;
            break;
        case cd::EffectKind::diffuser:
            value = (std::sin(t * kTwoPi * 3.0F) + std::sin(t * kTwoPi * 7.0F) * 0.5F) *
                effect.amount * 0.55F;
            break;
        case cd::EffectKind::ahdr: {
            const float phase = std::fmod(t * (1.0F + effect.tone * 3.0F), 1.0F);
            value = (phase < 0.15F ? phase / 0.15F : std::max(0.0F, 1.0F - (phase - 0.15F) / 0.85F)) *
                effect.amount * 2.0F - 1.0F;
            break;
        }
        case cd::EffectKind::tape_void:
        case cd::EffectKind::black_hole:
        case cd::EffectKind::ritual_gate:
        case cd::EffectKind::rust_cloud:
        case cd::EffectKind::deep_sea:
            value = std::tanh((std::sin(t * kTwoPi * (1.0F + effect.tone * 6.0F)) +
                std::sin(t * kTwoPi * (3.0F + effect.feedback * 11.0F)) * 0.65F) *
                (0.5F + effect.amount * 2.5F));
            break;
        case cd::EffectKind::bypass:
        case cd::EffectKind::delay:
            break;''')

replace_once('README.md',
             'assets/branding/cursed-drone-banner.png',
             'assets/branding/cursed-drone-banner.svg')
replace_once('README.md',
             'https://img.shields.io/badge/version-0.8.0-public_test-eee2c5',
             'https://img.shields.io/badge/version-0.9.0-eee2c5')
replace_once('README.md', '`0.8.0`', '`0.9.0`')
replace_once('README.md',
'''- four serial FX slots per actor: drive, low/high-pass, tremolo, delay, crusher, wavefolder, ring modulation, comb or empty;''',
'''- four serial FX slots per actor with **basic** processors and **compound** drone/ambient recipes;
- basic FX: drive, low/high-pass, tremolo, delay, crusher, wavefolder, ring modulation, comb, chorus, flanger, phaser, diffuser and AHDR;
- compound FX: `Tape Void`, `Black Hole`, `Ritual Gate`, `Rust Cloud` and `Deep Sea`;''')
replace_once('README.md',
'''- [Third-party notices](THIRD_PARTY_NOTICES.md)''',
'''- [Effect architecture and recipes](docs/effects.en.md) · [Архитектура эффектов и рецепты](docs/effects.ru.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)''')
replace_once('README.md',
'''- четыре последовательных FX-слота на актёра: drive, low/high-pass, tremolo, delay, crusher, wavefolder, ring modulation, comb или пусто;''',
'''- четыре последовательных FX-слота на актёра с **базовыми** процессорами и **составными** рецептами для дрона/эмбиента;
- базовые FX: drive, low/high-pass, tremolo, delay, crusher, wavefolder, ring modulation, comb, chorus, flanger, phaser, diffuser и AHDR;
- составные FX: `Лентопустота`, `Чёрная дыра`, `Ритуальный гейт`, `Облако ржавчины` и `Глубина`;''')

replace_once('packaging/portmaster/curseddrone/README.md',
             '# Cursed Drone v0.8',
             '# Cursed Drone v0.9')
replace_once('packaging/portmaster/curseddrone/gameinfo.xml',
             '<version>0.8.0</version>',
             '<version>0.9.0</version>')

replace_once('tests/test_main.cpp',
'''        cd::EffectKind::crusher, cd::EffectKind::wavefolder, cd::EffectKind::ringmod,
        cd::EffectKind::comb};''',
'''        cd::EffectKind::crusher, cd::EffectKind::wavefolder, cd::EffectKind::ringmod,
        cd::EffectKind::comb, cd::EffectKind::chorus, cd::EffectKind::flanger,
        cd::EffectKind::phaser, cd::EffectKind::diffuser, cd::EffectKind::ahdr,
        cd::EffectKind::tape_void, cd::EffectKind::black_hole,
        cd::EffectKind::ritual_gate, cd::EffectKind::rust_cloud, cd::EffectKind::deep_sea};''')
replace_once('tests/test_main.cpp',
             'expect(loaded.schema_version == 7, "session should upgrade to schema 7");',
             'expect(loaded.schema_version == 8, "session should upgrade to schema 8");')

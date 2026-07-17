#!/usr/bin/env python3
from pathlib import Path
import re


def read(path: str) -> str:
    return Path(path).read_text(encoding="utf-8")


def write(path: str, text: str) -> None:
    Path(path).write_text(text, encoding="utf-8")


def replace(path: str, old: str, new: str, expected: int = 1) -> None:
    text = read(path)
    count = text.count(old)
    if count != expected:
        raise RuntimeError(f"{path}: expected {expected} copies, found {count}: {old[:80]!r}")
    write(path, text.replace(old, new))


def sub(path: str, pattern: str, replacement: str, expected: int = 1) -> None:
    text = read(path)
    changed, count = re.subn(pattern, replacement, text, flags=re.S)
    if count != expected:
        raise RuntimeError(f"{path}: regex expected {expected} matches, found {count}: {pattern[:80]!r}")
    write(path, changed)


# Version and public model.
replace("CMakeLists.txt", "project(cursed_drone VERSION 0.6.0 LANGUAGES CXX)",
        "project(cursed_drone VERSION 0.7.0 LANGUAGES CXX)")
replace("include/cursed_drone/session.hpp",
        "enum class SceneKind { derelict, factory, wasteland, wet_cave, metro, nursery };",
        "enum class SceneKind { derelict, factory, wasteland, wet_cave, metro, nursery, bunker, power_grid, deep_water, ash_field };")
replace("include/cursed_drone/session.hpp",
        "    toy_gears,\n    lullaby\n};",
        "    toy_gears,\n    lullaby,\n    sub_drone,\n    tape_drone,\n    bowed_metal,\n    earth_rumble\n};")
replace("include/cursed_drone/session.hpp", "    unsigned schema_version{6};", "    unsigned schema_version{7};")
replace("include/cursed_drone/audio.hpp", "inline constexpr std::size_t kScopePointCount = 64;",
        "inline constexpr std::size_t kScopePointCount = 32;")

# Four deliberately low, continuous engines.
replace("src/soundscape.hpp",
        "    float lullaby(float frequency, float timbre, float color, float motion,\n        float texture, float tempo_bpm, float activity, float tension, float evolution) noexcept;",
        "    float lullaby(float frequency, float timbre, float color, float motion,\n        float texture, float tempo_bpm, float activity, float tension, float evolution) noexcept;\n"
        "    float sub_drone(float frequency, float timbre, float color, float motion,\n"
        "        float texture, float activity, float tension, float evolution) noexcept;\n"
        "    float tape_drone(float frequency, float timbre, float color, float motion,\n"
        "        float texture, float activity, float tension, float evolution) noexcept;\n"
        "    float bowed_metal(float frequency, float timbre, float color, float motion,\n"
        "        float texture, float activity, float tension, float evolution) noexcept;\n"
        "    float earth_rumble(float frequency, float timbre, float color, float motion,\n"
        "        float texture, float activity, float tension, float evolution) noexcept;")
replace("src/soundscape.cpp",
        "    case EngineKind::lullaby:\n        return lullaby(frequency, timbre, color, motion, texture, tempo_bpm, activity, tension, evolution);\n    case EngineKind::diagnostic:",
        "    case EngineKind::lullaby:\n        return lullaby(frequency, timbre, color, motion, texture, tempo_bpm, activity, tension, evolution);\n"
        "    case EngineKind::sub_drone:\n        return sub_drone(frequency, timbre, color, motion, texture, activity, tension, evolution);\n"
        "    case EngineKind::tape_drone:\n        return tape_drone(frequency, timbre, color, motion, texture, activity, tension, evolution);\n"
        "    case EngineKind::bowed_metal:\n        return bowed_metal(frequency, timbre, color, motion, texture, activity, tension, evolution);\n"
        "    case EngineKind::earth_rumble:\n        return earth_rumble(frequency, timbre, color, motion, texture, activity, tension, evolution);\n"
        "    case EngineKind::diagnostic:")

new_engines = r'''
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
'''
replace("src/soundscape.cpp", "\n} // namespace cursed_drone::detail\n", "\n" + new_engines + "\n} // namespace cursed_drone::detail\n")

# Reduce the accidental flute/whistle bias without deleting those actors.
replace("src/soundscape.cpp",
        "    const float howl = sine(0U, howl_frequency) * std::pow(gust, 3.2F) * (0.015F + tension * 0.16F);",
        "    const float howl = sine(0U, howl_frequency) * std::pow(gust, 3.2F) * (0.006F + tension * 0.055F);")
replace("src/soundscape.cpp",
        "    const float base = std::clamp(frequency, 300.0F, 2'400.0F) * (0.72F + color * 1.35F);",
        "    const float base = std::clamp(frequency, 220.0F, 1'600.0F) * (0.72F + color * 1.15F);")
replace("src/soundscape.cpp",
        "    return (carrier * 0.62F + syrinx * (0.12F + color * 0.18F) + grain) * envelope * 0.72F;",
        "    return (carrier * 0.54F + syrinx * (0.08F + color * 0.12F) + grain) * envelope * 0.40F;")
replace("src/soundscape.cpp",
        "    const float base = std::clamp(frequency, 500.0F, 4'800.0F) * (0.72F + tension * 0.62F);",
        "    const float base = std::clamp(frequency, 260.0F, 2'800.0F) * (0.72F + tension * 0.48F);")
replace("src/soundscape.cpp",
        "    const float scratch = filters_[0].Band() * texture * gate_b * 0.22F;\n    return (insect_a * gate_a * 0.34F + insect_b * gate_b * 0.18F + scratch) *",
        "    const float scratch = filters_[0].Band() * texture * gate_b * 0.12F;\n    return (insect_a * gate_a * 0.18F + insect_b * gate_b * 0.09F + scratch) *")
replace("src/soundscape.cpp",
        "        std::pow(2.0F, rise * (0.7F + tension * 1.8F));",
        "        std::pow(2.0F, rise * (0.42F + tension * 1.05F));")
replace("src/soundscape.cpp",
        "    return (tone * 0.72F + overtone * (0.06F + color * 0.16F) + grit) * primary_envelope_;",
        "    return (tone * 0.50F + overtone * (0.035F + color * 0.09F) + grit) * primary_envelope_;")
replace("src/soundscape.cpp",
        "    const float whine = sine(3U, inverter) * (0.025F + color * 0.10F) * std::pow(run, 1.7F);",
        "    const float whine = sine(3U, inverter) * (0.010F + color * 0.045F) * std::pow(run, 1.7F);")
replace("src/soundscape.cpp",
        "        primary_envelope_ * 0.18F;",
        "        primary_envelope_ * 0.08F;", expected=1)

# Route and macro-map the new engines.
replace("src/audio.cpp",
        "        case EngineKind::toy_gears:\n        case EngineKind::lullaby:\n            return soundscape_.next(",
        "        case EngineKind::toy_gears:\n        case EngineKind::lullaby:\n        case EngineKind::sub_drone:\n        case EngineKind::tape_drone:\n        case EngineKind::bowed_metal:\n        case EngineKind::earth_rumble:\n            return soundscape_.next(")
replace("src/audio.cpp",
        "                case EngineKind::footsteps:\n                case EngineKind::machinery:",
        "                case EngineKind::sub_drone:\n"
        "                case EngineKind::tape_drone:\n"
        "                case EngineKind::bowed_metal:\n"
        "                case EngineKind::earth_rumble:\n"
        "                    parameters.timbre += texture_macro * 0.12F;\n"
        "                    parameters.color += texture_macro * 0.10F;\n"
        "                    parameters.motion += events_macro * 0.12F + chaos_macro * 0.08F;\n"
        "                    parameters.texture += texture_macro * 0.30F;\n"
        "                    break;\n"
        "                case EngineKind::footsteps:\n                case EngineKind::machinery:")
replace("src/audio.cpp",
        "                case EngineKind::lullaby: pulse_ratio = 0.25F; pulse_depth = 0.0F; break;\n                case EngineKind::diagnostic: break;",
        "                case EngineKind::lullaby: pulse_ratio = 0.25F; pulse_depth = 0.0F; break;\n"
        "                case EngineKind::sub_drone: pulse_ratio = 0.125F; pulse_depth = 0.025F; break;\n"
        "                case EngineKind::tape_drone: pulse_ratio = 0.17F; pulse_depth = 0.035F; break;\n"
        "                case EngineKind::bowed_metal: pulse_ratio = 0.21F; pulse_depth = 0.04F; break;\n"
        "                case EngineKind::earth_rumble: pulse_ratio = 0.10F; pulse_depth = 0.02F; break;\n"
        "                case EngineKind::diagnostic: break;")

# Scope telemetry is visual, not audio. Capture it at ~24 Hz and halve its width.
replace("src/audio.cpp",
        "        pulse_phase_ = 0.0F;\n        chaos_phase_ = 0.0F;",
        "        pulse_phase_ = 0.0F;\n        chaos_phase_ = 0.0F;\n        scope_publish_counter_ = 0U;")
replace("src/audio.cpp",
        "        std::array<std::array<float, kScopePointCount>, kSlotCount> scope_frames{};\n        std::array<float, kScopePointCount> master_scope_frame{};",
        "        std::array<std::array<float, kScopePointCount>, kSlotCount> scope_frames{};\n"
        "        std::array<float, kScopePointCount> master_scope_frame{};\n"
        "        const bool capture_scope = ++scope_publish_counter_ >= 4U;\n"
        "        if (capture_scope) scope_publish_counter_ = 0U;")
replace("src/audio.cpp",
        "            const std::size_t scope_index = std::min(\n                kScopePointCount - 1U, frame_index * kScopePointCount / output.size());",
        "            const std::size_t scope_index = capture_scope\n"
        "                ? std::min(kScopePointCount - 1U, frame_index * kScopePointCount / output.size())\n"
        "                : 0U;")
replace("src/audio.cpp",
        "                scope_frames[slot_index][scope_index] = 0.5F * (slot_frame.left + slot_frame.right);",
        "                if (capture_scope) {\n"
        "                    scope_frames[slot_index][scope_index] = 0.5F * (slot_frame.left + slot_frame.right);\n"
        "                }")
replace("src/audio.cpp",
        "            master_scope_frame[scope_index] = 0.5F * (frame.left + frame.right);",
        "            if (capture_scope) master_scope_frame[scope_index] = 0.5F * (frame.left + frame.right);")
replace("src/audio.cpp",
        "            for (std::size_t point = 0; point < kScopePointCount; ++point) {\n                slot_scope_[index][point].store(scope_frames[index][point], std::memory_order_relaxed);\n            }",
        "            if (capture_scope) {\n"
        "                for (std::size_t point = 0; point < kScopePointCount; ++point) {\n"
        "                    slot_scope_[index][point].store(scope_frames[index][point], std::memory_order_relaxed);\n"
        "                }\n"
        "            }")
replace("src/audio.cpp",
        "        for (std::size_t point = 0; point < kScopePointCount; ++point) {\n            master_scope_[point].store(master_scope_frame[point], std::memory_order_relaxed);\n        }",
        "        if (capture_scope) {\n"
        "            for (std::size_t point = 0; point < kScopePointCount; ++point) {\n"
        "                master_scope_[point].store(master_scope_frame[point], std::memory_order_relaxed);\n"
        "            }\n"
        "        }")
replace("src/audio.cpp",
        "    float chaos_phase_{0.0F};\n    std::uint32_t chaos_random_state_",
        "    float chaos_phase_{0.0F};\n    unsigned scope_publish_counter_{0U};\n    std::uint32_t chaos_random_state_")

# Session names, engines, ten balanced presets and schema migration.
replace("src/session.cpp",
        "    std::pair{SceneKind::nursery, std::string_view{\"nursery\"}},\n};",
        "    std::pair{SceneKind::nursery, std::string_view{\"nursery\"}},\n"
        "    std::pair{SceneKind::bunker, std::string_view{\"bunker\"}},\n"
        "    std::pair{SceneKind::power_grid, std::string_view{\"power_grid\"}},\n"
        "    std::pair{SceneKind::deep_water, std::string_view{\"deep_water\"}},\n"
        "    std::pair{SceneKind::ash_field, std::string_view{\"ash_field\"}},\n};")
replace("src/session.cpp",
        "    std::pair{EngineKind::lullaby, std::string_view{\"lullaby\"}},\n};",
        "    std::pair{EngineKind::lullaby, std::string_view{\"lullaby\"}},\n"
        "    std::pair{EngineKind::sub_drone, std::string_view{\"sub_drone\"}},\n"
        "    std::pair{EngineKind::tape_drone, std::string_view{\"tape_drone\"}},\n"
        "    std::pair{EngineKind::bowed_metal, std::string_view{\"bowed_metal\"}},\n"
        "    std::pair{EngineKind::earth_rumble, std::string_view{\"earth_rumble\"}},\n};")

scene_switch = r'''    switch (scene) {
    case SceneKind::derelict:
        set_actor(0U, EngineKind::derelict_bed, 34.0F, 0.20F, 0.24F, 0.14F, 0.17F, 0.34F, -0.12F);
        set_actor(1U, EngineKind::footsteps, 48.0F, 0.46F, 0.28F, 0.25F, 0.36F, 0.26F, -0.30F);
        set_actor(2U, EngineKind::door, 72.0F, 0.42F, 0.30F, 0.18F, 0.34F, 0.18F, 0.24F);
        set_actor(3U, EngineKind::pipe, 54.0F, 0.44F, 0.26F, 0.19F, 0.31F, 0.17F, 0.44F);
        break;
    case SceneKind::factory:
        set_actor(0U, EngineKind::motor, 30.0F, 0.40F, 0.32F, 0.42F, 0.30F, 0.34F, -0.42F);
        set_actor(1U, EngineKind::machinery, 44.0F, 0.42F, 0.44F, 0.46F, 0.38F, 0.27F, 0.28F);
        set_actor(2U, EngineKind::tape_drone, 38.0F, 0.36F, 0.26F, 0.20F, 0.24F, 0.24F, -0.08F);
        set_actor(3U, EngineKind::metal, 61.0F, 0.56F, 0.48F, 0.20F, 0.46F, 0.18F, 0.48F);
        break;
    case SceneKind::wasteland:
        set_actor(0U, EngineKind::earth_rumble, 22.0F, 0.32F, 0.20F, 0.18F, 0.28F, 0.30F, -0.10F);
        set_actor(1U, EngineKind::wind, 38.0F, 0.32F, 0.34F, 0.28F, 0.29F, 0.24F, 0.34F);
        set_actor(2U, EngineKind::insects, 620.0F, 0.38F, 0.42F, 0.34F, 0.30F, 0.075F, -0.52F);
        set_actor(3U, EngineKind::signal, 58.0F, 0.28F, 0.30F, 0.18F, 0.24F, 0.12F, 0.22F);
        break;
    case SceneKind::wet_cave:
        set_actor(0U, EngineKind::earth_rumble, 20.0F, 0.34F, 0.22F, 0.12F, 0.30F, 0.28F, -0.18F);
        set_actor(1U, EngineKind::water_drip, 320.0F, 0.44F, 0.48F, 0.17F, 0.25F, 0.16F, 0.38F);
        set_actor(2U, EngineKind::water_flow, 72.0F, 0.38F, 0.40F, 0.28F, 0.44F, 0.25F, -0.42F);
        set_actor(3U, EngineKind::stone, 48.0F, 0.54F, 0.28F, 0.14F, 0.34F, 0.16F, 0.46F);
        break;
    case SceneKind::metro:
        set_actor(0U, EngineKind::tape_drone, 31.0F, 0.36F, 0.22F, 0.24F, 0.24F, 0.31F, -0.18F);
        set_actor(1U, EngineKind::rail_joint, 58.0F, 0.52F, 0.42F, 0.48F, 0.34F, 0.27F, 0.34F);
        set_actor(2U, EngineKind::brake, 125.0F, 0.46F, 0.48F, 0.26F, 0.40F, 0.10F, -0.44F);
        set_actor(3U, EngineKind::carriage, 42.0F, 0.34F, 0.34F, 0.38F, 0.42F, 0.27F, 0.42F);
        break;
    case SceneKind::nursery:
        set_actor(0U, EngineKind::tape_drone, 32.0F, 0.42F, 0.20F, 0.20F, 0.34F, 0.28F, -0.22F);
        set_actor(1U, EngineKind::music_box, 178.0F, 0.48F, 0.52F, 0.17F, 0.20F, 0.15F, 0.32F);
        set_actor(2U, EngineKind::toy_gears, 31.0F, 0.40F, 0.24F, 0.34F, 0.38F, 0.17F, -0.46F);
        set_actor(3U, EngineKind::lullaby, 196.0F, 0.38F, 0.46F, 0.14F, 0.22F, 0.11F, 0.44F);
        break;
    case SceneKind::bunker:
        set_actor(0U, EngineKind::sub_drone, 27.5F, 0.42F, 0.18F, 0.12F, 0.26F, 0.36F, -0.12F);
        set_actor(1U, EngineKind::derelict_bed, 43.0F, 0.28F, 0.24F, 0.16F, 0.22F, 0.24F, 0.18F);
        set_actor(2U, EngineKind::pipe, 49.0F, 0.38F, 0.22F, 0.17F, 0.24F, 0.13F, -0.42F);
        set_actor(3U, EngineKind::machinery, 36.0F, 0.44F, 0.34F, 0.28F, 0.30F, 0.17F, 0.46F);
        break;
    case SceneKind::power_grid:
        set_actor(0U, EngineKind::tape_drone, 30.0F, 0.48F, 0.24F, 0.22F, 0.28F, 0.34F, -0.18F);
        set_actor(1U, EngineKind::motor, 42.0F, 0.36F, 0.30F, 0.30F, 0.25F, 0.23F, 0.26F);
        set_actor(2U, EngineKind::bowed_metal, 52.0F, 0.54F, 0.38F, 0.24F, 0.40F, 0.18F, -0.44F);
        set_actor(3U, EngineKind::earth_rumble, 18.0F, 0.34F, 0.20F, 0.16F, 0.32F, 0.26F, 0.42F);
        break;
    case SceneKind::deep_water:
        set_actor(0U, EngineKind::earth_rumble, 16.0F, 0.46F, 0.18F, 0.12F, 0.42F, 0.34F, -0.18F);
        set_actor(1U, EngineKind::water_flow, 54.0F, 0.34F, 0.28F, 0.24F, 0.52F, 0.22F, 0.32F);
        set_actor(2U, EngineKind::cave_air, 24.0F, 0.30F, 0.20F, 0.14F, 0.38F, 0.24F, -0.40F);
        set_actor(3U, EngineKind::bowed_metal, 38.0F, 0.48F, 0.26F, 0.18F, 0.34F, 0.14F, 0.44F);
        break;
    case SceneKind::ash_field:
        set_actor(0U, EngineKind::sub_drone, 24.0F, 0.38F, 0.14F, 0.16F, 0.32F, 0.34F, -0.16F);
        set_actor(1U, EngineKind::wind, 32.0F, 0.28F, 0.24F, 0.22F, 0.42F, 0.20F, 0.30F);
        set_actor(2U, EngineKind::bowed_metal, 44.0F, 0.50F, 0.30F, 0.20F, 0.36F, 0.14F, -0.46F);
        set_actor(3U, EngineKind::signal, 46.0F, 0.24F, 0.26F, 0.14F, 0.18F, 0.08F, 0.42F);
        break;
    }
'''
sub("src/session.cpp", r"    switch \(scene\) \{.*?\n    \}\n\n    for \(auto& slot : session\.slots\)",
    scene_switch + "\n    for (auto& slot : session.slots)")

# Conservative defaults: preserve weight and distance, avoid global toy/lo-fi coloration.
replace("src/session.cpp",
        "        slot.effects[0] = {EffectKind::lowpass, 0.08F, 0.72F, 0.0F};\n"
        "        slot.effects[1] = {EffectKind::drive, 0.025F, 0.50F, 0.0F};\n"
        "        slot.effects[2] = {EffectKind::delay, 0.08F, 0.18F, 0.22F};",
        "        slot.effects[0] = {EffectKind::lowpass, 0.16F, 0.52F, 0.0F};\n"
        "        slot.effects[1] = {EffectKind::drive, 0.012F, 0.44F, 0.0F};\n"
        "        slot.effects[2] = {EffectKind::delay, 0.055F, 0.20F, 0.18F};")
replace("src/session.cpp", "    session.slots[0].effects[2].amount = 0.04F;",
        "    session.slots[0].effects[2].amount = 0.025F;")
replace("src/session.cpp", "    session.slots[2].effects[2] = {EffectKind::delay, 0.16F, 0.24F, 0.32F};",
        "    session.slots[2].effects[2] = {EffectKind::delay, 0.10F, 0.24F, 0.26F};")
replace("src/session.cpp", "    session.slots[3].effects[2] = {EffectKind::delay, 0.13F, 0.34F, 0.36F};",
        "    session.slots[3].effects[2] = {EffectKind::delay, 0.09F, 0.34F, 0.28F};")
replace("src/session.cpp",
        "    } else if (scene == SceneKind::nursery) {\n"
        "        session.slots[0].effects[2] = {EffectKind::delay, 0.27F, 0.48F, 0.52F};\n"
        "        session.slots[1].effects[1] = {EffectKind::ringmod, 0.16F, 0.23F, 0.0F};\n"
        "        session.slots[2].effects[1] = {EffectKind::crusher, 0.18F, 0.34F, 0.0F};\n"
        "        session.slots[3].effects[2] = {EffectKind::comb, 0.14F, 0.52F, 0.38F};\n"
        "    }",
        "    } else if (scene == SceneKind::nursery) {\n"
        "        session.slots[0].effects[0] = {EffectKind::lowpass, 0.28F, 0.34F, 0.0F};\n"
        "        session.slots[1].effects[2] = {EffectKind::delay, 0.18F, 0.46F, 0.42F};\n"
        "        session.slots[2].effects[1] = {EffectKind::drive, 0.035F, 0.34F, 0.0F};\n"
        "        session.slots[3].effects[2] = {EffectKind::comb, 0.08F, 0.48F, 0.24F};\n"
        "    } else if (scene == SceneKind::bunker) {\n"
        "        session.slots[0].effects[0] = {EffectKind::lowpass, 0.30F, 0.30F, 0.0F};\n"
        "        session.slots[1].effects[2] = {EffectKind::delay, 0.08F, 0.62F, 0.38F};\n"
        "    } else if (scene == SceneKind::power_grid) {\n"
        "        session.slots[0].effects[1] = {EffectKind::drive, 0.035F, 0.44F, 0.0F};\n"
        "        session.slots[2].effects[2] = {EffectKind::comb, 0.08F, 0.28F, 0.22F};\n"
        "    } else if (scene == SceneKind::deep_water) {\n"
        "        session.slots[0].effects[0] = {EffectKind::lowpass, 0.38F, 0.24F, 0.0F};\n"
        "        session.slots[1].effects[0] = {EffectKind::lowpass, 0.22F, 0.42F, 0.0F};\n"
        "        session.slots[2].effects[2] = {EffectKind::delay, 0.14F, 0.70F, 0.48F};\n"
        "    } else if (scene == SceneKind::ash_field) {\n"
        "        session.slots[0].effects[0] = {EffectKind::lowpass, 0.32F, 0.28F, 0.0F};\n"
        "        session.slots[1].effects[2] = {EffectKind::delay, 0.11F, 0.66F, 0.40F};\n"
        "    }")
replace("src/session.cpp",
        "            schema->second != \"4\" && schema->second != \"5\" && schema->second != \"6\")) {",
        "            schema->second != \"4\" && schema->second != \"5\" && schema->second != \"6\" &&\n"
        "            schema->second != \"7\")) {")
replace("src/session.cpp",
        "    if (schema->second != \"4\" && schema->second != \"5\" && schema->second != \"6\") {",
        "    if (schema->second != \"4\" && schema->second != \"5\" && schema->second != \"6\" &&\n"
        "        schema->second != \"7\") {")
replace("src/session.cpp", "    loaded.schema_version = 6;", "    loaded.schema_version = 7;")

# Handheld-aware interface, stable output meter and reliable exit chord.
replace("src/sdl_main.cpp", "#include <cstdio>\n#include <filesystem>", "#include <cstdio>\n#include <cstdlib>\n#include <filesystem>")
replace("src/sdl_main.cpp",
        "    int displayed_cpu_percent{0};\n    Uint32 cpu_display_updated_at{0};",
        "    int displayed_cpu_percent{0};\n    Uint32 cpu_display_updated_at{0};\n"
        "    bool start_held{false};\n    bool select_held{false};")
replace("src/sdl_main.cpp",
        "bool ru(const cd::Session& session) noexcept { return session.locale == cd::Locale::ru; }",
        "bool ru(const cd::Session& session) noexcept { return session.locale == cd::Locale::ru; }\n\n"
        "bool handheld() noexcept {\n"
        "    const char* value = std::getenv(\"CURSED_DRONE_HANDHELD\");\n"
        "    return value != nullptr && value[0] != '\\0' && value[0] != '0';\n"
        "}")
replace("src/sdl_main.cpp",
        "    case cd::SceneKind::nursery: return russian ? \"СЛОМАННАЯ ДЕТСКАЯ\" : \"BROKEN NURSERY\";\n    }",
        "    case cd::SceneKind::nursery: return russian ? \"СЛОМАННАЯ ДЕТСКАЯ\" : \"BROKEN NURSERY\";\n"
        "    case cd::SceneKind::bunker: return russian ? \"БУНКЕР\" : \"BUNKER\";\n"
        "    case cd::SceneKind::power_grid: return russian ? \"ПОДСТАНЦИЯ\" : \"POWER GRID\";\n"
        "    case cd::SceneKind::deep_water: return russian ? \"ГЛУБИНА\" : \"DEEP WATER\";\n"
        "    case cd::SceneKind::ash_field: return russian ? \"ПЕПЕЛ\" : \"ASH FIELD\";\n    }")
replace("src/sdl_main.cpp",
        "    case cd::EngineKind::lullaby: return russian ? \"КОЛЫБЕЛЬ\" : \"LULLABY\";\n    }",
        "    case cd::EngineKind::lullaby: return russian ? \"КОЛЫБЕЛЬ\" : \"LULLABY\";\n"
        "    case cd::EngineKind::sub_drone: return russian ? \"САБ-ДРОН\" : \"SUB DRONE\";\n"
        "    case cd::EngineKind::tape_drone: return russian ? \"ЛЕНТА\" : \"TAPE DRONE\";\n"
        "    case cd::EngineKind::bowed_metal: return russian ? \"СМЫЧОК\" : \"BOWED METAL\";\n"
        "    case cd::EngineKind::earth_rumble: return russian ? \"ГУД ЗЕМЛИ\" : \"EARTH RUMBLE\";\n    }")
replace("src/sdl_main.cpp",
        "    case cd::EngineKind::lullaby: {\n        constexpr std::array<std::string_view, 4> r{\"СТРУНА\", \"СТЕКЛО\", \"ТЕМП\", \"РАСПАД\"};\n        constexpr std::array<std::string_view, 4> e{\"STRING\", \"GLASS\", \"PACE\", \"DECAY\"};\n        return (russian ? r : e)[static_cast<std::size_t>(character)];\n    }\n    case cd::EngineKind::diagnostic:",
        "    case cd::EngineKind::lullaby: {\n        constexpr std::array<std::string_view, 4> r{\"СТРУНА\", \"СТЕКЛО\", \"ТЕМП\", \"РАСПАД\"};\n        constexpr std::array<std::string_view, 4> e{\"STRING\", \"GLASS\", \"PACE\", \"DECAY\"};\n        return (russian ? r : e)[static_cast<std::size_t>(character)];\n    }\n"
        "    case cd::EngineKind::sub_drone: {\n        constexpr std::array<std::string_view, 4> r{\"ТЕЛО\", \"ОБЕРТОН\", \"ДЫХАНИЕ\", \"ВОЗДУХ\"};\n        constexpr std::array<std::string_view, 4> e{\"BODY\", \"OVERTONE\", \"BREATH\", \"AIR\"};\n        return (russian ? r : e)[static_cast<std::size_t>(character)];\n    }\n"
        "    case cd::EngineKind::tape_drone: {\n        constexpr std::array<std::string_view, 4> r{\"ЛЕНТА\", \"ТОН\", \"ПЛАВАНИЕ\", \"ИЗНОС\"};\n        constexpr std::array<std::string_view, 4> e{\"TAPE\", \"TONE\", \"WOW\", \"WEAR\"};\n        return (russian ? r : e)[static_cast<std::size_t>(character)];\n    }\n"
        "    case cd::EngineKind::bowed_metal: {\n        constexpr std::array<std::string_view, 4> r{\"КОРПУС\", \"МОДЫ\", \"СМЫЧОК\", \"ТРЕНИЕ\"};\n        constexpr std::array<std::string_view, 4> e{\"BODY\", \"MODES\", \"BOW\", \"FRICTION\"};\n        return (russian ? r : e)[static_cast<std::size_t>(character)];\n    }\n"
        "    case cd::EngineKind::earth_rumble: {\n        constexpr std::array<std::string_view, 4> r{\"МАССА\", \"ПОРОДА\", \"СДВИГ\", \"УДАРЫ\"};\n        constexpr std::array<std::string_view, 4> e{\"MASS\", \"GROUND\", \"HEAVE\", \"IMPACTS\"};\n        return (russian ? r : e)[static_cast<std::size_t>(character)];\n    }\n"
        "    case cd::EngineKind::diagnostic:")
replace("src/sdl_main.cpp",
        "constexpr std::array<std::array<cd::EngineKind, 4>, 7> kEngineGroups{{",
        "constexpr std::array<std::array<cd::EngineKind, 4>, 8> kEngineGroups{{")
replace("src/sdl_main.cpp",
        "    {cd::EngineKind::music_box, cd::EngineKind::toy_voice, cd::EngineKind::toy_gears, cd::EngineKind::lullaby},\n}};",
        "    {cd::EngineKind::music_box, cd::EngineKind::toy_voice, cd::EngineKind::toy_gears, cd::EngineKind::lullaby},\n"
        "    {cd::EngineKind::sub_drone, cd::EngineKind::tape_drone, cd::EngineKind::bowed_metal, cd::EngineKind::earth_rumble},\n}};")
replace("src/sdl_main.cpp",
        "constexpr std::array<cd::SceneKind, 6> kScenes{\n    cd::SceneKind::derelict, cd::SceneKind::factory, cd::SceneKind::wasteland,\n    cd::SceneKind::wet_cave, cd::SceneKind::metro, cd::SceneKind::nursery};",
        "constexpr std::array<cd::SceneKind, 10> kScenes{\n"
        "    cd::SceneKind::derelict, cd::SceneKind::factory, cd::SceneKind::wasteland,\n"
        "    cd::SceneKind::wet_cave, cd::SceneKind::metro, cd::SceneKind::nursery,\n"
        "    cd::SceneKind::bunker, cd::SceneKind::power_grid, cd::SceneKind::deep_water,\n"
        "    cd::SceneKind::ash_field};")
replace("src/sdl_main.cpp",
        "    constexpr std::array<std::string_view, 7> r{\n        \"ОБЩИЕ\", \"ЗАБРОШЕННОЕ\", \"ЦЕХ\", \"ПУСТОШЬ\", \"ПЕЩЕРА\", \"МЕТРО\", \"ДЕТСКАЯ\"};\n    constexpr std::array<std::string_view, 7> e{\n        \"GENERAL\", \"DERELICT\", \"FACTORY\", \"WASTELAND\", \"CAVE\", \"METRO\", \"NURSERY\"};",
        "    constexpr std::array<std::string_view, 8> r{\n        \"ОБЩИЕ\", \"ЗАБРОШЕННОЕ\", \"ЦЕХ\", \"ПУСТОШЬ\", \"ПЕЩЕРА\", \"МЕТРО\", \"ДЕТСКАЯ\", \"ДРОНЫ\"};\n"
        "    constexpr std::array<std::string_view, 8> e{\n        \"GENERAL\", \"DERELICT\", \"FACTORY\", \"WASTELAND\", \"CAVE\", \"METRO\", \"NURSERY\", \"DRONES\"};")
replace("src/sdl_main.cpp", "%.1f Hz", "%.1f HZ")
replace("src/sdl_main.cpp", "%.2f s", "%.2f S", expected=2)

meter_helper = r'''
void segmented_output_meter(SDL_Renderer* renderer, int x, int y, int width, int height,
    float rms, float peak) {
    constexpr int segments = 10;
    constexpr int gap = 2;
    const int segment_width = (width - gap * (segments - 1)) / segments;
    const float level = std::clamp(rms * 4.0F, 0.0F, 1.0F);
    const float peak_level = std::clamp(peak * 1.8F, 0.0F, 1.0F);
    for (int index = 0; index < segments; ++index) {
        const float threshold = static_cast<float>(index + 1) / static_cast<float>(segments);
        SDL_Color color = threshold > 0.82F ? SDL_Color{225, 77, 96, 255}
            : (threshold > 0.62F ? SDL_Color{224, 154, 63, 255} : SDL_Color{80, 169, 154, 255});
        if (level < threshold) color = {38, 33, 48, 255};
        fill(renderer, {x + index * (segment_width + gap), y, segment_width, height}, color);
        if (peak_level >= threshold && peak_level < threshold + 0.1F) {
            outline(renderer, {x + index * (segment_width + gap), y, segment_width, height}, kInk);
        }
    }
}

'''
replace("src/sdl_main.cpp", "void draw_header(\n", meter_helper + "void draw_header(\n")
sub("src/sdl_main.cpp", r"void draw_header\(.*?\n\}\n\nstd::string_view macro_endpoint",
r'''void draw_header(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state) {
    cd::ui::draw_text(renderer, 10, 5, cd::tr(session.locale, cd::TextId::app_name), kInk, 2);
    std::string status;
    SDL_Color status_color = kDim;
    if (state.auto_fade) {
        status = std::string{ru(session) ? "ФЕЙД " : "FADE "} +
            (state.fade_target > session.performance.fade ? "IN " : "OUT ") +
            std::to_string(static_cast<int>(std::lround(session.performance.fade * 100.0F))) + "%";
        status_color = {91, 218, 179, 255};
    } else {
        status = "DSP " + std::to_string(state.displayed_cpu_percent) + "%";
        if (state.displayed_cpu_percent >= 75) status_color = kFxColors[0];
    }
    const int meter_width = 80;
    const int meter_x = kWidth - 10 - meter_width;
    cd::ui::draw_text(renderer, meter_x - 12 - cd::ui::text_width(status), 9, status, status_color);
    segmented_output_meter(renderer, meter_x, 8, meter_width, 9,
        telemetry.master_rms, telemetry.master_peak);
    constexpr std::array<Page, 5> pages{
        Page::perform, Page::slot, Page::effects, Page::master, Page::setup};
    for (int index = 0; index < 5; ++index) {
        const int x = 10 + index * 99;
        const bool selected = pages[static_cast<std::size_t>(index)] == state.page;
        if (selected) {
            fill(renderer, {x, 26, 95, 16}, kPurple);
        } else {
            outline(renderer, {x, 26, 95, 16}, {58, 49, 72, 255});
        }
        cd::ui::draw_text(renderer, x + 4, 30, page_name(pages[static_cast<std::size_t>(index)], ru(session)),
            selected ? kInk : kDim);
    }
}

std::string_view macro_endpoint''')
replace("src/sdl_main.cpp",
        "    cd::ui::draw_text(renderer, 22, 250,\n        ru(session) ? \"ДОРОЖКИ  </> ВЫБОР  A/D УРОВЕНЬ  SPACE MUTE\" :\n                      \"TRACKS  </> SELECT  A/D LEVEL  SPACE MUTE\",\n        state.scene_track_focus ? kInk : kDim);",
        "    const char* track_help = handheld()\n"
        "        ? (ru(session) ? \"ДОРОЖКИ  </> ВЫБОР  L/R УРОВЕНЬ  B MUTE\"\n"
        "                       : \"TRACKS  </> SELECT  L/R LEVEL  B MUTE\")\n"
        "        : (ru(session) ? \"ДОРОЖКИ  </> ВЫБОР  A/D УРОВЕНЬ  SPACE MUTE\"\n"
        "                       : \"TRACKS  </> SELECT  A/D LEVEL  SPACE MUTE\");\n"
        "    cd::ui::draw_text(renderer, 22, 250, track_help,\n"
        "        state.scene_track_focus ? kInk : kDim);")

# Ten scenes need a two-column picker.
sub("src/sdl_main.cpp", r"        for \(int item = 0; item < static_cast<int>\(kScenes\.size\(\)\); \+\+item\) \{.*?\n        \}\n        cd::ui::draw_text\(renderer, 92, 334,",
r'''        for (int item = 0; item < static_cast<int>(kScenes.size()); ++item) {
            const int column = item / 5;
            const int row = item % 5;
            const int x = 92 + column * 164;
            const int y = 110 + row * 42;
            const bool selected = item == state.picker_item;
            if (selected) fill(renderer, {x, y - 5, 154, 24}, {73, 46, 104, 255});
            const std::string label = std::to_string(item + 1) + "  " +
                std::string{scene_name(kScenes[static_cast<std::size_t>(item)], ru(session))};
            cd::ui::draw_text(renderer, x + 8, y, label, selected ? kInk : kDim);
        }
        cd::ui::draw_text(renderer, 92, 334,''')

# Handheld instructions everywhere, while retaining keyboard help on desktop.
sub("src/sdl_main.cpp", r"    std::string help;\n    if \(state\.picker != Picker::none\) \{.*?\n    \}\n    cd::ui::draw_text\(renderer, 10, 370, help, kDim\);",
r'''    std::string help;
    if (handheld()) {
        if (state.picker != Picker::none) {
            help = ru(session) ? "B ВЫБРАТЬ  A НАЗАД" : "B SELECT  A BACK";
        } else if (state.page == Page::effects) {
            help = ru(session) ? "</> FX  ^/V ПАРАМ.  Y ДОРОЖКА  L/R ЗНАЧ.  START ТИП"
                               : "</> FX  ^/V PARAM  Y TRACK  L/R VALUE  START TYPE";
        } else if (state.page == Page::perform) {
            help = ru(session) ? "^/V РУЧКА/УРОВ.  </> ДОРОЖКА  B MUTE  START ЛАНДШАФТ"
                               : "^/V MACRO/LEVEL  </> TRACK  B MUTE  START LANDSCAPE";
        } else if (state.page == Page::master || state.page == Page::setup) {
            help = ru(session) ? "^/V ПАРАМ.  L/R ЗНАЧ.  X ЭКРАН  SELECT ФЕЙД  A KILL"
                               : "^/V PARAM  L/R VALUE  X PAGE  SELECT FADE  A KILL";
        } else {
            help = ru(session) ? "</> СЛОТ  ^/V ПАРАМ.  L/R ЗНАЧ.  START ВЫБОР  B MUTE"
                               : "</> SLOT  ^/V PARAM  L/R VALUE  START CHOOSE  B MUTE";
        }
    } else if (state.picker != Picker::none) {
        help = ru(session) ? "E ВЫБРАТЬ  ESC НАЗАД" : "E SELECT  ESC BACK";
    } else if (state.page == Page::effects) {
        help = ru(session) ? "</> FX  ^/V ПАРАМ.  S ДОРОЖКА  A/D ЗНАЧ.  E ТИП"
                           : "</> FX  ^/V PARAM  S TRACK  A/D VALUE  E TYPE";
    } else if (state.page == Page::perform) {
        help = ru(session) ? "^/V РУЧКА/УРОВ.  </> ДОРОЖКА  SPACE MUTE  E ЛАНДШАФТ"
                           : "^/V MACRO/LEVEL  </> TRACK  SPACE MUTE  E LANDSCAPE";
    } else if (state.page == Page::master || state.page == Page::setup) {
        help = ru(session) ? "^/V ПАРАМ.  A/D ЗНАЧ.  TAB ЭКРАН  F ФЕЙД  K KILL"
                           : "^/V PARAM  A/D VALUE  TAB PAGE  F FADE  K KILL";
    } else {
        help = ru(session) ? "</> СЛОТ  ^/V ПАРАМ.  A/D ЗНАЧ.  E ВЫБОР  SPACE MUTE"
                           : "</> SLOT  ^/V PARAM  A/D VALUE  E CHOOSE  SPACE MUTE";
    }
    cd::ui::draw_text(renderer, 10, 370, help, kDim);''')
replace("src/sdl_main.cpp",
        "    std::snprintf(next_fade, sizeof(next_fade), \"%s F: %s %.1fS\",\n        ru(session) ? \"КНОПКА\" : \"BUTTON\",",
        "    const char* fade_button = handheld() ? \"SELECT\" : \"F\";\n"
        "    std::snprintf(next_fade, sizeof(next_fade), \"%s %s: %s %.1fS\",\n"
        "        ru(session) ? \"КНОПКА\" : \"BUTTON\", fade_button,")
replace("src/sdl_main.cpp",
        "        ru(session) ? \"A/D: ИЗМЕНИТЬ / НАСТРОЙКИ СОХРАНЯЮТСЯ АВТОМАТИЧЕСКИ\"\n                    : \"A/D: CHANGE / SETTINGS ARE SAVED AUTOMATICALLY\",",
        "        handheld()\n"
        "            ? (ru(session) ? \"L/R: ИЗМЕНИТЬ / АВТОСОХРАНЕНИЕ\" : \"L/R: CHANGE / AUTOSAVE\")\n"
        "            : (ru(session) ? \"A/D: ИЗМЕНИТЬ / НАСТРОЙКИ СОХРАНЯЮТСЯ АВТОМАТИЧЕСКИ\"\n"
        "                           : \"A/D: CHANGE / SETTINGS ARE SAVED AUTOMATICALLY\"),")

# Start+Select exits in either press order. On the tested Brick this means hold Start, press Select.
replace("src/sdl_main.cpp",
        "            } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {\n                if (state.picker != Picker::none) {",
        "            } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {\n"
        "                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START) state.start_held = true;\n"
        "                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) state.select_held = true;\n"
        "                if (state.start_held && state.select_held) {\n"
        "                    running = false;\n"
        "                    continue;\n"
        "                }\n"
        "                if (state.picker != Picker::none) {")
replace("src/sdl_main.cpp",
        "            } else if (event.type == SDL_CONTROLLERBUTTONUP) {\n                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER) stop_adjust(state, -1);\n                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) stop_adjust(state, 1);",
        "            } else if (event.type == SDL_CONTROLLERBUTTONUP) {\n"
        "                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START) state.start_held = false;\n"
        "                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) state.select_held = false;\n"
        "                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER) stop_adjust(state, -1);\n"
        "                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) stop_adjust(state, 1);")
replace("src/sdl_main.cpp", "        SDL_Delay(8);", "        SDL_Delay(16);")

# Tell the app that PortMaster is using the physical Brick labels.
replace("packaging/portmaster/curseddrone/Cursed Drone.sh",
        "export CURSED_DRONE_DATA_DIR=\"$CONFDIR\"",
        "export CURSED_DRONE_DATA_DIR=\"$CONFDIR\"\nexport CURSED_DRONE_HANDHELD=1")

# Render and validate all ten landscapes.
replace("tools/render_demo.cpp",
        "            std::cerr << \"Invalid scene (use derelict, factory, wasteland, wet_cave, metro or nursery)\\n\";",
        "            std::cerr << \"Invalid scene (derelict, factory, wasteland, wet_cave, metro, nursery, bunker, power_grid, deep_water, ash_field)\\n\";")
replace("tests/test_main.cpp",
        "        cd::SceneKind::wet_cave, cd::SceneKind::metro, cd::SceneKind::nursery};",
        "        cd::SceneKind::wet_cave, cd::SceneKind::metro, cd::SceneKind::nursery,\n"
        "        cd::SceneKind::bunker, cd::SceneKind::power_grid, cd::SceneKind::deep_water,\n"
        "        cd::SceneKind::ash_field};")
replace("tests/test_main.cpp", "expect(loaded.schema_version == 6, \"session should upgrade to schema 6\");",
        "expect(loaded.schema_version == 7, \"session should upgrade to schema 7\");")
replace(".github/workflows/build.yml",
        "          build/cursed-drone-render previews/broken-nursery.wav 30 nursery",
        "          build/cursed-drone-render previews/broken-nursery.wav 30 nursery\n"
        "          build/cursed-drone-render previews/bunker.wav 30 bunker\n"
        "          build/cursed-drone-render previews/power-grid.wav 30 power_grid\n"
        "          build/cursed-drone-render previews/deep-water.wav 30 deep_water\n"
        "          build/cursed-drone-render previews/ash-field.wav 30 ash_field")

# Shareable README: production status, real Brick controls and the expanded palette.
replace("README.md", "experimental `0.6.0` development version. Six procedural landscapes and twenty-eight selectable engines",
        "public test `0.7.0` version. Ten procedural landscapes and thirty-two selectable engines")
replace("README.md", "six landscape recipes: `Derelict`, `Factory`, `Wasteland`, `Wet Cave`, `Metro Car` and `Broken Nursery`;",
        "ten landscape recipes, including four bass-first scenes: `Bunker`, `Power Grid`, `Deep Water` and `Ash Field`;")
replace("README.md", "twenty-four procedural actors with independent gesture, material and timing models;",
        "twenty-eight procedural actors, including four continuous low-frequency drone engines;")
replace("README.md", "| exit | Escape | Menu / OS action |",
        "| exit | Escape | hold Start, then press Select |")
replace("README.md",
        "| `Broken Nursery` | music-box tines, bent toy voice, gears and sparse decaying lullaby notes | intimate, fragile, deliberately uncanny |",
        "| `Broken Nursery` | tape bed, quieter music box, gears and sparse decaying lullaby notes | intimate and uncanny without dominating the whole instrument |\n"
        "| `Bunker` | sub drone, room pressure, pipe resonance and distant machinery | heavy enclosed low end with sparse movement |\n"
        "| `Power Grid` | tape drone, motor, bowed metal and ground rumble | electrical mass, transformer vibration and restrained metal |\n"
        "| `Deep Water` | earth rumble, water flow, cave air and bowed body | slow pressure, depth and very little high-frequency ornament |\n"
        "| `Ash Field` | sub drone, low wind, bowed metal and distant signal | wide, dry and bass-led with rare foreground events |")
replace("README.md", "экспериментальная версия `0.6.0` в разработке. Уже работают шесть процедурных ландшафтов и двадцать восемь выбираемых движков",
        "публичная тестовая версия `0.7.0`. Работают десять процедурных ландшафтов и тридцать два выбираемых движка")
replace("README.md", "шесть рецептов: `Заброшенное`, `Цех`, `Пустошь`, `Мокрая пещера`, `Вагон метро` и `Сломанная детская`;",
        "десять рецептов, включая четыре басовых сцены: `Бункер`, `Подстанция`, `Глубина` и `Пепел`;")
replace("README.md", "двадцать четыре процедурных актёра с независимыми моделями жеста, материала и времени;",
        "двадцать восемь процедурных актёров, включая четыре непрерывных низкочастотных дроновых движка;")
replace("README.md", "| выход | Escape | Menu / системная кнопка |",
        "| выход | Escape | удерживать Start, затем нажать Select |")

# Remove the one-shot machinery from the resulting source commit.
Path("tools/apply_v07_patch.py").unlink()
Path(".github/workflows/apply-v07.yml").unlink()
print("v0.7 patch applied")

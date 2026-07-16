// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>

namespace cursed_drone {

inline constexpr std::size_t kSlotCount = 4;
inline constexpr std::size_t kEffectsPerSlot = 4;
inline constexpr std::size_t kModulatorsPerSlot = 4;

enum class Locale { ru, en };
enum class SceneKind { derelict, factory, wasteland };
enum class EngineKind {
    diagnostic,
    macro,
    body,
    grain,
    particle,
    derelict_bed,
    footsteps,
    door,
    pipe,
    motor,
    machinery,
    crowd,
    metal,
    wind,
    birds,
    insects,
    signal
};
enum class EffectKind { bypass, drive, lowpass, tremolo, delay, crusher };
enum class ModWave { sine, triangle, sample_hold, random_walk };
enum class ModDestination {
    pitch,
    timbre,
    color,
    motion,
    texture,
    level,
    pan,
    fx1,
    fx2,
    fx3,
    fx4
};

struct EffectSettings {
    EffectKind kind{EffectKind::bypass};
    float amount{0.0F};
    float tone{0.5F};
    float feedback{0.0F};
};

struct ModSettings {
    bool enabled{false};
    ModWave wave{ModWave::sine};
    ModDestination destination{ModDestination::timbre};
    float rate_hz{0.1F};
    float depth{0.0F};
    float offset{0.0F};
};

struct SlotSettings {
    bool enabled{true};
    EngineKind engine{EngineKind::diagnostic};
    float frequency_hz{55.0F};
    float timbre{0.5F};
    float color{0.5F};
    float motion{0.25F};
    float texture{0.25F};
    float level{0.35F};
    float pan{0.0F};
    std::array<EffectSettings, kEffectsPerSlot> effects{};
    std::array<ModSettings, kModulatorsPerSlot> modulators{};
};

struct PerformanceSettings {
    // Global, non-destructive macros layered over the detailed slot settings.
    float texture{0.22F};
    float pulse{0.12F};
    float chaos{0.08F};
    float space{0.20F};
    float events{0.30F};
    float fade{1.0F};
};

struct Session {
    unsigned schema_version{4};
    Locale locale{Locale::ru};
    SceneKind scene{SceneKind::derelict};
    float tempo_bpm{45.0F};
    float master_level{0.80F};
    float fade_in_seconds{4.0F};
    float fade_out_seconds{4.0F};
    PerformanceSettings performance{};
    std::array<SlotSettings, kSlotCount> slots{};
};

[[nodiscard]] Session make_default_session();
void apply_scene_recipe(Session& session, SceneKind scene);
[[nodiscard]] bool save_session(const Session& session, const std::filesystem::path& path, std::string& error);
[[nodiscard]] bool load_session(const std::filesystem::path& path, Session& session, std::string& error);

[[nodiscard]] std::string to_string(Locale value);
[[nodiscard]] std::string to_string(SceneKind value);
[[nodiscard]] std::string to_string(EngineKind value);
[[nodiscard]] std::string to_string(EffectKind value);
[[nodiscard]] std::string to_string(ModWave value);
[[nodiscard]] std::string to_string(ModDestination value);

[[nodiscard]] bool parse_locale(const std::string& text, Locale& value);
[[nodiscard]] bool parse_scene(const std::string& text, SceneKind& value);

} // namespace cursed_drone

// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.
#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>

namespace cursed_drone {

inline constexpr std::size_t kSlotCount = 4;
inline constexpr std::size_t kEffectsPerSlot = 4;
inline constexpr std::size_t kModulatorsPerSlot = 4;
inline constexpr std::size_t kMasterEffects = 4;
inline constexpr std::size_t kMemorySlots = 8;
inline constexpr std::size_t kScaleDegreeCount = 24;
inline constexpr std::size_t kScaleNameLength = 32;


enum class Locale { ru, en };
enum class SceneKind { derelict, factory, wasteland, wet_cave, metro, nursery, bunker, power_grid, deep_water, ash_field };
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
    signal,
    cave_air,
    water_drip,
    water_flow,
    stone,
    metro_traction,
    rail_joint,
    brake,
    carriage,
    music_box,
    toy_voice,
    toy_gears,
    lullaby,
    sub_drone,
    tape_drone,
    bowed_metal,
    earth_rumble,
    plaits
};
enum class EffectKind {
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
    deep_sea,
    granular_reverse
};
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
enum class PlaitsModel {
    virtual_analog_vcf,
    phase_distortion,
    wave_terrain,
    string_machine,
    chiptune,
    virtual_analog,
    waveshaping,
    fm,
    grain,
    additive,
    wavetable,
    chord,
    swarm,
    noise,
    string,
    modal
};
enum class PlaitsOutputMode { main, aux, mix, stereo };

struct ScalaTuning {
    bool enabled{true};
    int root_midi{36};
    int degree_count{12};
    float period_cents{1200.0F};
    std::array<float, kScaleDegreeCount> cents{
        100.0F, 200.0F, 300.0F, 400.0F, 500.0F, 600.0F,
        700.0F, 800.0F, 900.0F, 1000.0F, 1100.0F, 1200.0F};
    std::array<char, kScaleNameLength> name{
        '1', '2', '-', 'T', 'E', 'T', '\0'};
};

struct EuclideanSettings {
    bool enabled{false};
    int steps{16};
    int pulses{5};
    int rotation{0};
    float probability{1.0F};
};

struct EffectSettings {
    EffectKind kind{EffectKind::bypass};
    float amount{0.0F};
    float tone{0.5F};
    float feedback{0.0F};
    // Bypass is an empty slot. enabled independently bypasses a configured
    // effect without destroying its kind or parameter values.
    bool enabled{true};
};

struct ModSettings {
    bool enabled{false};
    ModWave wave{ModWave::sine};
    ModDestination destination{ModDestination::timbre};
    float rate_hz{0.1F};
    float depth{0.0F};
    float offset{0.0F};
    int rate_mod_source{-1};
    float rate_mod_amount{0.0F};
};

struct SlotSettings {
    bool enabled{true};
    EngineKind engine{EngineKind::diagnostic};
    float frequency_hz{55.0F};
    float timbre{0.5F};
    float color{0.5F};
    float motion{0.25F};
    float texture{0.25F};
    float event_density{0.50F};
    float level{0.35F};
    float pan{0.0F};
    PlaitsModel plaits_model{PlaitsModel::virtual_analog};
    PlaitsOutputMode plaits_output{PlaitsOutputMode::stereo};
    ScalaTuning tuning{};
    EuclideanSettings euclidean{};
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
    SceneKind morph_target{SceneKind::derelict};
    float morph{0.0F};
};

struct Session {
    unsigned schema_version{12};
    Locale locale{Locale::en};
    SceneKind scene{SceneKind::derelict};
    bool scene_modified{false};
    float tempo_bpm{45.0F};
    float master_level{0.80F};
    float fade_in_seconds{4.0F};
    float fade_out_seconds{4.0F};
    PerformanceSettings performance{};
    std::array<SlotSettings, kSlotCount> slots{};
    std::array<EffectSettings, kMasterEffects> master_effects{};
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
[[nodiscard]] std::string to_string(PlaitsModel value);
[[nodiscard]] std::string to_string(PlaitsOutputMode value);
[[nodiscard]] bool supports_manual_trigger(EngineKind value) noexcept;
[[nodiscard]] bool supports_event_rate(EngineKind value) noexcept;
[[nodiscard]] float effective_event_density(float actor_density, float global_events) noexcept;
[[nodiscard]] float event_rate_hz(float tempo_bpm, float event_density, float motion) noexcept;
[[nodiscard]] float event_max_wait_seconds(float tempo_bpm, float event_density, float motion) noexcept;

[[nodiscard]] bool parse_locale(const std::string& text, Locale& value);
[[nodiscard]] bool parse_scene(const std::string& text, SceneKind& value);

} // namespace cursed_drone

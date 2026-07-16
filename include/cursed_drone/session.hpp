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
enum class EngineKind { diagnostic, macro, body, grain, particle };
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

struct Session {
    unsigned schema_version{1};
    Locale locale{Locale::ru};
    float tempo_bpm{60.0F};
    float master_level{0.75F};
    std::array<SlotSettings, kSlotCount> slots{};
};

[[nodiscard]] Session make_default_session();
[[nodiscard]] bool save_session(const Session& session, const std::filesystem::path& path, std::string& error);
[[nodiscard]] bool load_session(const std::filesystem::path& path, Session& session, std::string& error);

[[nodiscard]] std::string to_string(Locale value);
[[nodiscard]] std::string to_string(EngineKind value);
[[nodiscard]] std::string to_string(EffectKind value);
[[nodiscard]] std::string to_string(ModWave value);
[[nodiscard]] std::string to_string(ModDestination value);

[[nodiscard]] bool parse_locale(const std::string& text, Locale& value);

} // namespace cursed_drone

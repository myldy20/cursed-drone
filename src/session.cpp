// SPDX-License-Identifier: GPL-3.0-or-later
#include "cursed_drone/session.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <unordered_map>

namespace cursed_drone {
namespace {

template <typename Enum, std::size_t Size>
std::string enum_name(Enum value, const std::array<std::pair<Enum, std::string_view>, Size>& names) {
    for (const auto& [candidate, name] : names) {
        if (candidate == value) {
            return std::string{name};
        }
    }
    return "unknown";
}

template <typename Enum, std::size_t Size>
bool parse_enum(
    const std::string& text,
    Enum& value,
    const std::array<std::pair<Enum, std::string_view>, Size>& names) {
    for (const auto& [candidate, name] : names) {
        if (name == text) {
            value = candidate;
            return true;
        }
    }
    return false;
}

constexpr std::array kLocales{
    std::pair{Locale::ru, std::string_view{"ru"}},
    std::pair{Locale::en, std::string_view{"en"}},
};

constexpr std::array kEngines{
    std::pair{EngineKind::diagnostic, std::string_view{"diagnostic"}},
    std::pair{EngineKind::macro, std::string_view{"macro"}},
    std::pair{EngineKind::body, std::string_view{"body"}},
    std::pair{EngineKind::grain, std::string_view{"grain"}},
    std::pair{EngineKind::particle, std::string_view{"particle"}},
};

constexpr std::array kEffects{
    std::pair{EffectKind::bypass, std::string_view{"bypass"}},
    std::pair{EffectKind::drive, std::string_view{"drive"}},
    std::pair{EffectKind::lowpass, std::string_view{"lowpass"}},
    std::pair{EffectKind::tremolo, std::string_view{"tremolo"}},
    std::pair{EffectKind::delay, std::string_view{"delay"}},
    std::pair{EffectKind::crusher, std::string_view{"crusher"}},
};

constexpr std::array kWaves{
    std::pair{ModWave::sine, std::string_view{"sine"}},
    std::pair{ModWave::triangle, std::string_view{"triangle"}},
    std::pair{ModWave::sample_hold, std::string_view{"sample_hold"}},
    std::pair{ModWave::random_walk, std::string_view{"random_walk"}},
};

constexpr std::array kDestinations{
    std::pair{ModDestination::pitch, std::string_view{"pitch"}},
    std::pair{ModDestination::timbre, std::string_view{"timbre"}},
    std::pair{ModDestination::color, std::string_view{"color"}},
    std::pair{ModDestination::motion, std::string_view{"motion"}},
    std::pair{ModDestination::texture, std::string_view{"texture"}},
    std::pair{ModDestination::level, std::string_view{"level"}},
    std::pair{ModDestination::pan, std::string_view{"pan"}},
    std::pair{ModDestination::fx1, std::string_view{"fx1"}},
    std::pair{ModDestination::fx2, std::string_view{"fx2"}},
    std::pair{ModDestination::fx3, std::string_view{"fx3"}},
    std::pair{ModDestination::fx4, std::string_view{"fx4"}},
};

std::string key(std::size_t slot, std::string_view field) {
    return "slot." + std::to_string(slot) + "." + std::string{field};
}

std::string effect_key(std::size_t slot, std::size_t effect, std::string_view field) {
    return "slot." + std::to_string(slot) + ".effect." + std::to_string(effect) + "." + std::string{field};
}

std::string mod_key(std::size_t slot, std::size_t mod, std::string_view field) {
    return "slot." + std::to_string(slot) + ".mod." + std::to_string(mod) + "." + std::string{field};
}

bool parse_float(const std::unordered_map<std::string, std::string>& values, const std::string& name, float& value) {
    const auto found = values.find(name);
    if (found == values.end()) {
        return true;
    }
    try {
        std::size_t consumed = 0;
        const float parsed = std::stof(found->second, &consumed);
        if (consumed != found->second.size()) {
            return false;
        }
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool parse_bool(const std::unordered_map<std::string, std::string>& values, const std::string& name, bool& value) {
    const auto found = values.find(name);
    if (found == values.end()) {
        return true;
    }
    if (found->second == "1" || found->second == "true") {
        value = true;
        return true;
    }
    if (found->second == "0" || found->second == "false") {
        value = false;
        return true;
    }
    return false;
}

template <typename Enum, std::size_t Size>
bool parse_enum_value(
    const std::unordered_map<std::string, std::string>& values,
    const std::string& name,
    Enum& value,
    const std::array<std::pair<Enum, std::string_view>, Size>& names) {
    const auto found = values.find(name);
    return found == values.end() || parse_enum(found->second, value, names);
}

} // namespace

Session make_default_session() {
    Session session{};
    constexpr std::array frequencies{32.70F, 48.99F, 73.42F, 110.0F};
    constexpr std::array pans{-0.55F, -0.18F, 0.18F, 0.55F};

    for (std::size_t index = 0; index < kSlotCount; ++index) {
        auto& slot = session.slots[index];
        slot.frequency_hz = frequencies[index];
        slot.pan = pans[index];
        slot.timbre = 0.25F + static_cast<float>(index) * 0.13F;
        slot.color = 0.18F + static_cast<float>(index) * 0.17F;
        slot.motion = 0.18F + static_cast<float>(index) * 0.08F;
        slot.texture = 0.12F + static_cast<float>(index) * 0.10F;
        slot.level = 0.23F;

        slot.effects[0] = {EffectKind::drive, 0.12F + 0.05F * static_cast<float>(index), 0.5F, 0.0F};
        slot.effects[1] = {EffectKind::lowpass, 0.35F, 0.28F + 0.10F * static_cast<float>(index), 0.0F};
        slot.effects[2] = {EffectKind::tremolo, 0.16F, 0.08F + 0.04F * static_cast<float>(index), 0.0F};
        slot.effects[3] = {EffectKind::delay, 0.20F, 0.30F + 0.10F * static_cast<float>(index), 0.42F};

        slot.modulators[0] = {true, ModWave::sine, ModDestination::timbre, 0.027F + 0.011F * static_cast<float>(index), 0.18F, 0.0F};
        slot.modulators[1] = {true, ModWave::random_walk, ModDestination::pan, 0.07F + 0.02F * static_cast<float>(index), 0.16F, 0.0F};
        slot.modulators[2] = {true, ModWave::triangle, ModDestination::fx3, 0.11F + 0.03F * static_cast<float>(index), 0.12F, 0.0F};
        slot.modulators[3] = {true, ModWave::sample_hold, ModDestination::texture, 0.13F + 0.05F * static_cast<float>(index), 0.10F, 0.0F};
    }
    return session;
}

bool save_session(const Session& session, const std::filesystem::path& path, std::string& error) {
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        error = "cannot open session for writing: " + path.string();
        return false;
    }

    output << std::setprecision(9);
    output << "cursed_drone_session=" << session.schema_version << '\n';
    output << "locale=" << to_string(session.locale) << '\n';
    output << "tempo_bpm=" << session.tempo_bpm << '\n';
    output << "master_level=" << session.master_level << '\n';
    for (std::size_t slot_index = 0; slot_index < kSlotCount; ++slot_index) {
        const auto& slot = session.slots[slot_index];
        output << key(slot_index, "enabled") << '=' << (slot.enabled ? 1 : 0) << '\n';
        output << key(slot_index, "engine") << '=' << to_string(slot.engine) << '\n';
        output << key(slot_index, "frequency_hz") << '=' << slot.frequency_hz << '\n';
        output << key(slot_index, "timbre") << '=' << slot.timbre << '\n';
        output << key(slot_index, "color") << '=' << slot.color << '\n';
        output << key(slot_index, "motion") << '=' << slot.motion << '\n';
        output << key(slot_index, "texture") << '=' << slot.texture << '\n';
        output << key(slot_index, "level") << '=' << slot.level << '\n';
        output << key(slot_index, "pan") << '=' << slot.pan << '\n';
        for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {
            const auto& effect = slot.effects[effect_index];
            output << effect_key(slot_index, effect_index, "kind") << '=' << to_string(effect.kind) << '\n';
            output << effect_key(slot_index, effect_index, "amount") << '=' << effect.amount << '\n';
            output << effect_key(slot_index, effect_index, "tone") << '=' << effect.tone << '\n';
            output << effect_key(slot_index, effect_index, "feedback") << '=' << effect.feedback << '\n';
        }
        for (std::size_t mod_index = 0; mod_index < kModulatorsPerSlot; ++mod_index) {
            const auto& mod = slot.modulators[mod_index];
            output << mod_key(slot_index, mod_index, "enabled") << '=' << (mod.enabled ? 1 : 0) << '\n';
            output << mod_key(slot_index, mod_index, "wave") << '=' << to_string(mod.wave) << '\n';
            output << mod_key(slot_index, mod_index, "destination") << '=' << to_string(mod.destination) << '\n';
            output << mod_key(slot_index, mod_index, "rate_hz") << '=' << mod.rate_hz << '\n';
            output << mod_key(slot_index, mod_index, "depth") << '=' << mod.depth << '\n';
            output << mod_key(slot_index, mod_index, "offset") << '=' << mod.offset << '\n';
        }
    }
    if (!output) {
        error = "failed while writing session: " + path.string();
        return false;
    }
    return true;
}

bool load_session(const std::filesystem::path& path, Session& session, std::string& error) {
    std::ifstream input(path);
    if (!input) {
        error = "cannot open session: " + path.string();
        return false;
    }

    std::unordered_map<std::string, std::string> values;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (line.empty() || line.front() == '#') {
            continue;
        }
        const auto separator = line.find('=');
        if (separator == std::string::npos || separator == 0U) {
            error = "invalid session line " + std::to_string(line_number);
            return false;
        }
        values[line.substr(0U, separator)] = line.substr(separator + 1U);
    }

    const auto schema = values.find("cursed_drone_session");
    if (schema == values.end() || schema->second != "1") {
        error = "unsupported or missing session schema";
        return false;
    }

    Session loaded = make_default_session();
    const auto locale = values.find("locale");
    if (locale != values.end() && !parse_locale(locale->second, loaded.locale)) {
        error = "invalid locale";
        return false;
    }
    if (!parse_float(values, "tempo_bpm", loaded.tempo_bpm) ||
        !parse_float(values, "master_level", loaded.master_level)) {
        error = "invalid master value";
        return false;
    }

    for (std::size_t slot_index = 0; slot_index < kSlotCount; ++slot_index) {
        auto& slot = loaded.slots[slot_index];
        if (!parse_bool(values, key(slot_index, "enabled"), slot.enabled) ||
            !parse_enum_value(values, key(slot_index, "engine"), slot.engine, kEngines) ||
            !parse_float(values, key(slot_index, "frequency_hz"), slot.frequency_hz) ||
            !parse_float(values, key(slot_index, "timbre"), slot.timbre) ||
            !parse_float(values, key(slot_index, "color"), slot.color) ||
            !parse_float(values, key(slot_index, "motion"), slot.motion) ||
            !parse_float(values, key(slot_index, "texture"), slot.texture) ||
            !parse_float(values, key(slot_index, "level"), slot.level) ||
            !parse_float(values, key(slot_index, "pan"), slot.pan)) {
            error = "invalid value in slot " + std::to_string(slot_index + 1U);
            return false;
        }
        for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {
            auto& effect = slot.effects[effect_index];
            if (!parse_enum_value(values, effect_key(slot_index, effect_index, "kind"), effect.kind, kEffects) ||
                !parse_float(values, effect_key(slot_index, effect_index, "amount"), effect.amount) ||
                !parse_float(values, effect_key(slot_index, effect_index, "tone"), effect.tone) ||
                !parse_float(values, effect_key(slot_index, effect_index, "feedback"), effect.feedback)) {
                error = "invalid effect in slot " + std::to_string(slot_index + 1U);
                return false;
            }
        }
        for (std::size_t mod_index = 0; mod_index < kModulatorsPerSlot; ++mod_index) {
            auto& mod = slot.modulators[mod_index];
            if (!parse_bool(values, mod_key(slot_index, mod_index, "enabled"), mod.enabled) ||
                !parse_enum_value(values, mod_key(slot_index, mod_index, "wave"), mod.wave, kWaves) ||
                !parse_enum_value(values, mod_key(slot_index, mod_index, "destination"), mod.destination, kDestinations) ||
                !parse_float(values, mod_key(slot_index, mod_index, "rate_hz"), mod.rate_hz) ||
                !parse_float(values, mod_key(slot_index, mod_index, "depth"), mod.depth) ||
                !parse_float(values, mod_key(slot_index, mod_index, "offset"), mod.offset)) {
                error = "invalid modulator in slot " + std::to_string(slot_index + 1U);
                return false;
            }
        }
    }

    loaded.tempo_bpm = std::clamp(loaded.tempo_bpm, 10.0F, 300.0F);
    loaded.master_level = std::clamp(loaded.master_level, 0.0F, 1.0F);
    session = loaded;
    return true;
}

std::string to_string(Locale value) { return enum_name(value, kLocales); }
std::string to_string(EngineKind value) { return enum_name(value, kEngines); }
std::string to_string(EffectKind value) { return enum_name(value, kEffects); }
std::string to_string(ModWave value) { return enum_name(value, kWaves); }
std::string to_string(ModDestination value) { return enum_name(value, kDestinations); }

bool parse_locale(const std::string& text, Locale& value) { return parse_enum(text, value, kLocales); }

} // namespace cursed_drone

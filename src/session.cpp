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

constexpr std::array kScenes{
    std::pair{SceneKind::derelict, std::string_view{"derelict"}},
    std::pair{SceneKind::factory, std::string_view{"factory"}},
    std::pair{SceneKind::wasteland, std::string_view{"wasteland"}},
    std::pair{SceneKind::wet_cave, std::string_view{"wet_cave"}},
    std::pair{SceneKind::metro, std::string_view{"metro"}},
    std::pair{SceneKind::nursery, std::string_view{"nursery"}},
    std::pair{SceneKind::bunker, std::string_view{"bunker"}},
    std::pair{SceneKind::power_grid, std::string_view{"power_grid"}},
    std::pair{SceneKind::deep_water, std::string_view{"deep_water"}},
    std::pair{SceneKind::ash_field, std::string_view{"ash_field"}},
};

constexpr std::array kEngines{
    std::pair{EngineKind::diagnostic, std::string_view{"diagnostic"}},
    std::pair{EngineKind::macro, std::string_view{"macro"}},
    std::pair{EngineKind::body, std::string_view{"body"}},
    std::pair{EngineKind::grain, std::string_view{"grain"}},
    std::pair{EngineKind::particle, std::string_view{"particle"}},
    std::pair{EngineKind::derelict_bed, std::string_view{"derelict_bed"}},
    std::pair{EngineKind::footsteps, std::string_view{"footsteps"}},
    std::pair{EngineKind::door, std::string_view{"door"}},
    std::pair{EngineKind::pipe, std::string_view{"pipe"}},
    std::pair{EngineKind::motor, std::string_view{"motor"}},
    std::pair{EngineKind::machinery, std::string_view{"machinery"}},
    std::pair{EngineKind::crowd, std::string_view{"crowd"}},
    std::pair{EngineKind::metal, std::string_view{"metal"}},
    std::pair{EngineKind::wind, std::string_view{"wind"}},
    std::pair{EngineKind::birds, std::string_view{"birds"}},
    std::pair{EngineKind::insects, std::string_view{"insects"}},
    std::pair{EngineKind::signal, std::string_view{"signal"}},
    std::pair{EngineKind::cave_air, std::string_view{"cave_air"}},
    std::pair{EngineKind::water_drip, std::string_view{"water_drip"}},
    std::pair{EngineKind::water_flow, std::string_view{"water_flow"}},
    std::pair{EngineKind::stone, std::string_view{"stone"}},
    std::pair{EngineKind::metro_traction, std::string_view{"metro_traction"}},
    std::pair{EngineKind::rail_joint, std::string_view{"rail_joint"}},
    std::pair{EngineKind::brake, std::string_view{"brake"}},
    std::pair{EngineKind::carriage, std::string_view{"carriage"}},
    std::pair{EngineKind::music_box, std::string_view{"music_box"}},
    std::pair{EngineKind::toy_voice, std::string_view{"toy_voice"}},
    std::pair{EngineKind::toy_gears, std::string_view{"toy_gears"}},
    std::pair{EngineKind::lullaby, std::string_view{"lullaby"}},
    std::pair{EngineKind::sub_drone, std::string_view{"sub_drone"}},
    std::pair{EngineKind::tape_drone, std::string_view{"tape_drone"}},
    std::pair{EngineKind::bowed_metal, std::string_view{"bowed_metal"}},
    std::pair{EngineKind::earth_rumble, std::string_view{"earth_rumble"}},
};

constexpr std::array kEffects{
    std::pair{EffectKind::bypass, std::string_view{"bypass"}},
    std::pair{EffectKind::drive, std::string_view{"drive"}},
    std::pair{EffectKind::lowpass, std::string_view{"lowpass"}},
    std::pair{EffectKind::highpass, std::string_view{"highpass"}},
    std::pair{EffectKind::tremolo, std::string_view{"tremolo"}},
    std::pair{EffectKind::delay, std::string_view{"delay"}},
    std::pair{EffectKind::crusher, std::string_view{"crusher"}},
    std::pair{EffectKind::wavefolder, std::string_view{"wavefolder"}},
    std::pair{EffectKind::ringmod, std::string_view{"ringmod"}},
    std::pair{EffectKind::comb, std::string_view{"comb"}},
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
    constexpr std::array engines{
        EngineKind::macro, EngineKind::body, EngineKind::grain, EngineKind::particle};
    constexpr std::array frequencies{64.60F, 48.99F, 129.20F, 96.90F};
    constexpr std::array pans{-0.55F, -0.18F, 0.18F, 0.55F};
    constexpr std::array levels{0.30F, 0.38F, 0.25F, 0.32F};

    for (std::size_t index = 0; index < kSlotCount; ++index) {
        auto& slot = session.slots[index];
        slot.engine = engines[index];
        slot.frequency_hz = frequencies[index];
        slot.pan = pans[index];
        slot.timbre = 0.25F + static_cast<float>(index) * 0.13F;
        slot.color = 0.18F + static_cast<float>(index) * 0.17F;
        slot.motion = 0.18F + static_cast<float>(index) * 0.08F;
        slot.texture = 0.12F + static_cast<float>(index) * 0.10F;
        slot.level = levels[index];

        slot.modulators[0] = {true, ModWave::sine, ModDestination::timbre, 0.027F + 0.011F * static_cast<float>(index), 0.18F, 0.0F};
        slot.modulators[1] = {true, ModWave::random_walk, ModDestination::pan, 0.07F + 0.02F * static_cast<float>(index), 0.16F, 0.0F};
        slot.modulators[2] = {true, ModWave::triangle, ModDestination::fx3, 0.11F + 0.03F * static_cast<float>(index), 0.12F, 0.0F};
        slot.modulators[3] = {true, ModWave::sample_hold, ModDestination::texture, 0.13F + 0.05F * static_cast<float>(index), 0.10F, 0.0F};
    }

    session.slots[0].effects = {{
        {EffectKind::lowpass, 0.20F, 0.48F, 0.0F},
        {EffectKind::drive, 0.08F, 0.50F, 0.0F},
        {EffectKind::tremolo, 0.06F, 0.18F, 0.0F},
        {EffectKind::delay, 0.14F, 0.28F, 0.34F},
    }};
    session.slots[1].effects = {{
        {EffectKind::drive, 0.11F, 0.45F, 0.0F},
        {EffectKind::lowpass, 0.18F, 0.56F, 0.0F},
        {EffectKind::delay, 0.16F, 0.36F, 0.38F},
        {EffectKind::bypass, 0.0F, 0.50F, 0.0F},
    }};
    session.slots[2].effects = {{
        {EffectKind::lowpass, 0.16F, 0.64F, 0.0F},
        {EffectKind::crusher, 0.08F, 0.24F, 0.0F},
        {EffectKind::tremolo, 0.10F, 0.31F, 0.0F},
        {EffectKind::delay, 0.23F, 0.44F, 0.46F},
    }};
    session.slots[3].effects = {{
        {EffectKind::crusher, 0.10F, 0.18F, 0.0F},
        {EffectKind::lowpass, 0.22F, 0.52F, 0.0F},
        {EffectKind::delay, 0.18F, 0.30F, 0.42F},
        {EffectKind::bypass, 0.0F, 0.50F, 0.0F},
    }};
    session.performance.texture = 0.34F;
    session.performance.pulse = 0.32F;
    session.performance.chaos = 0.22F;
    session.performance.space = 0.42F;
    session.performance.events = 0.44F;
    apply_scene_recipe(session, SceneKind::derelict);
    return session;
}

void apply_scene_recipe(Session& session, SceneKind scene) {
    session.scene = scene;
    session.scene_modified = false;

    const auto set_actor = [&session](
                               std::size_t index,
                               EngineKind engine,
                               float frequency,
                               float timbre,
                               float color,
                               float motion,
                               float texture,
                               float level,
                               float pan) {
        auto& slot = session.slots[index];
        slot.enabled = true;
        slot.engine = engine;
        slot.frequency_hz = frequency;
        slot.timbre = timbre;
        slot.color = color;
        slot.motion = motion;
        slot.texture = texture;
        slot.level = level;
        slot.pan = pan;
        for (auto& effect : slot.effects) {
            effect = {};
        }
        for (auto& modulator : slot.modulators) {
            modulator = {};
        }
        slot.modulators[0] = {
            true, ModWave::random_walk, ModDestination::pan, 0.035F + 0.017F * static_cast<float>(index), 0.10F, 0.0F};
    };

    switch (scene) {
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

    for (auto& slot : session.slots) {
        slot.effects[0] = {EffectKind::lowpass, 0.16F, 0.52F, 0.0F};
        slot.effects[1] = {EffectKind::drive, 0.012F, 0.44F, 0.0F};
        slot.effects[2] = {EffectKind::delay, 0.055F, 0.20F, 0.18F};
        slot.effects[3] = {EffectKind::bypass, 0.0F, 0.50F, 0.0F};
    }
    session.slots[0].effects[2].amount = 0.025F;
    session.slots[2].effects[2] = {EffectKind::delay, 0.10F, 0.24F, 0.26F};
    session.slots[3].effects[2] = {EffectKind::delay, 0.09F, 0.34F, 0.28F};

    if (scene == SceneKind::wet_cave) {
        session.slots[0].effects[0] = {EffectKind::lowpass, 0.24F, 0.42F, 0.0F};
        session.slots[1].effects[2] = {EffectKind::delay, 0.34F, 0.58F, 0.58F};
        session.slots[2].effects[0] = {EffectKind::highpass, 0.12F, 0.18F, 0.0F};
        session.slots[3].effects[2] = {EffectKind::comb, 0.18F, 0.24F, 0.42F};
    } else if (scene == SceneKind::metro) {
        session.slots[0].effects[1] = {EffectKind::wavefolder, 0.08F, 0.42F, 0.0F};
        session.slots[1].effects[2] = {EffectKind::comb, 0.12F, 0.16F, 0.34F};
        session.slots[2].effects[0] = {EffectKind::highpass, 0.20F, 0.36F, 0.0F};
        session.slots[3].effects[1] = {EffectKind::drive, 0.06F, 0.50F, 0.0F};
    } else if (scene == SceneKind::nursery) {
        session.slots[0].effects[0] = {EffectKind::lowpass, 0.28F, 0.34F, 0.0F};
        session.slots[1].effects[2] = {EffectKind::delay, 0.18F, 0.46F, 0.42F};
        session.slots[2].effects[1] = {EffectKind::drive, 0.035F, 0.34F, 0.0F};
        session.slots[3].effects[2] = {EffectKind::comb, 0.08F, 0.48F, 0.24F};
    } else if (scene == SceneKind::bunker) {
        session.slots[0].effects[0] = {EffectKind::lowpass, 0.30F, 0.30F, 0.0F};
        session.slots[1].effects[2] = {EffectKind::delay, 0.08F, 0.62F, 0.38F};
    } else if (scene == SceneKind::power_grid) {
        session.slots[0].effects[1] = {EffectKind::drive, 0.035F, 0.44F, 0.0F};
        session.slots[2].effects[2] = {EffectKind::comb, 0.08F, 0.28F, 0.22F};
    } else if (scene == SceneKind::deep_water) {
        session.slots[0].effects[0] = {EffectKind::lowpass, 0.38F, 0.24F, 0.0F};
        session.slots[1].effects[0] = {EffectKind::lowpass, 0.22F, 0.42F, 0.0F};
        session.slots[2].effects[2] = {EffectKind::delay, 0.14F, 0.70F, 0.48F};
    } else if (scene == SceneKind::ash_field) {
        session.slots[0].effects[0] = {EffectKind::lowpass, 0.32F, 0.28F, 0.0F};
        session.slots[1].effects[2] = {EffectKind::delay, 0.11F, 0.66F, 0.40F};
    }
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
    output << "scene=" << to_string(session.scene) << '\n';
    output << "scene_modified=" << (session.scene_modified ? 1 : 0) << '\n';
    output << "tempo_bpm=" << session.tempo_bpm << '\n';
    output << "master_level=" << session.master_level << '\n';
    output << "fade_in_seconds=" << session.fade_in_seconds << '\n';
    output << "fade_out_seconds=" << session.fade_out_seconds << '\n';
    output << "performance.texture=" << session.performance.texture << '\n';
    output << "performance.pulse=" << session.performance.pulse << '\n';
    output << "performance.chaos=" << session.performance.chaos << '\n';
    output << "performance.space=" << session.performance.space << '\n';
    output << "performance.events=" << session.performance.events << '\n';
    output << "performance.fade=" << session.performance.fade << '\n';
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
    if (schema == values.end() ||
        (schema->second != "1" && schema->second != "2" && schema->second != "3" &&
            schema->second != "4" && schema->second != "5" && schema->second != "6" &&
            schema->second != "7")) {
        error = "unsupported or missing session schema";
        return false;
    }

    Session loaded = make_default_session();
    const auto locale = values.find("locale");
    if (locale != values.end() && !parse_locale(locale->second, loaded.locale)) {
        error = "invalid locale";
        return false;
    }
    if (!parse_enum_value(values, "scene", loaded.scene, kScenes)) {
        error = "invalid scene";
        return false;
    }
    if (!parse_bool(values, "scene_modified", loaded.scene_modified)) {
        error = "invalid scene modification state";
        return false;
    }
    if (!parse_float(values, "tempo_bpm", loaded.tempo_bpm) ||
        !parse_float(values, "master_level", loaded.master_level) ||
        !parse_float(values, "fade_in_seconds", loaded.fade_in_seconds) ||
        !parse_float(values, "fade_out_seconds", loaded.fade_out_seconds) ||
        !parse_float(values, "performance.texture", loaded.performance.texture) ||
        !parse_float(values, "performance.pulse", loaded.performance.pulse) ||
        !parse_float(values, "performance.chaos", loaded.performance.chaos) ||
        !parse_float(values, "performance.space", loaded.performance.space) ||
        !parse_float(values, "performance.events", loaded.performance.events) ||
        !parse_float(values, "performance.fade", loaded.performance.fade)) {
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
    loaded.fade_in_seconds = std::clamp(loaded.fade_in_seconds, 0.25F, 30.0F);
    loaded.fade_out_seconds = std::clamp(loaded.fade_out_seconds, 0.25F, 30.0F);
    loaded.performance.texture = std::clamp(loaded.performance.texture, 0.0F, 1.0F);
    loaded.performance.pulse = std::clamp(loaded.performance.pulse, 0.0F, 1.0F);
    loaded.performance.chaos = std::clamp(loaded.performance.chaos, 0.0F, 1.0F);
    loaded.performance.space = std::clamp(loaded.performance.space, 0.0F, 1.0F);
    loaded.performance.events = std::clamp(loaded.performance.events, 0.0F, 1.0F);
    loaded.performance.fade = std::clamp(loaded.performance.fade, 0.0F, 1.0F);
    if (schema->second != "4" && schema->second != "5" && schema->second != "6" &&
        schema->second != "7") {
        apply_scene_recipe(loaded, SceneKind::derelict);
    }
    loaded.schema_version = 7;
    session = loaded;
    return true;
}

std::string to_string(Locale value) { return enum_name(value, kLocales); }
std::string to_string(SceneKind value) { return enum_name(value, kScenes); }
std::string to_string(EngineKind value) { return enum_name(value, kEngines); }
std::string to_string(EffectKind value) { return enum_name(value, kEffects); }
std::string to_string(ModWave value) { return enum_name(value, kWaves); }
std::string to_string(ModDestination value) { return enum_name(value, kDestinations); }

bool parse_locale(const std::string& text, Locale& value) { return parse_enum(text, value, kLocales); }
bool parse_scene(const std::string& text, SceneKind& value) { return parse_enum(text, value, kScenes); }

} // namespace cursed_drone

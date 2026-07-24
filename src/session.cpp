// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.
#include "cursed_drone/session.hpp"
#include "cursed_drone/scala.hpp"

#include <algorithm>
#include <array>
#include <cmath>
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
    std::pair{EngineKind::plaits, std::string_view{"plaits"}},
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
    std::pair{EffectKind::granular_reverse, std::string_view{"granular_reverse"}},
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

constexpr std::array kPlaitsModels{
    std::pair{PlaitsModel::virtual_analog_vcf, std::string_view{"virtual_analog_vcf"}},
    std::pair{PlaitsModel::phase_distortion, std::string_view{"phase_distortion"}},
    std::pair{PlaitsModel::wave_terrain, std::string_view{"wave_terrain"}},
    std::pair{PlaitsModel::string_machine, std::string_view{"string_machine"}},
    std::pair{PlaitsModel::chiptune, std::string_view{"chiptune"}},
    std::pair{PlaitsModel::virtual_analog, std::string_view{"virtual_analog"}},
    std::pair{PlaitsModel::waveshaping, std::string_view{"waveshaping"}},
    std::pair{PlaitsModel::fm, std::string_view{"fm"}},
    std::pair{PlaitsModel::grain, std::string_view{"grain"}},
    std::pair{PlaitsModel::additive, std::string_view{"additive"}},
    std::pair{PlaitsModel::wavetable, std::string_view{"wavetable"}},
    std::pair{PlaitsModel::chord, std::string_view{"chord"}},
    std::pair{PlaitsModel::swarm, std::string_view{"swarm"}},
    std::pair{PlaitsModel::noise, std::string_view{"noise"}},
    std::pair{PlaitsModel::string, std::string_view{"string"}},
    std::pair{PlaitsModel::modal, std::string_view{"modal"}},
};

constexpr std::array kPlaitsOutputs{
    std::pair{PlaitsOutputMode::main, std::string_view{"main"}},
    std::pair{PlaitsOutputMode::aux, std::string_view{"aux"}},
    std::pair{PlaitsOutputMode::mix, std::string_view{"mix"}},
    std::pair{PlaitsOutputMode::stereo, std::string_view{"stereo"}},
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

std::string master_effect_key(std::size_t effect, std::string_view field) {
    return "master.effect." + std::to_string(effect) + "." + std::string{field};
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

bool parse_int(const std::unordered_map<std::string, std::string>& values, const std::string& name, int& value) {
    const auto found = values.find(name);
    if (found == values.end()) return true;
    try {
        std::size_t consumed = 0;
        const int parsed = std::stoi(found->second, &consumed);
        if (consumed != found->second.size()) return false;
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool parse_name(
    const std::unordered_map<std::string, std::string>& values,
    const std::string& name,
    std::array<char, kScaleNameLength>& value) {
    const auto found = values.find(name);
    if (found == values.end()) return true;
    value.fill('\0');
    const std::size_t count = std::min(value.size() - 1U, found->second.size());
    std::copy_n(found->second.data(), count, value.data());
    return true;
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
        set_equal_temperament(slot.tuning);

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
        slot.event_density = std::clamp(0.30F + motion * 0.60F, 0.0F, 1.0F);
        slot.level = level;
        slot.pan = pan;
        slot.plaits_model = PlaitsModel::virtual_analog;
        slot.plaits_output = PlaitsOutputMode::stereo;
        slot.euclidean = {};
        set_equal_temperament(slot.tuning);
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
        set_actor(0U, EngineKind::earth_rumble, 20.0F, 0.34F, 0.22F, 0.12F, 0.30F, 0.40F, -0.18F);
        set_actor(1U, EngineKind::water_drip, 240.0F, 0.38F, 0.34F, 0.13F, 0.20F, 0.08F, 0.38F);
        set_actor(2U, EngineKind::water_flow, 58.0F, 0.34F, 0.28F, 0.22F, 0.36F, 0.18F, -0.42F);
        set_actor(3U, EngineKind::stone, 42.0F, 0.46F, 0.24F, 0.12F, 0.28F, 0.11F, 0.46F);
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
        set_actor(3U, EngineKind::earth_rumble, 20.0F, 0.34F, 0.20F, 0.16F, 0.32F, 0.26F, 0.42F);
        break;
    case SceneKind::deep_water:
        set_actor(0U, EngineKind::earth_rumble, 20.0F, 0.52F, 0.12F, 0.10F, 0.46F, 0.50F, -0.18F);
        set_actor(1U, EngineKind::water_flow, 42.0F, 0.30F, 0.18F, 0.18F, 0.40F, 0.11F, 0.32F);
        set_actor(2U, EngineKind::cave_air, 22.0F, 0.24F, 0.12F, 0.10F, 0.30F, 0.10F, -0.40F);
        set_actor(3U, EngineKind::sub_drone, 30.0F, 0.34F, 0.10F, 0.12F, 0.24F, 0.12F, 0.44F);
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
        session.slots[2].effects[0] = {EffectKind::lowpass, 0.34F, 0.22F, 0.0F};
        session.slots[2].effects[2] = {EffectKind::delay, 0.08F, 0.70F, 0.34F};
        session.slots[3].effects[0] = {EffectKind::lowpass, 0.30F, 0.26F, 0.0F};
    } else if (scene == SceneKind::ash_field) {
        session.slots[0].effects[0] = {EffectKind::lowpass, 0.32F, 0.28F, 0.0F};
        session.slots[1].effects[2] = {EffectKind::delay, 0.11F, 0.66F, 0.40F};
    }
}

bool save_session(const Session& session, const std::filesystem::path& path, std::string& error) {
    auto temporary_path = path;
    temporary_path += ".tmp";
    auto backup_path = path;
    backup_path += ".bak";
    auto backup_temporary_path = backup_path;
    backup_temporary_path += ".tmp";

    std::error_code filesystem_error;
    std::filesystem::remove(temporary_path, filesystem_error);
    filesystem_error.clear();

    std::ofstream output(temporary_path, std::ios::trunc);
    if (!output) {
        error = "cannot open temporary session for writing: " + temporary_path.string();
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
    output << "performance.morph_target=" << to_string(session.performance.morph_target) << '\n';
    output << "performance.morph=" << session.performance.morph << '\n';
    for (std::size_t effect_index = 0; effect_index < kMasterEffects; ++effect_index) {
        const auto& effect = session.master_effects[effect_index];
        output << master_effect_key(effect_index, "kind") << '=' << to_string(effect.kind) << '\n';
        output << master_effect_key(effect_index, "enabled") << '=' << (effect.enabled ? 1 : 0) << '\n';
        output << master_effect_key(effect_index, "amount") << '=' << effect.amount << '\n';
        output << master_effect_key(effect_index, "tone") << '=' << effect.tone << '\n';
        output << master_effect_key(effect_index, "feedback") << '=' << effect.feedback << '\n';
    }
    for (std::size_t slot_index = 0; slot_index < kSlotCount; ++slot_index) {
        const auto& slot = session.slots[slot_index];
        output << key(slot_index, "enabled") << '=' << (slot.enabled ? 1 : 0) << '\n';
        output << key(slot_index, "engine") << '=' << to_string(slot.engine) << '\n';
        output << key(slot_index, "frequency_hz") << '=' << slot.frequency_hz << '\n';
        output << key(slot_index, "timbre") << '=' << slot.timbre << '\n';
        output << key(slot_index, "color") << '=' << slot.color << '\n';
        output << key(slot_index, "motion") << '=' << slot.motion << '\n';
        output << key(slot_index, "texture") << '=' << slot.texture << '\n';
        output << key(slot_index, "event_density") << '=' << slot.event_density << '\n';
        output << key(slot_index, "level") << '=' << slot.level << '\n';
        output << key(slot_index, "pan") << '=' << slot.pan << '\n';
        output << key(slot_index, "plaits_model") << '=' << to_string(slot.plaits_model) << '\n';
        output << key(slot_index, "plaits_output") << '=' << to_string(slot.plaits_output) << '\n';
        output << key(slot_index, "tuning_enabled") << '=' << (slot.tuning.enabled ? 1 : 0) << '\n';
        output << key(slot_index, "tuning_root") << '=' << slot.tuning.root_midi << '\n';
        output << key(slot_index, "tuning_count") << '=' << slot.tuning.degree_count << '\n';
        output << key(slot_index, "tuning_period") << '=' << slot.tuning.period_cents << '\n';
        output << key(slot_index, "tuning_name") << '=' << slot.tuning.name.data() << '\n';
        for (std::size_t degree = 0; degree < kScaleDegreeCount; ++degree) {
            output << key(slot_index, "tuning_cents_" + std::to_string(degree)) << '='
                   << slot.tuning.cents[degree] << '\n';
        }
        output << key(slot_index, "euclidean_enabled") << '=' << (slot.euclidean.enabled ? 1 : 0) << '\n';
        output << key(slot_index, "euclidean_steps") << '=' << slot.euclidean.steps << '\n';
        output << key(slot_index, "euclidean_pulses") << '=' << slot.euclidean.pulses << '\n';
        output << key(slot_index, "euclidean_rotation") << '=' << slot.euclidean.rotation << '\n';
        output << key(slot_index, "euclidean_probability") << '=' << slot.euclidean.probability << '\n';
        for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {
            const auto& effect = slot.effects[effect_index];
            output << effect_key(slot_index, effect_index, "kind") << '=' << to_string(effect.kind) << '\n';
            output << effect_key(slot_index, effect_index, "enabled") << '=' << (effect.enabled ? 1 : 0) << '\n';
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
            output << mod_key(slot_index, mod_index, "rate_mod_source") << '=' << mod.rate_mod_source << '\n';
            output << mod_key(slot_index, mod_index, "rate_mod_amount") << '=' << mod.rate_mod_amount << '\n';
        }
    }
    output.flush();
    if (!output) {
        output.close();
        std::filesystem::remove(temporary_path, filesystem_error);
        error = "failed while writing session: " + temporary_path.string();
        return false;
    }
    output.close();
    if (!output) {
        std::filesystem::remove(temporary_path, filesystem_error);
        error = "failed while closing session: " + temporary_path.string();
        return false;
    }

    if (std::filesystem::is_regular_file(path, filesystem_error)) {
        filesystem_error.clear();
        std::filesystem::remove(backup_temporary_path, filesystem_error);
        filesystem_error.clear();
        std::filesystem::copy_file(
            path, backup_temporary_path, std::filesystem::copy_options::overwrite_existing, filesystem_error);
        if (filesystem_error) {
            std::filesystem::remove(temporary_path, filesystem_error);
            error = "cannot create session backup: " + backup_path.string();
            return false;
        }
        std::filesystem::remove(backup_path, filesystem_error);
        filesystem_error.clear();
        std::filesystem::rename(backup_temporary_path, backup_path, filesystem_error);
        if (filesystem_error) {
            std::filesystem::remove(temporary_path, filesystem_error);
            error = "cannot publish session backup: " + backup_path.string();
            return false;
        }
    }

    filesystem_error.clear();
    std::filesystem::rename(temporary_path, path, filesystem_error);
#if defined(_WIN32)
    if (filesystem_error) {
        // Windows rename does not replace an existing destination. This fallback
        // is not needed on the handheld targets, but keeps desktop saves working.
        filesystem_error.clear();
        std::filesystem::remove(path, filesystem_error);
        filesystem_error.clear();
        std::filesystem::rename(temporary_path, path, filesystem_error);
    }
#endif
    if (filesystem_error) {
        std::filesystem::remove(temporary_path, filesystem_error);
        error = "cannot publish session atomically: " + path.string();
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
            schema->second != "7" && schema->second != "8" && schema->second != "9" &&
            schema->second != "10" && schema->second != "11" && schema->second != "12")) {
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
        !parse_float(values, "performance.fade", loaded.performance.fade) ||
        !parse_enum_value(values, "performance.morph_target", loaded.performance.morph_target, kScenes) ||
        !parse_float(values, "performance.morph", loaded.performance.morph)) {
        error = "invalid master value";
        return false;
    }

    for (std::size_t effect_index = 0; effect_index < kMasterEffects; ++effect_index) {
        auto& effect = loaded.master_effects[effect_index];
        if (!parse_enum_value(values, master_effect_key(effect_index, "kind"), effect.kind, kEffects) ||
            !parse_bool(values, master_effect_key(effect_index, "enabled"), effect.enabled) ||
            !parse_float(values, master_effect_key(effect_index, "amount"), effect.amount) ||
            !parse_float(values, master_effect_key(effect_index, "tone"), effect.tone) ||
            !parse_float(values, master_effect_key(effect_index, "feedback"), effect.feedback)) {
            error = "invalid master effect " + std::to_string(effect_index + 1U);
            return false;
        }
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
            !parse_float(values, key(slot_index, "event_density"), slot.event_density) ||
            !parse_float(values, key(slot_index, "level"), slot.level) ||
            !parse_float(values, key(slot_index, "pan"), slot.pan) ||
            !parse_enum_value(values, key(slot_index, "plaits_model"), slot.plaits_model, kPlaitsModels) ||
            !parse_enum_value(values, key(slot_index, "plaits_output"), slot.plaits_output, kPlaitsOutputs) ||
            !parse_bool(values, key(slot_index, "tuning_enabled"), slot.tuning.enabled) ||
            !parse_int(values, key(slot_index, "tuning_root"), slot.tuning.root_midi) ||
            !parse_int(values, key(slot_index, "tuning_count"), slot.tuning.degree_count) ||
            !parse_float(values, key(slot_index, "tuning_period"), slot.tuning.period_cents) ||
            !parse_name(values, key(slot_index, "tuning_name"), slot.tuning.name) ||
            !parse_bool(values, key(slot_index, "euclidean_enabled"), slot.euclidean.enabled) ||
            !parse_int(values, key(slot_index, "euclidean_steps"), slot.euclidean.steps) ||
            !parse_int(values, key(slot_index, "euclidean_pulses"), slot.euclidean.pulses) ||
            !parse_int(values, key(slot_index, "euclidean_rotation"), slot.euclidean.rotation) ||
            !parse_float(values, key(slot_index, "euclidean_probability"), slot.euclidean.probability)) {
            error = "invalid value in slot " + std::to_string(slot_index + 1U);
            return false;
        }
        for (std::size_t degree = 0; degree < kScaleDegreeCount; ++degree) {
            if (!parse_float(values, key(slot_index, "tuning_cents_" + std::to_string(degree)),
                    slot.tuning.cents[degree])) {
                error = "invalid tuning in slot " + std::to_string(slot_index + 1U);
                return false;
            }
        }
        for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {
            auto& effect = slot.effects[effect_index];
            if (!parse_enum_value(values, effect_key(slot_index, effect_index, "kind"), effect.kind, kEffects) ||
                !parse_bool(values, effect_key(slot_index, effect_index, "enabled"), effect.enabled) ||
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
                !parse_float(values, mod_key(slot_index, mod_index, "offset"), mod.offset) ||
                !parse_int(values, mod_key(slot_index, mod_index, "rate_mod_source"), mod.rate_mod_source) ||
                !parse_float(values, mod_key(slot_index, mod_index, "rate_mod_amount"), mod.rate_mod_amount)) {
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
    loaded.performance.morph = std::clamp(loaded.performance.morph, 0.0F, 1.0F);
    for (auto& effect : loaded.master_effects) {
        effect.amount = std::clamp(effect.amount, 0.0F, 1.0F);
        effect.tone = std::clamp(effect.tone, 0.0F, 1.0F);
        effect.feedback = std::clamp(effect.feedback, 0.0F, 1.0F);
    }
    for (auto& slot : loaded.slots) {
        slot.frequency_hz = std::clamp(slot.frequency_hz, 20.0F, 2'000.0F);
        slot.event_density = std::clamp(slot.event_density, 0.0F, 1.0F);
        slot.tuning.root_midi = std::clamp(slot.tuning.root_midi, 0, 127);
        slot.tuning.degree_count = std::clamp(slot.tuning.degree_count, 1, static_cast<int>(kScaleDegreeCount));
        slot.tuning.period_cents = std::clamp(slot.tuning.period_cents, 50.0F, 4800.0F);
        slot.euclidean.steps = std::clamp(slot.euclidean.steps, 1, 32);
        slot.euclidean.pulses = std::clamp(slot.euclidean.pulses, 0, slot.euclidean.steps);
        slot.euclidean.rotation = std::clamp(slot.euclidean.rotation, 0, slot.euclidean.steps - 1);
        slot.euclidean.probability = std::clamp(slot.euclidean.probability, 0.0F, 1.0F);
        for (auto& mod : slot.modulators) {
            mod.depth = std::clamp(mod.depth, -1.0F, 1.0F);
            mod.rate_mod_source = std::clamp(mod.rate_mod_source, -1, 3);
            mod.rate_mod_amount = std::clamp(mod.rate_mod_amount, -1.0F, 1.0F);
        }
    }
    if (schema->second != "4" && schema->second != "5" && schema->second != "6" &&
        schema->second != "7" && schema->second != "8" && schema->second != "9" &&
        schema->second != "10" && schema->second != "11" && schema->second != "12") {
        apply_scene_recipe(loaded, SceneKind::derelict);
    }
    loaded.schema_version = 12;
    session = loaded;
    return true;
}

bool supports_event_rate(EngineKind value) noexcept {
    switch (value) {
    case EngineKind::body:
    case EngineKind::grain:
    case EngineKind::particle:
    case EngineKind::footsteps:
    case EngineKind::door:
    case EngineKind::pipe:
    case EngineKind::machinery:
    case EngineKind::metal:
    case EngineKind::birds:
    case EngineKind::signal:
    case EngineKind::water_drip:
    case EngineKind::water_flow:
    case EngineKind::stone:
    case EngineKind::rail_joint:
    case EngineKind::brake:
    case EngineKind::music_box:
    case EngineKind::toy_voice:
    case EngineKind::toy_gears:
    case EngineKind::lullaby:
    case EngineKind::earth_rumble:
        return true;
    default:
        return false;
    }
}

bool supports_manual_trigger(EngineKind value) noexcept {
    return supports_event_rate(value) || value == EngineKind::plaits;
}

float effective_event_density(float actor_density, float global_events) noexcept {
    return std::clamp(actor_density * 0.72F + global_events * 0.55F, 0.0F, 1.0F);
}

float event_rate_hz(float tempo_bpm, float event_density, float motion) noexcept {
    const float beat_hz = std::clamp(tempo_bpm, 10.0F, 300.0F) / 60.0F;
    const float density = std::clamp(event_density, 0.0F, 1.0F);
    const float movement = std::clamp(motion, 0.0F, 1.0F);
    return beat_hz * (0.08F + density * density * 2.9F) * (0.65F + movement * 1.25F);
}

float event_max_wait_seconds(float tempo_bpm, float event_density, float motion) noexcept {
    return 1.8F / std::max(0.001F, event_rate_hz(tempo_bpm, event_density, motion));
}

std::string to_string(Locale value) { return enum_name(value, kLocales); }
std::string to_string(SceneKind value) { return enum_name(value, kScenes); }
std::string to_string(EngineKind value) { return enum_name(value, kEngines); }
std::string to_string(EffectKind value) { return enum_name(value, kEffects); }
std::string to_string(ModWave value) { return enum_name(value, kWaves); }
std::string to_string(ModDestination value) { return enum_name(value, kDestinations); }
std::string to_string(PlaitsModel value) { return enum_name(value, kPlaitsModels); }
std::string to_string(PlaitsOutputMode value) { return enum_name(value, kPlaitsOutputs); }

bool parse_locale(const std::string& text, Locale& value) { return parse_enum(text, value, kLocales); }
bool parse_scene(const std::string& text, SceneKind& value) { return parse_enum(text, value, kScenes); }

} // namespace cursed_drone

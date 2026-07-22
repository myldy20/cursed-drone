// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.
#pragma once

#include "cursed_drone/session.hpp"

#include <array>

namespace cursed_drone::catalog {

inline constexpr std::array<SceneKind, 10> scenes{
    SceneKind::derelict, SceneKind::factory, SceneKind::wasteland,
    SceneKind::wet_cave, SceneKind::metro, SceneKind::nursery,
    SceneKind::bunker, SceneKind::power_grid, SceneKind::deep_water,
    SceneKind::ash_field};

inline constexpr std::array<std::array<EngineKind, 4>, 8> engine_groups{{
    {EngineKind::macro, EngineKind::body, EngineKind::grain, EngineKind::particle},
    {EngineKind::derelict_bed, EngineKind::footsteps, EngineKind::door, EngineKind::pipe},
    {EngineKind::motor, EngineKind::machinery, EngineKind::crowd, EngineKind::metal},
    {EngineKind::wind, EngineKind::birds, EngineKind::insects, EngineKind::signal},
    {EngineKind::cave_air, EngineKind::water_drip, EngineKind::water_flow, EngineKind::stone},
    {EngineKind::metro_traction, EngineKind::rail_joint, EngineKind::brake, EngineKind::carriage},
    {EngineKind::music_box, EngineKind::toy_voice, EngineKind::toy_gears, EngineKind::lullaby},
    {EngineKind::sub_drone, EngineKind::tape_drone, EngineKind::bowed_metal, EngineKind::earth_rumble},
}};

inline constexpr std::array<EngineKind, 34> engines{
    EngineKind::diagnostic,
    EngineKind::macro, EngineKind::body, EngineKind::grain, EngineKind::particle,
    EngineKind::derelict_bed, EngineKind::footsteps, EngineKind::door, EngineKind::pipe,
    EngineKind::motor, EngineKind::machinery, EngineKind::crowd, EngineKind::metal,
    EngineKind::wind, EngineKind::birds, EngineKind::insects, EngineKind::signal,
    EngineKind::cave_air, EngineKind::water_drip, EngineKind::water_flow, EngineKind::stone,
    EngineKind::metro_traction, EngineKind::rail_joint, EngineKind::brake, EngineKind::carriage,
    EngineKind::music_box, EngineKind::toy_voice, EngineKind::toy_gears, EngineKind::lullaby,
    EngineKind::sub_drone, EngineKind::tape_drone, EngineKind::bowed_metal,
    EngineKind::earth_rumble, EngineKind::plaits};

inline constexpr std::array<EffectKind, 15> basic_effects{
    EffectKind::bypass, EffectKind::drive, EffectKind::lowpass,
    EffectKind::highpass, EffectKind::tremolo, EffectKind::delay,
    EffectKind::crusher, EffectKind::wavefolder, EffectKind::ringmod,
    EffectKind::comb, EffectKind::chorus, EffectKind::flanger,
    EffectKind::phaser, EffectKind::diffuser, EffectKind::ahdr};

inline constexpr std::array<EffectKind, 6> compound_effects{
    EffectKind::tape_void, EffectKind::black_hole, EffectKind::ritual_gate,
    EffectKind::rust_cloud, EffectKind::deep_sea, EffectKind::granular_reverse};

inline constexpr std::array<EffectKind, 21> effects{
    EffectKind::bypass, EffectKind::drive, EffectKind::lowpass,
    EffectKind::highpass, EffectKind::tremolo, EffectKind::delay,
    EffectKind::crusher, EffectKind::wavefolder, EffectKind::ringmod,
    EffectKind::comb, EffectKind::chorus, EffectKind::flanger,
    EffectKind::phaser, EffectKind::diffuser, EffectKind::ahdr,
    EffectKind::tape_void, EffectKind::black_hole, EffectKind::ritual_gate,
    EffectKind::rust_cloud, EffectKind::deep_sea, EffectKind::granular_reverse};

inline constexpr std::array<PlaitsModel, 16> plaits_models{
    PlaitsModel::virtual_analog_vcf, PlaitsModel::phase_distortion,
    PlaitsModel::wave_terrain, PlaitsModel::string_machine,
    PlaitsModel::chiptune, PlaitsModel::virtual_analog,
    PlaitsModel::waveshaping, PlaitsModel::fm, PlaitsModel::grain,
    PlaitsModel::additive, PlaitsModel::wavetable, PlaitsModel::chord,
    PlaitsModel::swarm, PlaitsModel::noise, PlaitsModel::string,
    PlaitsModel::modal};

inline constexpr std::array<PlaitsOutputMode, 4> plaits_outputs{
    PlaitsOutputMode::main, PlaitsOutputMode::aux,
    PlaitsOutputMode::mix, PlaitsOutputMode::stereo};

inline constexpr std::array<ModWave, 4> mod_waves{
    ModWave::sine, ModWave::triangle, ModWave::sample_hold, ModWave::random_walk};

inline constexpr std::array<ModDestination, 11> mod_destinations{
    ModDestination::pitch, ModDestination::timbre, ModDestination::color,
    ModDestination::motion, ModDestination::texture, ModDestination::level,
    ModDestination::pan, ModDestination::fx1, ModDestination::fx2,
    ModDestination::fx3, ModDestination::fx4};

} // namespace cursed_drone::catalog

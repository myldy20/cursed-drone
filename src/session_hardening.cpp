// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.
#include "cursed_drone/session.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <type_traits>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

namespace cursed_drone {

// src/session.cpp is compiled with symbol-renaming definitions so the original
// parser/writer remain the single implementation of the on-disk format. These
// wrappers add validation and recovery without duplicating the schema code.
[[nodiscard]] bool save_session_unchecked(
    const Session& session,
    const std::filesystem::path& path,
    std::string& error);
[[nodiscard]] bool load_session_unchecked(
    const std::filesystem::path& path,
    Session& session,
    std::string& error);

namespace {

template <typename Enum>
[[nodiscard]] bool enum_between(Enum value, Enum first, Enum last) noexcept {
    using Underlying = std::underlying_type_t<Enum>;
    const auto raw = static_cast<Underlying>(value);
    return raw >= static_cast<Underlying>(first) &&
        raw <= static_cast<Underlying>(last);
}

[[nodiscard]] float finite_clamp(
    float value,
    float minimum,
    float maximum,
    float fallback) noexcept {
    if (!std::isfinite(value)) value = fallback;
    return std::clamp(value, minimum, maximum);
}

void request_platform_persist() noexcept {
#if defined(__EMSCRIPTEN__)
    EM_ASM({
        if (typeof Module !== 'undefined' &&
            typeof Module.cursedDronePersistNow === 'function') {
            Module.cursedDronePersistNow();
        }
    });
#endif
}

void sanitize_effect(
    EffectSettings& effect,
    const EffectSettings& fallback) noexcept {
    if (!enum_between(effect.kind, EffectKind::bypass,
            EffectKind::granular_reverse)) {
        effect.kind = fallback.kind;
    }
    effect.amount = finite_clamp(effect.amount, 0.0F, 1.0F,
        fallback.amount);
    effect.tone = finite_clamp(effect.tone, 0.0F, 1.0F,
        fallback.tone);
    effect.feedback = finite_clamp(effect.feedback, 0.0F, 1.0F,
        fallback.feedback);
}

void sanitize_tuning(
    ScalaTuning& tuning,
    const ScalaTuning& fallback) noexcept {
    tuning.root_midi = std::clamp(tuning.root_midi, 0, 127);
    tuning.degree_count = std::clamp(tuning.degree_count, 1,
        static_cast<int>(kScaleDegreeCount));
    tuning.period_cents = finite_clamp(tuning.period_cents, 50.0F,
        4'800.0F, fallback.period_cents);
    for (std::size_t index = 0; index < tuning.cents.size(); ++index) {
        tuning.cents[index] = finite_clamp(tuning.cents[index], -9'600.0F,
            9'600.0F, fallback.cents[index]);
    }
    tuning.name.back() = '\0';
}

void sanitize_modulator(
    ModSettings& mod,
    const ModSettings& fallback) noexcept {
    if (!enum_between(mod.wave, ModWave::sine, ModWave::random_walk)) {
        mod.wave = fallback.wave;
    }
    if (!enum_between(mod.destination, ModDestination::pitch,
            ModDestination::fx4)) {
        mod.destination = fallback.destination;
    }
    mod.rate_hz = finite_clamp(mod.rate_hz, 0.001F, 40.0F,
        fallback.rate_hz);
    mod.depth = finite_clamp(mod.depth, -1.0F, 1.0F, fallback.depth);
    mod.offset = finite_clamp(mod.offset, -1.0F, 1.0F, fallback.offset);
    mod.rate_mod_source = std::clamp(mod.rate_mod_source, -1,
        static_cast<int>(kModulatorsPerSlot) - 1);
    mod.rate_mod_amount = finite_clamp(mod.rate_mod_amount, -1.0F, 1.0F,
        fallback.rate_mod_amount);
}

[[nodiscard]] bool load_candidate(
    const std::filesystem::path& path,
    Session& session,
    std::string& error) {
    Session loaded{};
    if (!load_session_unchecked(path, loaded, error)) return false;
    sanitize_session(loaded);
    session = loaded;
    return true;
}

} // namespace

void sanitize_session(Session& session) noexcept {
    const Session fallback = make_default_session();
    session.schema_version = fallback.schema_version;

    if (!enum_between(session.locale, Locale::ru, Locale::en)) {
        session.locale = fallback.locale;
    }
    if (!enum_between(session.scene, SceneKind::derelict,
            SceneKind::ash_field)) {
        session.scene = fallback.scene;
    }
    if (!enum_between(session.performance.morph_target,
            SceneKind::derelict, SceneKind::ash_field)) {
        session.performance.morph_target = fallback.performance.morph_target;
    }

    session.tempo_bpm = finite_clamp(session.tempo_bpm, 10.0F, 300.0F,
        fallback.tempo_bpm);
    session.master_level = finite_clamp(session.master_level, 0.0F, 1.0F,
        fallback.master_level);
    session.fade_in_seconds = finite_clamp(session.fade_in_seconds, 0.25F,
        30.0F, fallback.fade_in_seconds);
    session.fade_out_seconds = finite_clamp(session.fade_out_seconds, 0.25F,
        30.0F, fallback.fade_out_seconds);

    session.performance.texture = finite_clamp(session.performance.texture,
        0.0F, 1.0F, fallback.performance.texture);
    session.performance.pulse = finite_clamp(session.performance.pulse,
        0.0F, 1.0F, fallback.performance.pulse);
    session.performance.chaos = finite_clamp(session.performance.chaos,
        0.0F, 1.0F, fallback.performance.chaos);
    session.performance.space = finite_clamp(session.performance.space,
        0.0F, 1.0F, fallback.performance.space);
    session.performance.events = finite_clamp(session.performance.events,
        0.0F, 1.0F, fallback.performance.events);
    session.performance.fade = finite_clamp(session.performance.fade,
        0.0F, 1.0F, fallback.performance.fade);
    session.performance.morph = finite_clamp(session.performance.morph,
        0.0F, 1.0F, fallback.performance.morph);

    for (std::size_t index = 0; index < session.master_effects.size(); ++index) {
        sanitize_effect(session.master_effects[index],
            fallback.master_effects[index]);
    }

    for (std::size_t slot_index = 0; slot_index < session.slots.size();
         ++slot_index) {
        auto& slot = session.slots[slot_index];
        const auto& default_slot = fallback.slots[slot_index];
        if (!enum_between(slot.engine, EngineKind::diagnostic,
                EngineKind::plaits)) {
            slot.engine = default_slot.engine;
        }
        if (!enum_between(slot.plaits_model,
                PlaitsModel::virtual_analog_vcf, PlaitsModel::modal)) {
            slot.plaits_model = default_slot.plaits_model;
        }
        if (!enum_between(slot.plaits_output, PlaitsOutputMode::main,
                PlaitsOutputMode::stereo)) {
            slot.plaits_output = default_slot.plaits_output;
        }

        slot.frequency_hz = finite_clamp(slot.frequency_hz, 20.0F, 2'000.0F,
            default_slot.frequency_hz);
        slot.timbre = finite_clamp(slot.timbre, 0.0F, 1.0F,
            default_slot.timbre);
        slot.color = finite_clamp(slot.color, 0.0F, 1.0F,
            default_slot.color);
        slot.motion = finite_clamp(slot.motion, 0.0F, 1.0F,
            default_slot.motion);
        slot.texture = finite_clamp(slot.texture, 0.0F, 1.0F,
            default_slot.texture);
        slot.event_density = finite_clamp(slot.event_density, 0.0F, 1.0F,
            default_slot.event_density);
        slot.level = finite_clamp(slot.level, 0.0F, 1.5F,
            default_slot.level);
        slot.pan = finite_clamp(slot.pan, -1.0F, 1.0F,
            default_slot.pan);

        sanitize_tuning(slot.tuning, default_slot.tuning);
        slot.euclidean.steps = std::clamp(slot.euclidean.steps, 1, 32);
        slot.euclidean.pulses = std::clamp(slot.euclidean.pulses, 0,
            slot.euclidean.steps);
        slot.euclidean.rotation = std::clamp(slot.euclidean.rotation, 0,
            slot.euclidean.steps - 1);
        slot.euclidean.probability = finite_clamp(
            slot.euclidean.probability, 0.0F, 1.0F,
            default_slot.euclidean.probability);

        for (std::size_t index = 0; index < slot.effects.size(); ++index) {
            sanitize_effect(slot.effects[index], default_slot.effects[index]);
        }
        for (std::size_t index = 0; index < slot.modulators.size(); ++index) {
            sanitize_modulator(slot.modulators[index],
                default_slot.modulators[index]);
        }
    }
}

bool save_session(
    const Session& session,
    const std::filesystem::path& path,
    std::string& error) {
    Session sanitized = session;
    sanitize_session(sanitized);
    if (!save_session_unchecked(sanitized, path, error)) return false;
    request_platform_persist();
    return true;
}

bool load_session(
    const std::filesystem::path& path,
    Session& session,
    std::string& error) {
    std::string primary_error;
    if (load_candidate(path, session, primary_error)) {
        error.clear();
        return true;
    }

    auto backup_path = path;
    backup_path += ".bak";
    std::string backup_error;
    if (std::filesystem::exists(backup_path) &&
        load_candidate(backup_path, session, backup_error)) {
        error = "recovered from backup after: " + primary_error;
        return true;
    }

    error = primary_error;
    if (!backup_error.empty()) {
        error += "; backup failed: " + backup_error;
    }
    return false;
}

} // namespace cursed_drone

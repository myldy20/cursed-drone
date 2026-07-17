#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text(encoding='utf-8')
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f'{path}: expected one match, found {count}: {old[:120]!r}')
    p.write_text(text.replace(old, new), encoding='utf-8')

path = 'src/sdl_main.cpp'
replace_once(path,
'''#include "cursed_drone/i18n.hpp"
#include "cursed_drone/session.hpp"
''',
'''#include "cursed_drone/i18n.hpp"
#include "cursed_drone/scala.hpp"
#include "cursed_drone/session.hpp"
''')
replace_once(path,
'''#include <system_error>
''',
'''#include <system_error>
#include <vector>
''')
replace_once(path,
'''    bool back_long_action{false};
    Uint32 back_held_since{0};
};''',
'''    bool back_long_action{false};
    Uint32 back_held_since{0};
    bool lab_open{false};
    int lab_tab{0};
    int lab_field{0};
    int lab_modulator{0};
    bool stage_mode{false};
    int stage_field{0};
};

std::vector<cd::ParsedScale> g_scales{};''')

replace_once(path,
'''    case cd::EngineKind::earth_rumble: return russian ? "ГУД ЗЕМЛИ" : "EARTH RUMBLE";
    }''',
'''    case cd::EngineKind::earth_rumble: return russian ? "ГУД ЗЕМЛИ" : "EARTH RUMBLE";
    case cd::EngineKind::plaits: return russian ? "МАКРО-АКТЕР" : "MACRO ACTOR";
    }''')
replace_once(path,
'''    case cd::EngineKind::earth_rumble: {
        constexpr std::array<std::string_view, 4> r{"МАССА", "ПОРОДА", "СДВИГ", "УДАРЫ"};
        constexpr std::array<std::string_view, 4> e{"MASS", "GROUND", "HEAVE", "IMPACTS"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::diagnostic:''',
'''    case cd::EngineKind::earth_rumble: {
        constexpr std::array<std::string_view, 4> r{"МАССА", "ПОРОДА", "СДВИГ", "УДАРЫ"};
        constexpr std::array<std::string_view, 4> e{"MASS", "GROUND", "HEAVE", "IMPACTS"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::plaits: {
        constexpr std::array<std::string_view, 4> r{"ГАРМОНИКИ", "ТЕМБР", "МОРФ", "РАСПАД"};
        constexpr std::array<std::string_view, 4> e{"HARMONICS", "TIMBRE", "MORPH", "DECAY"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::diagnostic:''')
replace_once(path,
'''    case cd::EffectKind::deep_sea: return russian ? "ГЛУБИНА" : "DEEP SEA";
    }''',
'''    case cd::EffectKind::deep_sea: return russian ? "ГЛУБИНА" : "DEEP SEA";
    case cd::EffectKind::granular_reverse: return russian ? "ОБРАТНЫЕ ЗЕРНА" : "REVERSE GRAINS";
    }''')
replace_once(path,
'''    case cd::EffectKind::deep_sea: return 3;
    }''',
'''    case cd::EffectKind::deep_sea:
    case cd::EffectKind::granular_reverse: return 3;
    }''')
replace_once(path,
'''    case cd::EffectKind::deep_sea:
        if (index == 0) return russian ? "ИНТЕНСИВН." : "INTENSITY";
        if (index == 1) return russian ? "ХАРАКТЕР" : "CHARACTER";
        return russian ? "ДВИЖЕНИЕ" : "MOTION";
    }''',
'''    case cd::EffectKind::deep_sea:
        if (index == 0) return russian ? "ИНТЕНСИВН." : "INTENSITY";
        if (index == 1) return russian ? "ХАРАКТЕР" : "CHARACTER";
        return russian ? "ДВИЖЕНИЕ" : "MOTION";
    case cd::EffectKind::granular_reverse:
        if (index == 0) return russian ? "ПОДМЕСЬ" : "MIX";
        if (index == 1) return russian ? "ЗЕРНО" : "GRAIN";
        return russian ? "ОБР. СВЯЗЬ" : "FEEDBACK";
    }''')
replace_once(path,
'''constexpr std::array<cd::EffectKind, 5> kCompoundEffects{
    cd::EffectKind::tape_void, cd::EffectKind::black_hole,
    cd::EffectKind::ritual_gate, cd::EffectKind::rust_cloud, cd::EffectKind::deep_sea};''',
'''constexpr std::array<cd::EffectKind, 6> kCompoundEffects{
    cd::EffectKind::tape_void, cd::EffectKind::black_hole,
    cd::EffectKind::ritual_gate, cd::EffectKind::rust_cloud,
    cd::EffectKind::deep_sea, cd::EffectKind::granular_reverse};''')

lab_block = r'''
std::string_view plaits_model_name(cd::PlaitsModel model) noexcept {
    constexpr std::array<std::string_view, 16> names{
        "VA FILTER", "PHASE DIST", "WAVE TERRAIN", "STRING MACHINE",
        "CHIPTUNE", "VIRTUAL ANALOG", "WAVESHAPER", "FM",
        "GRAIN", "ADDITIVE", "WAVETABLE", "CHORD",
        "SWARM", "NOISE", "STRING", "MODAL"};
    return names[static_cast<std::size_t>(model)];
}

std::string_view output_mode_name(cd::PlaitsOutputMode mode) noexcept {
    constexpr std::array<std::string_view, 4> names{"MAIN", "AUX", "MIX", "STEREO"};
    return names[static_cast<std::size_t>(mode)];
}

std::string_view mod_wave_name(cd::ModWave wave) noexcept {
    constexpr std::array<std::string_view, 4> names{"SINE", "TRIANGLE", "S&H", "RANDOM WALK"};
    return names[static_cast<std::size_t>(wave)];
}

std::string_view mod_destination_name(cd::ModDestination destination) noexcept {
    constexpr std::array<std::string_view, 11> names{
        "PITCH", "TIMBRE", "COLOR", "MOTION", "TEXTURE", "LEVEL", "PAN",
        "FX1", "FX2", "FX3", "FX4"};
    return names[static_cast<std::size_t>(destination)];
}

int lab_field_count(int tab) noexcept {
    constexpr std::array<int, 5> counts{5, 7, 6, 3, 1};
    return counts[static_cast<std::size_t>(std::clamp(tab, 0, 4))];
}

void navigate_lab(UiState& state, int horizontal, int vertical) noexcept {
    if (horizontal != 0) {
        state.lab_tab = (state.lab_tab + horizontal + 5) % 5;
        state.lab_field = std::min(state.lab_field, lab_field_count(state.lab_tab) - 1);
    }
    if (vertical != 0) {
        const int count = lab_field_count(state.lab_tab);
        state.lab_field = (state.lab_field + vertical + count) % count;
    }
}

int current_scale_index(const cd::ScalaTuning& tuning) noexcept {
    for (std::size_t index = 0; index < g_scales.size(); ++index) {
        if (g_scales[index].name == tuning.name.data()) return static_cast<int>(index);
    }
    return 0;
}

void adjust_lab(cd::Session& session, UiState& state, int direction) {
    auto& slot = session.slots[static_cast<std::size_t>(state.slot)];
    if (state.lab_tab == 0) {
        if (state.lab_field == 0) {
            slot.engine = cd::EngineKind::plaits;
        } else if (state.lab_field == 1) {
            const int count = 16;
            slot.plaits_model = static_cast<cd::PlaitsModel>(
                (static_cast<int>(slot.plaits_model) + direction + count) % count);
        } else if (state.lab_field == 2) {
            const int count = 4;
            slot.plaits_output = static_cast<cd::PlaitsOutputMode>(
                (static_cast<int>(slot.plaits_output) + direction + count) % count);
        } else if (state.lab_field == 3 && !g_scales.empty()) {
            int index = current_scale_index(slot.tuning);
            index = (index + direction + static_cast<int>(g_scales.size())) % static_cast<int>(g_scales.size());
            cd::apply_scale(slot.tuning, g_scales[static_cast<std::size_t>(index)]);
        } else if (state.lab_field == 4) {
            slot.tuning.root_midi = std::clamp(slot.tuning.root_midi + direction, 0, 127);
        }
    } else if (state.lab_tab == 1) {
        auto& mod = slot.modulators[static_cast<std::size_t>(state.lab_modulator)];
        if (state.lab_field == 0) mod.enabled = direction > 0;
        else if (state.lab_field == 1) mod.wave = static_cast<cd::ModWave>(
            (static_cast<int>(mod.wave) + direction + 4) % 4);
        else if (state.lab_field == 2) mod.destination = static_cast<cd::ModDestination>(
            (static_cast<int>(mod.destination) + direction + 11) % 11);
        else if (state.lab_field == 3) mod.rate_hz = std::clamp(
            mod.rate_hz * std::pow(2.0F, static_cast<float>(direction) / 12.0F), 0.001F, 40.0F);
        else if (state.lab_field == 4) mod.depth = std::clamp(mod.depth + direction * 0.02F, -1.0F, 1.0F);
        else if (state.lab_field == 5) {
            const int max_source = state.lab_modulator - 1;
            if (max_source < 0) mod.rate_mod_source = -1;
            else {
                const int count = max_source + 2;
                mod.rate_mod_source = ((mod.rate_mod_source + 1 + direction + count) % count) - 1;
            }
        } else if (state.lab_field == 6) {
            mod.rate_mod_amount = std::clamp(mod.rate_mod_amount + direction * 0.02F, -1.0F, 1.0F);
        }
    } else if (state.lab_tab == 2) {
        auto& sequence = slot.euclidean;
        if (state.lab_field == 0) sequence.enabled = direction > 0;
        else if (state.lab_field == 1) {
            sequence.steps = std::clamp(sequence.steps + direction, 1, 32);
            sequence.pulses = std::min(sequence.pulses, sequence.steps);
            sequence.rotation = std::min(sequence.rotation, sequence.steps - 1);
        } else if (state.lab_field == 2) sequence.pulses = std::clamp(sequence.pulses + direction, 0, sequence.steps);
        else if (state.lab_field == 3) sequence.rotation =
            (sequence.rotation + direction + sequence.steps) % sequence.steps;
        else if (state.lab_field == 4) sequence.probability =
            std::clamp(sequence.probability + direction * 0.02F, 0.0F, 1.0F);
        else if (state.lab_field == 5) {
            slot.effects[3].kind = direction > 0 ? cd::EffectKind::granular_reverse : cd::EffectKind::bypass;
            if (direction > 0 && slot.effects[3].amount < 0.01F) {
                slot.effects[3] = {cd::EffectKind::granular_reverse, 0.42F, 0.52F, 0.38F};
            }
        }
    } else if (state.lab_tab == 3) {
        if (state.lab_field == 0) {
            constexpr int count = 10;
            session.performance.morph_target = static_cast<cd::SceneKind>(
                (static_cast<int>(session.performance.morph_target) + direction + count) % count);
        } else if (state.lab_field == 1) {
            session.performance.morph = std::clamp(session.performance.morph + direction * 0.02F, 0.0F, 1.0F);
        }
    }
    session.scene_modified = true;
}

void activate_lab(cd::Session& session, UiState& state) {
    auto& slot = session.slots[static_cast<std::size_t>(state.slot)];
    if (state.lab_tab == 0 && state.lab_field == 0) {
        slot.engine = cd::EngineKind::plaits;
    } else if (state.lab_tab == 1 && state.lab_field == 0) {
        auto& mod = slot.modulators[static_cast<std::size_t>(state.lab_modulator)];
        mod.enabled = !mod.enabled;
    } else if (state.lab_tab == 2 && state.lab_field == 0) {
        slot.euclidean.enabled = !slot.euclidean.enabled;
    } else if (state.lab_tab == 2 && state.lab_field == 5) {
        const bool active = slot.effects[3].kind == cd::EffectKind::granular_reverse;
        slot.effects[3] = active
            ? cd::EffectSettings{}
            : cd::EffectSettings{cd::EffectKind::granular_reverse, 0.42F, 0.52F, 0.38F};
    } else if (state.lab_tab == 3 && state.lab_field == 2) {
        cd::apply_scene_recipe(session, session.performance.morph_target);
        session.performance.morph = 0.0F;
    } else if (state.lab_tab == 4) {
        state.lab_open = false;
        state.stage_mode = true;
    }
    session.scene_modified = true;
}

std::string lab_value(const cd::Session& session, const UiState& state, int field) {
    const auto& slot = session.slots[static_cast<std::size_t>(state.slot)];
    char value[48]{};
    if (state.lab_tab == 0) {
        if (field == 0) return slot.engine == cd::EngineKind::plaits ? "PLAITs ACTIVE" : "A: ACTIVATE";
        if (field == 1) return std::string{plaits_model_name(slot.plaits_model)};
        if (field == 2) return std::string{output_mode_name(slot.plaits_output)};
        if (field == 3) return slot.tuning.name.data();
        std::snprintf(value, sizeof(value), "MIDI %d", slot.tuning.root_midi);
    } else if (state.lab_tab == 1) {
        const auto& mod = slot.modulators[static_cast<std::size_t>(state.lab_modulator)];
        if (field == 0) return mod.enabled ? "ON" : "OFF";
        if (field == 1) return std::string{mod_wave_name(mod.wave)};
        if (field == 2) return std::string{mod_destination_name(mod.destination)};
        if (field == 3) std::snprintf(value, sizeof(value), "%.3f HZ", static_cast<double>(mod.rate_hz));
        else if (field == 4) std::snprintf(value, sizeof(value), "%+.0f%%", static_cast<double>(mod.depth * 100.0F));
        else if (field == 5) {
            if (mod.rate_mod_source < 0) return "NONE";
            return "MOD " + std::to_string(mod.rate_mod_source + 1);
        } else std::snprintf(value, sizeof(value), "%+.0f%%", static_cast<double>(mod.rate_mod_amount * 100.0F));
    } else if (state.lab_tab == 2) {
        const auto& sequence = slot.euclidean;
        if (field == 0) return sequence.enabled ? "ON" : "OFF";
        if (field == 1) std::snprintf(value, sizeof(value), "%d", sequence.steps);
        else if (field == 2) std::snprintf(value, sizeof(value), "%d", sequence.pulses);
        else if (field == 3) std::snprintf(value, sizeof(value), "%d", sequence.rotation);
        else if (field == 4) std::snprintf(value, sizeof(value), "%.0f%%", static_cast<double>(sequence.probability * 100.0F));
        else return slot.effects[3].kind == cd::EffectKind::granular_reverse ? "FX4 ACTIVE" : "A: INSTALL FX4";
    } else if (state.lab_tab == 3) {
        if (field == 0) return std::string{scene_name(session.performance.morph_target, ru(session))};
        if (field == 1) std::snprintf(value, sizeof(value), "%.0f%%", static_cast<double>(session.performance.morph * 100.0F));
        else return "A: COMMIT TARGET";
    } else {
        return "A: ENTER STAGE MODE";
    }
    return value;
}

void draw_lab(SDL_Renderer* renderer, const cd::Session& session, const UiState& state) {
    fill(renderer, {8, 46, 496, 324}, {8, 7, 12, 250});
    outline(renderer, {8, 46, 496, 324}, kInk);
    constexpr std::array<std::string_view, 5> tabs{"ACTOR", "MOD", "EVENT", "MORPH", "STAGE"};
    for (int tab = 0; tab < 5; ++tab) {
        const int x = 18 + tab * 96;
        if (tab == state.lab_tab) fill(renderer, {x, 58, 88, 20}, kPurple);
        cd::ui::draw_text(renderer, x + 5, 64, tabs[static_cast<std::size_t>(tab)],
            tab == state.lab_tab ? kInk : kDim);
    }
    cd::ui::draw_text(renderer, 20, 88,
        std::string{"ACTOR "} + std::to_string(state.slot + 1), kFxColors[static_cast<std::size_t>(state.slot)]);
    if (state.lab_tab == 1) {
        cd::ui::draw_text(renderer, 340, 88,
            std::string{"MOD "} + std::to_string(state.lab_modulator + 1) + "  X: NEXT", kInk);
    }
    constexpr std::array<std::array<std::string_view, 7>, 5> labels{{
        {"ENGINE", "MODEL", "OUTPUT", "SCALA", "ROOT", "", ""},
        {"ENABLED", "WAVE", "DEST", "RATE", "AMOUNT", "RATE SOURCE", "RATE AMOUNT"},
        {"ENABLED", "STEPS", "PULSES", "ROTATION", "PROBABILITY", "REVERSE GRAINS", ""},
        {"TARGET", "MORPH", "COMMIT", "", "", "", ""},
        {"PERFORMANCE VIEW", "", "", "", "", "", ""},
    }};
    const int count = lab_field_count(state.lab_tab);
    for (int field = 0; field < count; ++field) {
        const int y = 116 + field * 31;
        const bool selected = field == state.lab_field;
        if (selected) fill(renderer, {18, y - 5, 476, 25}, {73, 46, 104, 255});
        cd::ui::draw_text(renderer, 28, y,
            labels[static_cast<std::size_t>(state.lab_tab)][static_cast<std::size_t>(field)],
            selected ? kInk : kDim);
        const std::string value = lab_value(session, state, field);
        cd::ui::draw_text(renderer, 486 - cd::ui::text_width(value), y, value,
            selected ? kInk : kDim);
    }
    cd::ui::draw_text(renderer, 18, 346,
        "DPAD TAB/FIELD  L/R VALUE  A ACTION  Y ACTOR  B CLOSE", kDim);
}

void draw_stage(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state) {
    SDL_SetRenderDrawColor(renderer, 8, 7, 12, 255);
    SDL_RenderClear(renderer);
    cd::ui::draw_text(renderer, 16, 12, "CURSED DRONE / STAGE", kInk, 2);
    const std::string scene = std::string{scene_name(session.scene, ru(session))} + "  >  " +
        std::string{scene_name(session.performance.morph_target, ru(session))};
    cd::ui::draw_text(renderer, 18, 44, scene, kDim);
    for (int field = 0; field < 6; ++field) {
        const int y = 74 + field * 34;
        const bool selected = field == state.stage_field;
        if (selected) fill(renderer, {14, y - 5, 484, 28}, {73, 46, 104, 255});
        const std::string label = field < 5
            ? std::string{macro_name(field, ru(session))}
            : std::string{"MORPH"};
        const float value = field < 5
            ? macro_value(session.performance, field)
            : session.performance.morph;
        cd::ui::draw_text(renderer, 22, y, label, selected ? kInk : kDim);
        bar(renderer, 142, y, 344, 11, value,
            selected ? kInk : kFxColors[static_cast<std::size_t>(field % 4)]);
    }
    for (int actor = 0; actor < 4; ++actor) {
        const int x = 14 + actor * 122;
        const bool selected = actor == state.slot;
        if (selected) outline(renderer, {x, 282, 114, 54}, kInk);
        const auto& slot = session.slots[static_cast<std::size_t>(actor)];
        cd::ui::draw_text(renderer, x + 6, 290,
            std::to_string(actor + 1) + " " + std::string{engine_name(slot.engine, ru(session))},
            selected ? kInk : kDim);
        bar(renderer, x + 6, 311, 102, 10,
            std::clamp(telemetry.slot_rms[static_cast<std::size_t>(actor)] * 4.2F, 0.0F, 1.0F),
            kFxColors[static_cast<std::size_t>(actor)]);
        if (!slot.enabled) cd::ui::draw_text(renderer, x + 6, 326, "MUTE", kFxColors[0]);
    }
    cd::ui::draw_text(renderer, 14, 356,
        "UP/DN SELECT  L/R MOVE  Y ACTOR  A MUTE  SELECT FADE  B EXIT", kDim);
    SDL_RenderPresent(renderer);
}

void adjust_stage(cd::Session& session, UiState& state, int direction) {
    if (state.stage_field < 5) {
        float* value = macro_value(session.performance, state.stage_field);
        *value = std::clamp(*value + direction * 0.02F, 0.0F, 1.0F);
    } else {
        session.performance.morph = std::clamp(session.performance.morph + direction * 0.02F, 0.0F, 1.0F);
    }
}

'''
replace_once(path, 'void draw(\n', lab_block + 'void draw(\n')

replace_once(path,
'''    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 8, 7, 12, 255);''',
'''    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (state.stage_mode) {
        draw_stage(renderer, session, telemetry, state);
        return;
    }
    SDL_SetRenderDrawColor(renderer, 8, 7, 12, 255);''')
replace_once(path,
'''    if (state.picker != Picker::none) {
        draw_picker(renderer, session, state);
    }
    std::string help;''',
'''    if (state.picker != Picker::none) {
        draw_picker(renderer, session, state);
    }
    if (state.lab_open) {
        draw_lab(renderer, session, state);
    }
    std::string help;''')
replace_once(path,
'''    if (handheld()) {
        if (state.picker != Picker::none) {''',
'''    if (state.lab_open) {
        help.clear();
    } else if (handheld()) {
        if (state.picker != Picker::none) {''')

replace_once(path,
'''    if (state.picker != Picker::none) {
        state.picker = Picker::none;
    } else if (state.page == Page::perform && state.scene_track_focus) {''',
'''    if (state.lab_open) {
        state.lab_open = false;
    } else if (state.stage_mode) {
        state.stage_mode = false;
    } else if (state.picker != Picker::none) {
        state.picker = Picker::none;
    } else if (state.page == Page::perform && state.scene_track_focus) {''')

replace_once(path,
'''        const std::filesystem::path root{data_directory};
        autosave_path = root / "autosave.cdrone";
        legacy_autosave_path = root / "myldy20" / "cursed-drone" / "autosave.cdrone";''',
'''        const std::filesystem::path root{data_directory};
        autosave_path = root / "autosave.cdrone";
        legacy_autosave_path = root / "myldy20" / "cursed-drone" / "autosave.cdrone";
        g_scales = cd::load_scala_scales({root / "scales", std::filesystem::path{"assets/scales"}});''')
replace_once(path,
'''    } else if (char* preference_path = SDL_GetPrefPath("myldy20", "cursed-drone")) {
        autosave_path = std::filesystem::path{preference_path} / "autosave.cdrone";
        SDL_free(preference_path);
    }

    const std::filesystem::path load_path''',
'''    } else if (char* preference_path = SDL_GetPrefPath("myldy20", "cursed-drone")) {
        autosave_path = std::filesystem::path{preference_path} / "autosave.cdrone";
        g_scales = cd::load_scala_scales({std::filesystem::path{preference_path} / "scales",
            std::filesystem::path{"assets/scales"}});
        SDL_free(preference_path);
    }
    if (g_scales.empty()) g_scales.push_back(cd::equal_temperament_scale());

    const std::filesystem::path load_path''')

replace_once(path,
'''                if (state.picker != Picker::none) {
                    switch (event.cbutton.button) {''',
'''                if (state.picker != Picker::none) {
                    switch (event.cbutton.button) {''')
# Insert lab/stage branches between picker and normal controller handling.
replace_once(path,
'''                    default: break;
                    }
                } else {
                    switch (event.cbutton.button) {''',
'''                    default: break;
                    }
                } else if (state.lab_open) {
                    switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: navigate_lab(state, -1, 0); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: navigate_lab(state, 1, 0); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP: navigate_lab(state, 0, -1); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: navigate_lab(state, 0, 1); break;
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: adjust_lab(session, state, -1); changed = true; break;
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: adjust_lab(session, state, 1); changed = true; break;
                    case SDL_CONTROLLER_BUTTON_B: activate_lab(session, state); changed = true; break;
                    case SDL_CONTROLLER_BUTTON_A: state.lab_open = false; break;
                    case SDL_CONTROLLER_BUTTON_X:
                        if (state.lab_tab == 1) state.lab_modulator = (state.lab_modulator + 1) % 4;
                        break;
                    case SDL_CONTROLLER_BUTTON_Y: state.slot = (state.slot + 1) % 4; break;
                    default: break;
                    }
                } else if (state.stage_mode) {
                    switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_DPAD_UP: state.stage_field = (state.stage_field + 5) % 6; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: state.stage_field = (state.stage_field + 1) % 6; break;
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: adjust_stage(session, state, -1); changed = true; break;
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: adjust_stage(session, state, 1); changed = true; break;
                    case SDL_CONTROLLER_BUTTON_B:
                        session.slots[static_cast<std::size_t>(state.slot)].enabled =
                            !session.slots[static_cast<std::size_t>(state.slot)].enabled;
                        changed = true;
                        break;
                    case SDL_CONTROLLER_BUTTON_A: state.stage_mode = false; break;
                    case SDL_CONTROLLER_BUTTON_Y: state.slot = (state.slot + 1) % 4; break;
                    default: break;
                    }
                } else {
                    switch (event.cbutton.button) {''')
replace_once(path,
'''                    case SDL_CONTROLLER_BUTTON_START:
                        break;''',
'''                    case SDL_CONTROLLER_BUTTON_START:
                        state.lab_open = true;
                        state.lab_field = 0;
                        break;''')

# Add a compact visual for reverse grains by sharing the compound visual family.
replace_once(path,
'''        case cd::EffectKind::deep_sea:
            value = std::tanh((std::sin(t * 6.2831853F * (1.0F + effect.tone * 5.0F)) +''',
'''        case cd::EffectKind::deep_sea:
        case cd::EffectKind::granular_reverse:
            value = std::tanh((std::sin(t * 6.2831853F * (1.0F + effect.tone * 5.0F)) +''')

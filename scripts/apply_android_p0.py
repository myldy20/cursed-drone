#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def write(path: str, content: str) -> None:
    target = ROOT / path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(content, encoding="utf-8")


def replace_exact(path: str, old: str, new: str, expected: int = 1) -> None:
    content = read(path)
    count = content.count(old)
    if count != expected:
        raise RuntimeError(f"{path}: expected {expected} occurrences, found {count}: {old[:80]!r}")
    write(path, content.replace(old, new))


def replace_function(path: str, signature: str, replacement: str) -> None:
    content = read(path)
    start = content.find(signature)
    if start < 0:
        raise RuntimeError(f"{path}: function signature not found: {signature}")
    brace = content.find("{", start + len(signature))
    if brace < 0:
        raise RuntimeError(f"{path}: opening brace not found for {signature}")
    depth = 0
    end = None
    for index in range(brace, len(content)):
        char = content[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                end = index + 1
                break
    if end is None:
        raise RuntimeError(f"{path}: closing brace not found for {signature}")
    write(path, content[:start] + replacement.rstrip() + content[end:])


mapping_header = r'''// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.
#pragma once

#include <algorithm>
#include <cmath>

namespace cursed_drone::mapping {

constexpr float kMinimumFrequencyHz = 20.0F;
constexpr float kMaximumFrequencyHz = 2'000.0F;
constexpr float kMinimumModRateHz = 0.001F;
constexpr float kMaximumModRateHz = 40.0F;
constexpr float kMinimumFadeSeconds = 0.25F;
constexpr float kMaximumFadeSeconds = 30.0F;

inline float normalized(float value) noexcept {
    return std::clamp(value, 0.0F, 1.0F);
}

inline float normalize_log(float value, float minimum, float maximum) noexcept {
    value = std::clamp(value, minimum, maximum);
    return std::log(value / minimum) / std::log(maximum / minimum);
}

inline float denormalize_log(float value, float minimum, float maximum) noexcept {
    return minimum * std::pow(maximum / minimum, normalized(value));
}

inline float normalized_frequency(float value) noexcept {
    return normalize_log(value, kMinimumFrequencyHz, kMaximumFrequencyHz);
}

inline float frequency_from_normalized(float value) noexcept {
    return denormalize_log(value, kMinimumFrequencyHz, kMaximumFrequencyHz);
}

inline float normalized_mod_rate(float value) noexcept {
    return normalize_log(value, kMinimumModRateHz, kMaximumModRateHz);
}

inline float mod_rate_from_normalized(float value) noexcept {
    return denormalize_log(value, kMinimumModRateHz, kMaximumModRateHz);
}

inline float normalized_fade_seconds(float value) noexcept {
    return std::clamp(
        (value - kMinimumFadeSeconds) /
            (kMaximumFadeSeconds - kMinimumFadeSeconds),
        0.0F, 1.0F);
}

inline float fade_seconds_from_normalized(float value) noexcept {
    return kMinimumFadeSeconds +
        normalized(value) * (kMaximumFadeSeconds - kMinimumFadeSeconds);
}

inline float normalized_tuning_root(int value) noexcept {
    return static_cast<float>(std::clamp(value, 0, 127)) / 127.0F;
}

inline int tuning_root_from_normalized(float value) noexcept {
    return static_cast<int>(std::lround(normalized(value) * 127.0F));
}

inline float bipolar_from_normalized(float value) noexcept {
    return normalized(value) * 2.0F - 1.0F;
}

inline int next_rate_mod_source(int current, int modulator_index) noexcept {
    modulator_index = std::clamp(modulator_index, 0, 3);
    if (modulator_index == 0) {
        return -1;
    }
    if (current < -1 || current >= modulator_index) {
        current = -1;
    }
    return current + 1 >= modulator_index ? -1 : current + 1;
}

inline int picker_max_page(int item_count, int page_size) noexcept {
    if (item_count <= 0 || page_size <= 0) {
        return 0;
    }
    return (item_count - 1) / page_size;
}

inline int picker_next_page(int current, int item_count, int page_size) noexcept {
    return std::min(std::max(0, current) + 1,
        picker_max_page(item_count, page_size));
}

inline int picker_previous_page(int current) noexcept {
    return std::max(0, current - 1);
}

} // namespace cursed_drone::mapping
'''
write("include/cursed_drone/parameter_mapping.hpp", mapping_header)

mapping_tests = r'''// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.
#include "cursed_drone/parameter_mapping.hpp"

#include <cmath>
#include <iostream>

namespace map = cursed_drone::mapping;

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool near(float left, float right, float tolerance = 0.0001F) {
    return std::abs(left - right) <= tolerance;
}

} // namespace

int main() {
    expect(near(map::frequency_from_normalized(0.0F), 20.0F),
        "frequency lower endpoint");
    expect(near(map::frequency_from_normalized(1.0F), 2'000.0F, 0.01F),
        "frequency upper endpoint");
    expect(near(map::frequency_from_normalized(
        map::normalized_frequency(440.0F)), 440.0F, 0.01F),
        "frequency mapping should roundtrip");

    expect(near(map::mod_rate_from_normalized(0.0F), 0.001F),
        "mod-rate lower endpoint");
    expect(near(map::mod_rate_from_normalized(1.0F), 40.0F, 0.001F),
        "mod-rate upper endpoint");
    expect(near(map::mod_rate_from_normalized(
        map::normalized_mod_rate(2.5F)), 2.5F, 0.001F),
        "mod-rate mapping should roundtrip");

    expect(near(map::fade_seconds_from_normalized(0.0F), 0.25F),
        "fade lower endpoint");
    expect(near(map::fade_seconds_from_normalized(1.0F), 30.0F),
        "fade upper endpoint");
    expect(near(map::fade_seconds_from_normalized(
        map::normalized_fade_seconds(8.25F)), 8.25F, 0.001F),
        "fade mapping should roundtrip");

    expect(map::tuning_root_from_normalized(0.0F) == 0,
        "tuning root lower endpoint");
    expect(map::tuning_root_from_normalized(1.0F) == 127,
        "tuning root upper endpoint");
    expect(map::tuning_root_from_normalized(
        map::normalized_tuning_root(36)) == 36,
        "tuning root mapping should roundtrip");

    expect(near(map::bipolar_from_normalized(0.0F), -1.0F),
        "bipolar lower endpoint");
    expect(near(map::bipolar_from_normalized(0.5F), 0.0F),
        "bipolar midpoint");
    expect(near(map::bipolar_from_normalized(1.0F), 1.0F),
        "bipolar upper endpoint");

    expect(map::next_rate_mod_source(-1, 0) == -1,
        "first modulator cannot have a rate source");
    expect(map::next_rate_mod_source(-1, 1) == 0 &&
        map::next_rate_mod_source(0, 1) == -1,
        "second modulator cycles only none and MOD 1");
    expect(map::next_rate_mod_source(-1, 3) == 0 &&
        map::next_rate_mod_source(0, 3) == 1 &&
        map::next_rate_mod_source(1, 3) == 2 &&
        map::next_rate_mod_source(2, 3) == -1,
        "fourth modulator cycles only earlier modulators");
    expect(map::next_rate_mod_source(3, 2) == 0,
        "invalid future source should be sanitized");

    expect(map::picker_max_page(21, 8) == 2,
        "picker max page");
    expect(map::picker_next_page(2, 21, 8) == 2,
        "picker next must clamp at final page");
    expect(map::picker_previous_page(0) == 0,
        "picker previous must clamp at first page");

    if (failures == 0) {
        std::cout << "All parameter mapping tests passed\n";
        return 0;
    }
    std::cerr << failures << " parameter mapping test(s) failed\n";
    return 1;
}
'''
write("tests/parameter_mapping_tests.cpp", mapping_tests)

replace_exact(
    "CMakeLists.txt",
    '''        add_executable(cursed-drone-tests tests/test_main.cpp)
        target_link_libraries(cursed-drone-tests PRIVATE cursed_drone_core)
        add_test(NAME cursed-drone-tests COMMAND cursed-drone-tests)
''',
    '''        add_executable(cursed-drone-tests tests/test_main.cpp)
        target_link_libraries(cursed-drone-tests PRIVATE cursed_drone_core)
        add_test(NAME cursed-drone-tests COMMAND cursed-drone-tests)

        add_executable(cursed-drone-parameter-mapping-tests
            tests/parameter_mapping_tests.cpp)
        target_link_libraries(cursed-drone-parameter-mapping-tests
            PRIVATE cursed_drone_core)
        add_test(NAME cursed-drone-parameter-mapping-tests
            COMMAND cursed-drone-parameter-mapping-tests)
''')

replace_exact(
    "android/app/src/main/cpp/android_touch_main.cpp",
    '#include "cursed_drone/audio.hpp"\n',
    '#include "cursed_drone/audio.hpp"\n#include "cursed_drone/parameter_mapping.hpp"\n')

replace_function(
    "android/app/src/main/cpp/android_touch_main.cpp",
    "float normalized_frequency(float hz) noexcept",
    '''float normalized_frequency(float hz) noexcept {
    return cd::mapping::normalized_frequency(hz);
}''')
replace_function(
    "android/app/src/main/cpp/android_touch_main.cpp",
    "float frequency_from_normalized(float normalized) noexcept",
    '''float frequency_from_normalized(float normalized) noexcept {
    return cd::mapping::frequency_from_normalized(normalized);
}''')
replace_function(
    "android/app/src/main/cpp/android_touch_main.cpp",
    "float normalized_rate(float rate) noexcept",
    '''float normalized_rate(float rate) noexcept {
    return cd::mapping::normalized_mod_rate(rate);
}''')
replace_function(
    "android/app/src/main/cpp/android_touch_main.cpp",
    "float rate_from_normalized(float normalized) noexcept",
    '''float rate_from_normalized(float normalized) noexcept {
    return cd::mapping::mod_rate_from_normalized(normalized);
}''')

replace_exact(
    "android/app/src/main/cpp/android_touch_main.cpp",
    "case SliderKind::tuning_root: slot.tuning.root_midi = static_cast<int>(std::lround(normalized * 127.0F)); break;",
    "case SliderKind::tuning_root: slot.tuning.root_midi = cd::mapping::tuning_root_from_normalized(normalized); break;")
replace_exact(
    "android/app/src/main/cpp/android_touch_main.cpp",
    "case SliderKind::mod_depth: mod.depth = normalized; break;",
    "case SliderKind::mod_depth: mod.depth = cd::mapping::bipolar_from_normalized(normalized); break;")
replace_exact(
    "android/app/src/main/cpp/android_touch_main.cpp",
    "case SliderKind::fade_in: session.fade_in_seconds = 0.2F + normalized * 19.8F; break;",
    "case SliderKind::fade_in: session.fade_in_seconds = cd::mapping::fade_seconds_from_normalized(normalized); break;")
replace_exact(
    "android/app/src/main/cpp/android_touch_main.cpp",
    "case SliderKind::fade_out: session.fade_out_seconds = 0.2F + normalized * 19.8F; break;",
    "case SliderKind::fade_out: session.fade_out_seconds = cd::mapping::fade_seconds_from_normalized(normalized); break;")

replace_exact(
    "android/app/src/main/cpp/android_touch_main.cpp",
    '''    case Action::mod_source_cycle: {
        auto& source = session.slots[static_cast<std::size_t>(state.actor)]
            .modulators[static_cast<std::size_t>(state.modulator)].rate_mod_source;
        source = source >= 3 ? -1 : source + 1; return true;
    }
''',
    '''    case Action::mod_source_cycle: {
        auto& source = session.slots[static_cast<std::size_t>(state.actor)]
            .modulators[static_cast<std::size_t>(state.modulator)].rate_mod_source;
        source = cd::mapping::next_rate_mod_source(source, state.modulator);
        session.scene_modified = true;
        return true;
    }
''')

replace_exact(
    "android/app/src/main/cpp/android_touch_main.cpp",
    '''    case Action::picker_previous: state.picker_page = std::max(0, state.picker_page - 1); return false;
    case Action::picker_next: state.picker_page += 1; return false;
''',
    '''    case Action::picker_previous:
        state.picker_page = cd::mapping::picker_previous_page(state.picker_page);
        return false;
    case Action::picker_next:
        state.picker_page = cd::mapping::picker_next_page(
            state.picker_page, picker_count(state.picker), 8);
        return false;
''')

content = read("android/app/src/main/cpp/android_touch_main.cpp")
content = content.replace(
    "(session.fade_in_seconds - 0.2F) / 19.8F",
    "cd::mapping::normalized_fade_seconds(session.fade_in_seconds)")
content = content.replace(
    "(session.fade_out_seconds - 0.2F) / 19.8F",
    "cd::mapping::normalized_fade_seconds(session.fade_out_seconds)")
write("android/app/src/main/cpp/android_touch_main.cpp", content)

replace_exact(
    "android/app/src/main/cpp/android_approved_main.cpp",
    '#define SDL_main cursed_drone_legacy_android_main\n',
    '#include "cursed_drone/parameter_mapping.hpp"\n\n#define SDL_main cursed_drone_legacy_android_main\n')

replace_exact(
    "android/app/src/main/cpp/android_approved_main.cpp",
    '''        if (state.pending_trigger >= 0) {
            audio.graph.trigger_slot(static_cast<std::size_t>(
                state.pending_trigger));
            set_toast(state, ru(session) ? "СОБЫТИЕ ЗАПУЩЕНО" :
                "EVENT TRIGGERED");
            state.pending_trigger = -1;
        }
''',
    '''        if (state.pending_trigger >= 0) {
            audio.graph.trigger_slot(static_cast<std::size_t>(
                state.pending_trigger));
            set_toast(state, ru(session) ? "СОБЫТИЕ ЗАПУЩЕНО" :
                "EVENT TRIGGERED");
            state.pending_trigger = -1;
        }
        if (g_panic_requested) {
            audio.graph.panic();
            set_toast(state, ru(session) ? "ЗВУК СБРОШЕН" :
                "KILL SILENCE");
            g_panic_requested = false;
        }
''')

compat = r'''constexpr int kSourceLandscape = 90;
constexpr int kSourceMusical = 91;
constexpr int kActionPanic = 94;
bool g_panic_requested = false;

void restore_landscape_actor(cd::Session& session, int actor) {
    cd::Session recipe = session;
    cd::apply_scene_recipe(recipe, session.scene);
    session.slots[static_cast<std::size_t>(actor)] =
        recipe.slots[static_cast<std::size_t>(actor)];
    session.scene_modified = true;
}
'''
write("android/app/src/main/cpp/approved_ui_compat.inc", compat)

replace_exact(
    "android/app/src/main/cpp/approved_ui_primitives.inc",
    '''constexpr int kSourceLandscape = 90;
constexpr int kSourceMusical = 91;
''',
    '''bool a_compact_layout(int width, int height) noexcept {
    return width < 1'600 || height < 850;
}
''')

header_function = r'''void a_header(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, float cpu, int width, int height,
    int scale, int& content_y) {
    const bool compact = a_compact_layout(width, height);
    const int brand_h = compact ? 64 : std::max(78, height / 12);
    const int nav_h = compact ? 54 : std::max(66, height / 15);
    const int margin = compact ? 18 : 30;
    fill(renderer, {0, 0, width, brand_h + nav_h + 8}, aTop);
    const int brand_x = compact ? 20 : 34;
    text(renderer, brand_x, compact ? 16 : 20, "CURSED DRONE", kInk,
        scale + 1);
    text(renderer, brand_x + cd::ui::text_width("CURSED DRONE", scale + 1) +
        (compact ? 14 : 26), compact ? 23 : 30,
        std::string{"V"} + CURSED_DRONE_VERSION, kDim,
        std::max(1, scale - 1));

    const int status_w = compact ? std::min(450, width / 2) :
        std::max(460, width / 4);
    const int status_x = width - status_w - margin;
    const int gap = compact ? 8 : 12;
    const int fade_w = compact ? std::max(150, status_w * 38 / 100) :
        std::max(190, status_w * 2 / 5);
    const int kill_w = compact ? 90 : 116;
    SDL_Rect fade{status_x, compact ? 10 : 14, fade_w,
        brand_h - (compact ? 20 : 28)};
    a_panel(renderer, fade, state.fade_active ? aActive : aPanel2, kPurple);
    text(renderer, fade.x + (compact ? 10 : 18), fade.y + 12,
        ru(session) ? "ФЕЙД" : "FADE", kInk, scale);
    const std::string fade_value = percent(session.performance.fade);
    text(renderer, fade.x + fade.w - (compact ? 10 : 18) -
        cd::ui::text_width(fade_value, scale), fade.y + 12,
        fade_value, kGreen, scale);
    add_hit(state, fade, Action::fade);

    SDL_Rect kill{fade.x + fade.w + gap, fade.y, kill_w, fade.h};
    a_button(renderer, state, kill, "KILL", Action::actor_section,
        kActionPanic, 0, scale, true, kRed);

    const int mx = kill.x + kill.w + gap;
    text(renderer, mx, compact ? 12 : 18, "DSP", kDim,
        std::max(1, scale - 1));
    const std::string cpu_text = std::to_string(
        static_cast<int>(std::lround(cpu * 100.0F))) + "%";
    text(renderer, width - margin - cd::ui::text_width(cpu_text,
        std::max(1, scale - 1)), compact ? 12 : 18, cpu_text, kDim,
        std::max(1, scale - 1));
    constexpr int segments = 12;
    const int meter_gap = compact ? 3 : 5;
    const int meter_w = std::max(segments,
        width - mx - margin);
    const int seg_w = std::max(1,
        (meter_w - meter_gap * (segments - 1)) / segments);
    const float out = std::clamp(telemetry.master_rms * 4.0F, 0.0F, 1.0F);
    for (int i = 0; i < segments; ++i) {
        const float t = static_cast<float>(i + 1) / segments;
        const SDL_Color color = t <= out ? (t > 0.82F ? kRed :
            (t > 0.65F ? kActorColors[1] : kGreen)) : aTrack;
        fill(renderer, {mx + i * (seg_w + meter_gap),
            compact ? 42 : 49, seg_w, compact ? 13 : 17}, color);
    }

    constexpr std::array<Page, 5> pages{{Page::place, Page::actor,
        Page::fx, Page::master, Page::memory}};
    const int left = compact ? 18 : 30;
    const int tab_gap = compact ? 8 : 12;
    const int tab_w = (width - 2 * left - 4 * tab_gap) / 5;
    for (int i = 0; i < 5; ++i) {
        SDL_Rect tab{left + i * (tab_w + tab_gap), brand_h, tab_w, nav_h};
        const bool active = pages[static_cast<std::size_t>(i)] == state.page;
        a_panel(renderer, tab, active ? aActive : aPanel,
            active ? kInk : kBorder);
        centered_text(renderer, tab,
            page_name(pages[static_cast<std::size_t>(i)], ru(session)),
            active ? kInk : kDim, scale);
        add_hit(state, tab, Action::page,
            static_cast<int>(pages[static_cast<std::size_t>(i)]));
    }
    content_y = brand_h + nav_h + 14;
}'''
replace_function(
    "android/app/src/main/cpp/approved_ui_primitives.inc",
    "void a_header(SDL_Renderer* renderer, cd::Session& session, UiState& state,",
    header_function)

actor_card_function = r'''void a_actor_card(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, const cd::AudioTelemetry& telemetry, SDL_Rect rect,
    int actor, int scale, bool selected, bool open_actor) {
    const auto& slot = session.slots[static_cast<std::size_t>(actor)];
    const SDL_Color color = aActor[static_cast<std::size_t>(actor)];
    a_panel(renderer, rect, selected ? aPanel2 : aPanel,
        selected ? color : kBorder);
    text(renderer, rect.x + 14, rect.y + 12, std::to_string(actor + 1),
        color, scale);
    text(renderer, rect.x + 14 + 3 * 8 * scale, rect.y + 12,
        engine_name(slot.engine, ru(session)), slot.enabled ? kInk : kDim,
        scale);
    if (telemetry.slot_event[static_cast<std::size_t>(actor)] > 0.04F)
        fill(renderer, {rect.x + rect.w - 30, rect.y + 14, 16, 16}, color);

    const bool compact = rect.h < 132;
    const int mute_h = compact ? std::max(34, rect.h / 3) :
        std::max(40, rect.h / 5);
    SDL_Rect mute{rect.x + 12, rect.y + rect.h - mute_h - 10,
        rect.w - 24, mute_h};
    if (compact) {
        const int meter_y = rect.y + std::max(38, rect.h / 2 - 8);
        a_meter(renderer, {rect.x + 14, meter_y, rect.w - 28, 10},
            telemetry.slot_rms[static_cast<std::size_t>(actor)] * 4.2F,
            color);
        a_panel(renderer, mute, slot.enabled ? aPanel : kRed,
            slot.enabled ? color : kRed);
        centered_text(renderer, mute,
            slot.enabled ? (ru(session) ? "ЗАГЛУШИТЬ" : "MUTE") :
                (ru(session) ? "ВКЛЮЧИТЬ" : "UNMUTE"),
            slot.enabled ? kDim : kInk, std::max(1, scale - 1));
        add_hit(state, mute, Action::actor_toggle, actor);
        add_hit(state, {rect.x, rect.y, rect.w,
            std::max(1, mute.y - rect.y - 4)}, Action::actor_select,
            actor, open_actor ? 1 : 0);
        return;
    }

    const int level_y = rect.y + rect.h - mute_h - 58;
    text(renderer, rect.x + 14, level_y,
        ru(session) ? "УРОВЕНЬ" : "LEVEL", kDim,
        std::max(1, scale - 1));
    const std::string level = percent(slot.level);
    text(renderer, rect.x + rect.w - 14 - cd::ui::text_width(level,
        std::max(1, scale - 1)), level_y, level, color,
        std::max(1, scale - 1));
    SDL_Rect bar{rect.x + 14, level_y + 10 * scale, rect.w - 28, 12};
    a_meter(renderer, bar, slot.level, color);
    add_hit(state, {bar.x - 4, level_y - 4, bar.w + 8, 42},
        Action::slider, actor, 0, SliderKind::actor_level);

    a_panel(renderer, mute, slot.enabled ? aPanel : kRed,
        slot.enabled ? color : kRed);
    centered_text(renderer, mute,
        slot.enabled ? (ru(session) ? "ЗАГЛУШИТЬ" : "MUTE") :
            (ru(session) ? "ВКЛЮЧИТЬ" : "UNMUTE"),
        slot.enabled ? kDim : kInk, std::max(1, scale - 1));
    add_hit(state, mute, Action::actor_toggle, actor);
    add_hit(state, {rect.x, rect.y, rect.w,
        std::max(1, level_y - rect.y - 5)}, Action::actor_select,
        actor, open_actor ? 1 : 0);
}'''
replace_function(
    "android/app/src/main/cpp/approved_ui_primitives.inc",
    "void a_actor_card(SDL_Renderer* renderer, cd::Session& session,",
    actor_card_function)

actor_sound_function = r'''void a_actor_sound(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect area, int scale) {
    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    const bool compact = area.w < 920 || area.h < 440;
    const int gap = compact ? 6 : 10;
    const int left_w = static_cast<int>(area.w * (compact ? 0.60F : 0.65F));
    SDL_Rect left{area.x, area.y, left_w - gap, area.h};
    SDL_Rect right{area.x + left_w, area.y, area.w - left_w, area.h};
    a_panel(renderer, left, aPanel2);
    a_panel(renderer, right, aPanel2);
    const std::array<std::string, 7> labels{{
        ru(session) ? "ЧАСТОТА" : "FREQUENCY",
        ru(session) ? "ТЕМБР" : "TIMBRE",
        ru(session) ? "ЦВЕТ" : "COLOR",
        ru(session) ? "ДВИЖЕНИЕ" : "MOTION",
        ru(session) ? "ТЕКСТУРА" : "TEXTURE",
        ru(session) ? "УРОВЕНЬ" : "LEVEL",
        ru(session) ? "ПАНОРАМА" : "PAN"}};
    const std::array<float, 7> norms{{
        cd::mapping::normalized_frequency(slot.frequency_hz),
        slot.timbre, slot.color, slot.motion, slot.texture, slot.level,
        (slot.pan + 1.0F) * 0.5F}};
    const std::array<SliderKind, 7> kinds{{SliderKind::actor_frequency,
        SliderKind::actor_timbre, SliderKind::actor_color,
        SliderKind::actor_motion, SliderKind::actor_texture,
        SliderKind::actor_level, SliderKind::actor_pan}};
    const std::array<SDL_Color, 7> colors{{kGreen, kActorColors[1],
        aYellow, kActorColors[3], kPurple, kGreen, kDim}};
    const std::array<std::string, 7> values{{
        decimal(slot.frequency_hz, "HZ"), percent(slot.timbre),
        percent(slot.color), percent(slot.motion), percent(slot.texture),
        percent(slot.level), signed_percent(slot.pan)}};
    const int row_gap = compact ? 4 : 7;
    const int row_h = std::max(compact ? 28 : 38,
        (left.h - 18 - 6 * row_gap) / 7);
    for (int i = 0; i < 7; ++i)
        a_slider(renderer, state,
            {left.x + 9, left.y + 9 + i * (row_h + row_gap),
                left.w - 18, row_h},
            labels[static_cast<std::size_t>(i)],
            values[static_cast<std::size_t>(i)],
            norms[static_cast<std::size_t>(i)],
            kinds[static_cast<std::size_t>(i)], 0, 0, scale,
            colors[static_cast<std::size_t>(i)]);

    text(renderer, right.x + 14, right.y + (compact ? 10 : 14),
        ru(session) ? "СТРОЙ" : "TUNING", kInk, scale);
    const int button_h = compact ? std::max(40, right.h / 11) :
        std::max(64, right.h / 9);
    const int first_y = right.y + (compact ? 36 : 48);
    a_button(renderer, state, {right.x + 12, first_y,
        right.w - 24, button_h}, std::string{slot.tuning.name.data()},
        Action::actor_section, 99, 0, scale, true, kPurple);
    a_slider(renderer, state, {right.x + 12, first_y + button_h + gap,
        right.w - 24, button_h}, ru(session) ? "КОРЕНЬ MIDI" : "ROOT MIDI",
        std::to_string(slot.tuning.root_midi),
        cd::mapping::normalized_tuning_root(slot.tuning.root_midi),
        SliderKind::tuning_root, 0, 0, scale, kGreen);
    int y = first_y + 2 * (button_h + gap);
    if (slot.engine == cd::EngineKind::plaits) {
        a_button(renderer, state, {right.x + 12, y, right.w - 24, button_h},
            plaits_model_name(slot.plaits_model), Action::actor_section,
            97, 0, scale, true, kActorColors[1]);
        y += button_h + gap;
        a_button(renderer, state, {right.x + 12, y, right.w - 24, button_h},
            std::string{"OUTPUT: "} + output_name(slot.plaits_output),
            Action::actor_section, 98, 0, scale, true, kActorColors[3]);
    } else {
        a_panel(renderer, {right.x + 12, y, right.w - 24,
            std::max(1, right.y + right.h - y - 12)}, aPanel, kBorder);
        text(renderer, right.x + 24, y + (compact ? 14 : 22),
            ru(session) ? "ЛАНДШАФТНЫЙ ДВИЖОК" : "LANDSCAPE ENGINE",
            kDim, std::max(1, scale - 1));
        text(renderer, right.x + 24, y + (compact ? 40 : 58),
            engine_name(slot.engine, ru(session)),
            aActor[static_cast<std::size_t>(state.actor)], scale);
    }
}'''
replace_function(
    "android/app/src/main/cpp/approved_ui_actor.inc",
    "void a_actor_sound(SDL_Renderer* renderer, cd::Session& session,",
    actor_sound_function)

actor_events_function = r'''void a_actor_events(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect area, int scale) {
    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    const bool compact = area.w < 920 || area.h < 440;
    const bool has_rate = cd::supports_event_rate(slot.engine);
    const bool can_trigger = cd::supports_manual_trigger(slot.engine);
    const int gap = compact ? 6 : 12;
    const int half = (area.w - gap) / 2;
    SDL_Rect left{area.x, area.y, half, area.h};
    SDL_Rect right{area.x + half + gap, area.y, area.w - half - gap,
        area.h};
    a_panel(renderer, left, aPanel2);
    a_panel(renderer, right, aPanel2);
    text(renderer, left.x + 16, left.y + 16,
        ru(session) ? "СОБЫТИЯ АКТЕРА" : "ACTOR EVENTS", kInk, scale);

    if (has_rate) {
        const float density = cd::effective_event_density(slot.event_density,
            session.performance.events);
        const float rate = cd::event_rate_hz(session.tempo_bpm, density,
            slot.motion);
        const float wait = cd::event_max_wait_seconds(session.tempo_bpm,
            density, slot.motion);
        const int slider_h = compact ? std::max(70, left.h / 4) :
            std::max(108, left.h / 4);
        a_slider(renderer, state, {left.x + 12, left.y + 52,
            left.w - 24, slider_h},
            ru(session) ? "ЧАСТОТА СОБЫТИЙ" : "EVENT RATE",
            decimal(rate, "/S"), slot.event_density,
            SliderKind::actor_event_density, 0, 0, scale,
            kActorColors[1], ru(session) ? "РЕДКО" : "SPARSE",
            ru(session) ? "ЧАСТО" : "BUSY");
        SDL_Rect wait_box{left.x + 12, left.y + 52 + slider_h + gap,
            left.w - 24, compact ? 50 : std::max(72, left.h / 8)};
        a_panel(renderer, wait_box, aPanel, kBorder);
        text(renderer, wait_box.x + 16, wait_box.y + (compact ? 14 : 20),
            ru(session) ? "МАКС. ОЖИДАНИЕ" : "MAX WAIT", kDim, scale);
        const std::string wait_text = decimal(wait, "S");
        text(renderer, wait_box.x + wait_box.w - 16 -
            cd::ui::text_width(wait_text, scale),
            wait_box.y + (compact ? 14 : 20), wait_text,
            kActorColors[1], scale);
    } else {
        SDL_Rect info{left.x + 12, left.y + 52, left.w - 24,
            compact ? 88 : std::max(118, left.h / 3)};
        a_panel(renderer, info, aPanel, kBorder);
        text(renderer, info.x + 16, info.y + 18,
            can_trigger ? (ru(session) ? "МУЗЫКАЛЬНЫЙ ТРИГГЕР" :
                "MUSICAL TRIGGER") :
                (ru(session) ? "НЕПРЕРЫВНЫЙ АКТЕР" :
                "CONTINUOUS ACTOR"), kInk, scale);
        text(renderer, info.x + 16, info.y + 18 + 12 * scale,
            can_trigger ? (ru(session) ? "ЧАСТОТА ЗАДАЕТСЯ EUCLIDEAN" :
                "TIMING COMES FROM EUCLIDEAN") :
                (ru(session) ? "ЧАСТОТА СОБЫТИЙ НЕ ПРИМЕНЯЕТСЯ" :
                "EVENT RATE DOES NOT APPLY"), kDim,
            std::max(1, scale - 1));
    }

    if (can_trigger)
        a_button(renderer, state, {left.x + 12,
            left.y + left.h - (compact ? 54 :
                std::max(82, left.h / 7)) - 12,
            left.w - 24, compact ? 54 : std::max(82, left.h / 7)},
            ru(session) ? "ЗАПУСТИТЬ СОБЫТИЕ" : "TRIGGER EVENT",
            Action::actor_trigger, state.actor, 0, scale + (compact ? 0 : 1),
            true, kActorColors[1]);

    text(renderer, right.x + 16, right.y + 16, "EUCLIDEAN", kInk, scale);
    if (!can_trigger) {
        SDL_Rect info{right.x + 12, right.y + 52, right.w - 24,
            right.h - 64};
        a_panel(renderer, info, aPanel, kBorder);
        centered_text(renderer, info,
            ru(session) ? "ДЛЯ НЕПРЕРЫВНОГО ДВИЖКА НЕ ПРИМЕНЯЕТСЯ" :
                "NOT USED BY THIS CONTINUOUS ENGINE",
            kDim, scale);
        return;
    }

    const int toggle_h = compact ? 48 : std::max(64, right.h / 9);
    a_button(renderer, state, {right.x + 12, right.y + 52,
        right.w - 24, toggle_h}, slot.euclidean.enabled ?
        (ru(session) ? "ВКЛЮЧЕНО" : "ENABLED") :
        (ru(session) ? "ВЫКЛЮЧЕНО" : "DISABLED"),
        Action::euclidean_toggle, 0, 0, scale,
        slot.euclidean.enabled, kGreen);
    const int top = right.y + 52 + toggle_h + gap;
    const int row_h = std::max(compact ? 34 : 48,
        (right.y + right.h - top - 12 - 3 * gap) / 4);
    a_slider(renderer, state, {right.x + 12, top, right.w - 24, row_h},
        ru(session) ? "ШАГИ" : "STEPS", std::to_string(slot.euclidean.steps),
        static_cast<float>(slot.euclidean.steps - 1) / 31.0F,
        SliderKind::euclidean_steps, 0, 0, scale, kGreen);
    a_slider(renderer, state, {right.x + 12, top + row_h + gap,
        right.w - 24, row_h}, ru(session) ? "ИМПУЛЬСЫ" : "PULSES",
        std::to_string(slot.euclidean.pulses),
        static_cast<float>(slot.euclidean.pulses) /
            static_cast<float>(std::max(1, slot.euclidean.steps)),
        SliderKind::euclidean_pulses, 0, 0, scale, kActorColors[1]);
    a_slider(renderer, state, {right.x + 12, top + 2 * (row_h + gap),
        right.w - 24, row_h}, ru(session) ? "СДВИГ" : "ROTATION",
        std::to_string(slot.euclidean.rotation),
        static_cast<float>(slot.euclidean.rotation) /
            static_cast<float>(std::max(1, slot.euclidean.steps - 1)),
        SliderKind::euclidean_rotation, 0, 0, scale, kActorColors[3]);
    a_slider(renderer, state, {right.x + 12, top + 3 * (row_h + gap),
        right.w - 24, row_h}, ru(session) ? "ВЕРОЯТНОСТЬ" : "PROBABILITY",
        percent(slot.euclidean.probability), slot.euclidean.probability,
        SliderKind::euclidean_probability, 0, 0, scale, kPurple);
}'''
replace_function(
    "android/app/src/main/cpp/approved_ui_actor.inc",
    "void a_actor_events(SDL_Renderer* renderer, cd::Session& session,",
    actor_events_function)

actor_mod_function = r'''void a_actor_mod(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect area, int scale) {
    auto& mod = session.slots[static_cast<std::size_t>(state.actor)]
        .modulators[static_cast<std::size_t>(state.modulator)];
    const bool compact = area.w < 920 || area.h < 440;
    const int gap = compact ? 6 : 12;
    const int select_h = compact ? 48 : std::max(66, area.h / 9);
    const int select_w = (area.w - 3 * gap) / 4;
    for (int i = 0; i < 4; ++i)
        a_button(renderer, state, {area.x + i * (select_w + gap), area.y,
            select_w, select_h}, std::string{"MOD "} + std::to_string(i + 1),
            Action::mod_select, i, 0, scale, i == state.modulator,
            aActor[static_cast<std::size_t>(i)]);
    const int top = area.y + select_h + gap;
    const int left_w = static_cast<int>(area.w * (compact ? 0.42F : 0.38F));
    SDL_Rect left{area.x, top, left_w - gap, area.y + area.h - top};
    SDL_Rect right{area.x + left_w, top, area.w - left_w,
        area.y + area.h - top};
    a_panel(renderer, left, aPanel2);
    a_panel(renderer, right, aPanel2);
    const int button_h = compact ? 42 : std::max(62, left.h / 8);
    a_button(renderer, state, {left.x + 12, left.y + 12,
        left.w - 24, button_h}, mod.enabled ?
        (ru(session) ? "МОДУЛЯТОР ВКЛ" : "MODULATOR ON") :
        (ru(session) ? "МОДУЛЯТОР ВЫКЛ" : "MODULATOR OFF"),
        Action::mod_toggle, 0, 0, scale, mod.enabled, kGreen);
    a_button(renderer, state, {left.x + 12,
        left.y + 12 + button_h + gap, left.w - 24, button_h},
        wave_name(mod.wave, ru(session)), Action::actor_section,
        95, 0, scale, true, kActorColors[1]);
    a_button(renderer, state, {left.x + 12,
        left.y + 12 + 2 * (button_h + gap), left.w - 24, button_h},
        destination_name(mod.destination, ru(session)), Action::actor_section,
        96, 0, scale, true, kActorColors[3]);
    const std::string source = mod.rate_mod_source < 0 ?
        (ru(session) ? "ИСТОЧНИК: НЕТ" : "SOURCE: NONE") :
        std::string{"SOURCE: MOD "} + std::to_string(mod.rate_mod_source + 1);
    const int source_h = compact ? 48 : std::max(66, left.h / 7);
    a_button(renderer, state, {left.x + 12,
        left.y + left.h - source_h - 12, left.w - 24, source_h},
        source, Action::mod_source_cycle, 0, 0, scale, true, kPurple);
    const int row_h = std::max(compact ? 42 : 54,
        (right.h - 24 - 3 * gap) / 4);
    a_slider(renderer, state, {right.x + 12, right.y + 12,
        right.w - 24, row_h}, ru(session) ? "СКОРОСТЬ" : "RATE",
        decimal(mod.rate_hz, "HZ"),
        cd::mapping::normalized_mod_rate(mod.rate_hz),
        SliderKind::mod_rate, 0, 0, scale, kGreen);
    a_slider(renderer, state, {right.x + 12,
        right.y + 12 + row_h + gap, right.w - 24, row_h},
        ru(session) ? "ГЛУБИНА" : "DEPTH",
        signed_percent(mod.depth), (mod.depth + 1.0F) * 0.5F,
        SliderKind::mod_depth, 0, 0, scale, kActorColors[1]);
    a_slider(renderer, state, {right.x + 12,
        right.y + 12 + 2 * (row_h + gap), right.w - 24, row_h},
        ru(session) ? "СМЕЩЕНИЕ" : "OFFSET",
        signed_percent(mod.offset), (mod.offset + 1.0F) * 0.5F,
        SliderKind::mod_offset, 0, 0, scale, kActorColors[3]);
    a_slider(renderer, state, {right.x + 12,
        right.y + 12 + 3 * (row_h + gap), right.w - 24, row_h},
        ru(session) ? "КРОСС-МОД" : "CROSS MOD",
        signed_percent(mod.rate_mod_amount),
        (mod.rate_mod_amount + 1.0F) * 0.5F,
        SliderKind::mod_cross, 0, 0, scale, kPurple);
}'''
replace_function(
    "android/app/src/main/cpp/approved_ui_actor.inc",
    "void a_actor_mod(SDL_Renderer* renderer, cd::Session& session,",
    actor_mod_function)

actor_function = r'''void a_actor(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, SDL_Rect area, int scale) {
    const int gap = a_compact_layout(area.w, area.h) ? 8 : 12;
    const bool compact = area.w < 1'450 || area.h < 680;
    const int side_w = compact ? std::clamp(area.w / 5, 220, 270) :
        std::max(310, area.w / 5);
    SDL_Rect side{area.x, area.y, side_w, area.h};
    SDL_Rect main{area.x + side_w + gap, area.y,
        area.w - side_w - gap, area.h};
    a_actor_sidebar(renderer, session, state, telemetry, side, scale);
    a_panel(renderer, main);
    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    const SDL_Color color = aActor[static_cast<std::size_t>(state.actor)];
    const int header_h = compact ? 122 : std::max(112, main.h / 7);
    text(renderer, main.x + 20, main.y + 16,
        std::to_string(state.actor + 1), color, scale + 1);
    text(renderer, main.x + 20 + 4 * 8 * (scale + 1), main.y + 16,
        engine_name(slot.engine, ru(session)), kInk, scale + 1);

    if (compact) {
        const int control_gap = 8;
        const int control_x = main.x + 16;
        const int control_y = main.y + 58;
        const int control_w = (main.w - 32 - 2 * control_gap) / 3;
        a_button(renderer, state, {control_x, control_y, control_w, 50},
            ru(session) ? "ЛАНДШАФТ" : "LANDSCAPE",
            Action::actor_section, kSourceLandscape, 0, scale,
            slot.engine != cd::EngineKind::plaits, color);
        a_button(renderer, state, {control_x + control_w + control_gap,
            control_y, control_w, 50},
            ru(session) ? "МУЗЫКАЛЬНЫЙ" : "MUSICAL",
            Action::actor_section, kSourceMusical, 0, scale,
            slot.engine == cd::EngineKind::plaits, color);
        a_button(renderer, state, {control_x + 2 * (control_w + control_gap),
            control_y, control_w, 50},
            engine_name(slot.engine, ru(session)), Action::engine_picker,
            0, 0, scale, true, color);
    } else {
        const int source_x = main.x + main.w / 4;
        const int source_w = std::max(178, main.w / 7);
        a_button(renderer, state, {source_x, main.y + 15, source_w, 54},
            ru(session) ? "ЛАНДШАФТ" : "LANDSCAPE",
            Action::actor_section, kSourceLandscape, 0, scale,
            slot.engine != cd::EngineKind::plaits, color);
        a_button(renderer, state, {source_x + source_w + 8, main.y + 15,
            source_w, 54}, ru(session) ? "МУЗЫКАЛЬНЫЙ" : "MUSICAL",
            Action::actor_section, kSourceMusical, 0, scale,
            slot.engine == cd::EngineKind::plaits, color);
        a_button(renderer, state, {source_x + 2 * source_w + 24,
            main.y + 15, std::max(210, main.w / 6), 54},
            engine_name(slot.engine, ru(session)), Action::engine_picker,
            0, 0, scale, true, color);

        if (cd::supports_event_rate(slot.engine)) {
            const float density = cd::effective_event_density(
                slot.event_density, session.performance.events);
            const std::string rate = decimal(cd::event_rate_hz(
                session.tempo_bpm, density, slot.motion), "/S");
            const std::string wait = decimal(cd::event_max_wait_seconds(
                session.tempo_bpm, density, slot.motion), "S");
            text(renderer, main.x + main.w - 380, main.y + 15,
                ru(session) ? "СОБЫТИЯ" : "EVENT RATE", kDim,
                std::max(1, scale - 1));
            text(renderer, main.x + main.w - 380, main.y + 46,
                rate, color, scale);
            text(renderer, main.x + main.w - 228, main.y + 15,
                ru(session) ? "МАКС. ЖДАТЬ" : "MAX WAIT", kDim,
                std::max(1, scale - 1));
            text(renderer, main.x + main.w - 228, main.y + 46,
                wait, color, scale);
        }
        if (cd::supports_manual_trigger(slot.engine))
            a_button(renderer, state, {main.x + main.w - 126,
                main.y + 15, 108, 54},
                ru(session) ? "ЗАПУСК" : "TRIGGER",
                Action::actor_trigger, state.actor, 0, scale, true, color);
    }

    const int tabs_y = main.y + header_h;
    const int tab_w = (main.w - 36) / 3;
    constexpr std::array<ActorSection, 3> sections{{ActorSection::sound,
        ActorSection::events, ActorSection::modulation}};
    const std::array<std::string, 3> labels{{
        ru(session) ? "ЗВУК" : "SOUND",
        ru(session) ? "СОБЫТИЯ" : "EVENTS",
        ru(session) ? "МОДУЛЯЦИЯ" : "MODULATION"}};
    const int tab_h = compact ? 52 : 58;
    for (int i = 0; i < 3; ++i)
        a_button(renderer, state, {main.x + 12 + i * tab_w,
            tabs_y, tab_w - 8, tab_h}, labels[static_cast<std::size_t>(i)],
            Action::actor_section,
            static_cast<int>(sections[static_cast<std::size_t>(i)]),
            0, scale,
            state.actor_section == sections[static_cast<std::size_t>(i)],
            kPurple);
    SDL_Rect body{main.x + 12, tabs_y + tab_h + (compact ? 8 : 12),
        main.w - 24, main.y + main.h - tabs_y - tab_h -
            (compact ? 20 : 24)};
    switch (state.actor_section) {
    case ActorSection::sound:
        a_actor_sound(renderer, session, state, body, scale);
        break;
    case ActorSection::events:
        a_actor_events(renderer, session, state, body, scale);
        break;
    case ActorSection::modulation:
        a_actor_mod(renderer, session, state, body, scale);
        break;
    }
}'''
replace_function(
    "android/app/src/main/cpp/approved_ui_actor.inc",
    "void a_actor(SDL_Renderer* renderer, cd::Session& session, UiState& state,",
    actor_function)

replace_exact(
    "android/app/src/main/cpp/approved_ui_fx_memory.inc",
    "    const int select_w = std::max(210, rect.w / 5);",
    "    const int select_w = rect.w < 1'000 ? std::max(150, rect.w / 5) : std::max(210, rect.w / 5);")
replace_exact(
    "android/app/src/main/cpp/approved_ui_fx_memory.inc",
    "    const int side_w = std::max(310, area.w / 6);",
    "    const int side_w = area.w < 1'450 ? std::clamp(area.w / 5, 220, 270) : std::max(310, area.w / 6);")
replace_exact(
    "android/app/src/main/cpp/approved_ui_fx_memory.inc",
    '        SliderKind::master_level, 0, 0, scale, kGreen, "0", "+12 DB");',
    '        SliderKind::master_level, 0, 0, scale, kGreen, "MUTE", "0 DB");')
replace_exact(
    "android/app/src/main/cpp/approved_ui_fx_memory.inc",
    "        (session.fade_in_seconds - 0.2F) / 19.8F,",
    "        cd::mapping::normalized_fade_seconds(session.fade_in_seconds),")
replace_exact(
    "android/app/src/main/cpp/approved_ui_fx_memory.inc",
    "        (session.fade_out_seconds - 0.2F) / 19.8F,",
    "        cd::mapping::normalized_fade_seconds(session.fade_out_seconds),")
replace_exact(
    "android/app/src/main/cpp/approved_ui_fx_memory.inc",
    "    SDL_Rect content{28, content_y, width - 56, height - content_y - 28};",
    '''    const int outer = a_compact_layout(width, height) ? 16 : 28;
    SDL_Rect content{outer, content_y, width - 2 * outer,
        height - content_y - outer};''')
replace_exact(
    "android/app/src/main/cpp/approved_ui_fx_memory.inc",
    '''    const HitTarget hit = state.pressed;
    state.pressed = {};
    if (hit.action == Action::actor_section && hit.a == kSourceLandscape) {
''',
    '''    const HitTarget hit = state.pressed;
    state.pressed = {};
    if (hit.action == Action::actor_section && hit.a == kActionPanic) {
        g_panic_requested = true;
        return false;
    }
    if (hit.action == Action::actor_section && hit.a == kSourceLandscape) {
''')

docs_path = "docs/android-approved-ui.md"
docs = read(docs_path)
docs += r'''

## P0 interaction hardening

- Android slider mappings now use the full shared core ranges: 20–2000 Hz actor frequency, 0.001–40 Hz modulator rate, 0.25–30 second fades and MIDI roots 0–127.
- Modulation depth is correctly bipolar.
- Rate cross-modulation only offers earlier modulation rows, matching the DSP graph.
- The persistent KILL control performs the same constant-time audio and effect reset as handheld Kill Silence.
- Continuous actors no longer expose inactive Event Rate or Euclidean controls.
- Compact geometry prevents the Actor and FX headers/cards from overlapping at 1280×720-class landscape resolutions.
- Picker pagination is clamped so inactive navigation cannot open empty pages.
'''
write(docs_path, docs)

print("Android P0 patch applied")

#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: Path, before: str, after: str) -> None:
    source = path.read_text(encoding="utf-8")
    if before not in source:
        raise SystemExit(f"missing fragment in {path}: {before[:120]!r}")
    path.write_text(source.replace(before, after, 1), encoding="utf-8")


def replace_all(path: Path, before: str, after: str, expected: int) -> None:
    source = path.read_text(encoding="utf-8")
    count = source.count(before)
    if count != expected:
        raise SystemExit(f"expected {expected} fragments in {path}, found {count}: {before!r}")
    path.write_text(source.replace(before, after), encoding="utf-8")

session_hpp = ROOT / "include/cursed_drone/session.hpp"
replace_once(session_hpp,
'''struct EffectSettings {
    EffectKind kind{EffectKind::bypass};
    float amount{0.0F};
    float tone{0.5F};
    float feedback{0.0F};
};''',
'''struct EffectSettings {
    EffectKind kind{EffectKind::bypass};
    float amount{0.0F};
    float tone{0.5F};
    float feedback{0.0F};
    // Bypass is an empty slot. enabled independently bypasses a configured
    // effect without destroying its kind or parameter values.
    bool enabled{true};
};''')
replace_once(session_hpp, "unsigned schema_version{11};", "unsigned schema_version{12};")

session_cpp = ROOT / "src/session.cpp"
replace_once(session_cpp,
'''        output << master_effect_key(effect_index, "kind") << '=' << to_string(effect.kind) << '\\n';
        output << master_effect_key(effect_index, "amount") << '=' << effect.amount << '\\n';''',
'''        output << master_effect_key(effect_index, "kind") << '=' << to_string(effect.kind) << '\\n';
        output << master_effect_key(effect_index, "enabled") << '=' << (effect.enabled ? 1 : 0) << '\\n';
        output << master_effect_key(effect_index, "amount") << '=' << effect.amount << '\\n';''')
replace_once(session_cpp,
'''            output << effect_key(slot_index, effect_index, "kind") << '=' << to_string(effect.kind) << '\\n';
            output << effect_key(slot_index, effect_index, "amount") << '=' << effect.amount << '\\n';''',
'''            output << effect_key(slot_index, effect_index, "kind") << '=' << to_string(effect.kind) << '\\n';
            output << effect_key(slot_index, effect_index, "enabled") << '=' << (effect.enabled ? 1 : 0) << '\\n';
            output << effect_key(slot_index, effect_index, "amount") << '=' << effect.amount << '\\n';''')
replace_all(session_cpp,
'''schema->second != "10" && schema->second != "11"''',
'''schema->second != "10" && schema->second != "11" && schema->second != "12"''', 2)
replace_once(session_cpp,
'''        if (!parse_enum_value(values, master_effect_key(effect_index, "kind"), effect.kind, kEffects) ||
            !parse_float(values, master_effect_key(effect_index, "amount"), effect.amount) ||''',
'''        if (!parse_enum_value(values, master_effect_key(effect_index, "kind"), effect.kind, kEffects) ||
            !parse_bool(values, master_effect_key(effect_index, "enabled"), effect.enabled) ||
            !parse_float(values, master_effect_key(effect_index, "amount"), effect.amount) ||''')
replace_once(session_cpp,
'''            if (!parse_enum_value(values, effect_key(slot_index, effect_index, "kind"), effect.kind, kEffects) ||
                !parse_float(values, effect_key(slot_index, effect_index, "amount"), effect.amount) ||''',
'''            if (!parse_enum_value(values, effect_key(slot_index, effect_index, "kind"), effect.kind, kEffects) ||
                !parse_bool(values, effect_key(slot_index, effect_index, "enabled"), effect.enabled) ||
                !parse_float(values, effect_key(slot_index, effect_index, "amount"), effect.amount) ||''')
replace_once(session_cpp, "loaded.schema_version = 11;", "loaded.schema_version = 12;")

audio_cpp = ROOT / "src/audio.cpp"
replace_once(audio_cpp,
'''                    if (effect_kind == EffectKind::bypass) continue;''',
'''                    if (!settings.effects[effect_index].enabled ||
                        effect_kind == EffectKind::bypass) continue;''')
replace_once(audio_cpp,
'''                    if (kind == EffectKind::bypass) continue;''',
'''                    if (!settings.effects[effect_index].enabled ||
                        kind == EffectKind::bypass) continue;''')
replace_once(audio_cpp,
'''                if (settings.kind == EffectKind::bypass) continue;''',
'''                if (!settings.enabled || settings.kind == EffectKind::bypass) continue;''')

android_touch = ROOT / "android/app/src/main/cpp/android_touch_main.cpp"
replace_once(android_touch,
'''    actor_fx_select, actor_fx_picker,
    master_fx_select, master_fx_picker, euclidean_toggle,''',
'''    actor_fx_select, actor_fx_picker, actor_fx_toggle,
    master_fx_select, master_fx_picker, master_fx_toggle, euclidean_toggle,''')
replace_once(android_touch,
'''std::vector<cd::ParsedScale> g_scales{};''',
'''std::vector<cd::ParsedScale> g_scales{};
// Set by the approved renderer. Legacy/handheld layouts leave it at zero.
int g_ui_safe_side{0};''')
replace_once(android_touch,
'''    case PickerKind::effect:
        if (state.picker_master) session.master_effects[static_cast<std::size_t>(state.picker_effect)].kind = cd::catalog::effects[static_cast<std::size_t>(index)];
        else slot.effects[static_cast<std::size_t>(state.picker_effect)].kind = cd::catalog::effects[static_cast<std::size_t>(index)];
        session.scene_modified = true;
        break;''',
'''    case PickerKind::effect: {
        auto& effect = state.picker_master
            ? session.master_effects[static_cast<std::size_t>(state.picker_effect)]
            : slot.effects[static_cast<std::size_t>(state.picker_effect)];
        effect.kind = cd::catalog::effects[static_cast<std::size_t>(index)];
        if (effect.kind != cd::EffectKind::bypass) effect.enabled = true;
        session.scene_modified = true;
        break;
    }''')
replace_once(android_touch,
'''    case Action::actor_fx_picker:
        state.picker = PickerKind::effect; state.picker_master = false;
        state.picker_effect = state.actor_fx; state.picker_page = 0; return false;
    case Action::master_fx_select: state.master_fx = hit.a; return false;''',
'''    case Action::actor_fx_picker:
        state.picker = PickerKind::effect; state.picker_master = false;
        state.picker_effect = state.actor_fx; state.picker_page = 0; return false;
    case Action::actor_fx_toggle: {
        auto& effect = session.slots[static_cast<std::size_t>(state.actor)]
            .effects[static_cast<std::size_t>(state.actor_fx)];
        if (effect.kind == cd::EffectKind::bypass) return false;
        effect.enabled = !effect.enabled;
        session.scene_modified = true;
        return true;
    }
    case Action::master_fx_select: state.master_fx = hit.a; return false;''')
replace_once(android_touch,
'''    case Action::master_fx_picker:
        state.picker = PickerKind::effect; state.picker_master = true;
        state.picker_effect = state.master_fx; state.picker_page = 0; return false;
    case Action::euclidean_toggle: {''',
'''    case Action::master_fx_picker:
        state.picker = PickerKind::effect; state.picker_master = true;
        state.picker_effect = state.master_fx; state.picker_page = 0; return false;
    case Action::master_fx_toggle: {
        auto& effect = session.master_effects[static_cast<std::size_t>(state.master_fx)];
        if (effect.kind == cd::EffectKind::bypass) return false;
        effect.enabled = !effect.enabled;
        session.scene_modified = true;
        return true;
    }
    case Action::euclidean_toggle: {''')
replace_once(android_touch,
'''        auto& enabled = session.slots[static_cast<std::size_t>(state.actor)].euclidean.enabled;
        enabled = !enabled; return true;''',
'''        auto& enabled = session.slots[static_cast<std::size_t>(state.actor)].euclidean.enabled;
        enabled = !enabled;
        session.scene_modified = true;
        return true;''')
replace_once(android_touch,
'''        auto& enabled = session.slots[static_cast<std::size_t>(state.actor)]
            .modulators[static_cast<std::size_t>(state.modulator)].enabled;
        enabled = !enabled; return true;''',
'''        auto& enabled = session.slots[static_cast<std::size_t>(state.actor)]
            .modulators[static_cast<std::size_t>(state.modulator)].enabled;
        enabled = !enabled;
        session.scene_modified = true;
        return true;''')
replace_once(android_touch,
'''    const int pad = std::max(16, height / 45);
    const int title_h = std::max(70, height / 10);
    SDL_Rect title{pad, pad, width - 2 * pad, title_h};''',
'''    const int pad = std::max(16, height / 45);
    const int safe = std::clamp(g_ui_safe_side, 0, width / 4);
    const int usable_width = width - 2 * safe;
    const int title_h = std::max(70, height / 10);
    SDL_Rect title{safe + pad, pad, usable_width - 2 * pad, title_h};''')
replace_once(android_touch,
'''    const int item_w = (width - 2 * pad - gap * (columns - 1)) / columns;''',
'''    const int item_w = (usable_width - 2 * pad - gap * (columns - 1)) / columns;''')
replace_once(android_touch,
'''        SDL_Rect rect{pad + col * (item_w + gap), grid_y + row * (item_h + gap), item_w, item_h};''',
'''        SDL_Rect rect{safe + pad + col * (item_w + gap), grid_y + row * (item_h + gap), item_w, item_h};''')
replace_once(android_touch,
'''    const int nav_w = (width - 3 * pad) / 2;
    button(renderer, state, {pad, footer_y, nav_w, footer_h},''',
'''    const int nav_w = (usable_width - 3 * pad) / 2;
    button(renderer, state, {safe + pad, footer_y, nav_w, footer_h},''')
replace_once(android_touch,
'''    button(renderer, state, {2 * pad + nav_w, footer_y, nav_w, footer_h},''',
'''    button(renderer, state, {safe + 2 * pad + nav_w, footer_y, nav_w, footer_h},''')

primitives = ROOT / "android/app/src/main/cpp/approved_ui_primitives.inc"
replace_once(primitives,
'''    (void)height;
    fill(renderer, {0, 0, width, 118}, aTop);
    text(renderer, 28, 18, "CURSED DRONE", aCream, scale + 1);
    text(renderer, 230, 26,''',
'''    (void)height;
    const int safe = std::clamp(g_ui_safe_side, 0, width / 4);
    const int usable_width = width - 2 * safe;
    fill(renderer, {0, 0, width, 118}, aTop);
    text(renderer, safe + 28, 18, "CURSED DRONE", aCream, scale + 1);
    text(renderer, safe + 230, 26,''')
replace_once(primitives,
'''    SDL_Rect fade{width - 406, 13, 165, 35};''',
'''    SDL_Rect fade{safe + usable_width - 406, 13, 165, 35};''')
replace_once(primitives,
'''    const int dsp_x = width - 215;''',
'''    const int dsp_x = safe + usable_width - 215;''')
replace_once(primitives,
'''    const int margin = 27;
    const int gap = 7;
    const int nav_y = 66;
    const int nav_h = 42;
    const int tab_w = (width - 2 * margin - 4 * gap) / 5;
    for (int i = 0; i < 5; ++i) {
        SDL_Rect tab{margin + i * (tab_w + gap), nav_y, tab_w, nav_h};''',
'''    const int margin = 27;
    const int gap = 7;
    const int nav_y = 66;
    const int nav_h = 42;
    const int tab_w = (usable_width - 2 * margin - 4 * gap) / 5;
    for (int i = 0; i < 5; ++i) {
        SDL_Rect tab{safe + margin + i * (tab_w + gap), nav_y, tab_w, nav_h};''')

fx_memory = ROOT / "android/app/src/main/cpp/approved_ui_fx_memory.inc"
replace_once(fx_memory,
'''    state.hits.clear();
    fill(renderer, {0, 0, width, height}, kBackground);
    const int scale = font_scale_for(height);''',
'''    state.hits.clear();
    fill(renderer, {0, 0, width, height}, kBackground);
    // Pixel 8 Pro landscape cutout: reserve the same space on both sides so
    // rotation never moves controls underneath the camera.
    g_ui_safe_side = width >= 1'200 && width > height ? 48 : 0;
    const int scale = font_scale_for(height);''')
replace_once(fx_memory,
'''    const int outer = a_compact_layout(width, height) ? 16 : 28;
    SDL_Rect content{outer, content_y, width - 2 * outer,
        height - content_y - outer};''',
'''    const int outer = a_compact_layout(width, height) ? 16 : 28;
    const int side = g_ui_safe_side + outer;
    SDL_Rect content{side, content_y, width - 2 * side,
        height - content_y - outer};''')

android_main = ROOT / "android/app/src/main/cpp/android_approved_main.cpp"
replace_once(android_main,
'''    bool changed = false;
    bool save_pending = false;''',
'''    bool changed = false;
    bool audio_session_pending = false;
    bool save_pending = false;''')
replace_once(android_main,
'''        if (changed) {
            static_cast<void>(audio.graph.submit_session(session));
            save_pending = true;
            changed_at = now;
            changed = false;
        }''',
'''        if (changed) {
            // Keep the newest state pending until the audio mailbox accepts it.
            // This prevents the final value of a fast gesture or a later toggle
            // from being silently lost when the bounded queue is temporarily full.
            audio_session_pending = true;
            save_pending = true;
            changed_at = now;
            changed = false;
        }
        if (audio_session_pending && audio.graph.submit_session(session)) {
            audio_session_pending = false;
        }''')

fx_exact = ROOT / "android/app/src/main/cpp/approved_ui_fx_exact.inc"
replace_once(fx_exact,
'''    const SDL_Color color = aActor[static_cast<std::size_t>(index)];
    const bool active = effect.kind != cd::EffectKind::bypass;
    a_card(renderer, rect, aTop, active ? color : aBorderSoft, 8, false);''',
'''    const SDL_Color color = aActor[static_cast<std::size_t>(index)];
    const bool configured = effect.kind != cd::EffectKind::bypass;
    const bool active = configured && effect.enabled;
    a_card(renderer, rect, aTop, active ? color : aBorderSoft, 8, false);''')
replace_once(fx_exact,
'''        active ? (ru(session) ? "АКТИВЕН" : "ACTIVE") :
            (ru(session) ? "ПУСТО" : "EMPTY"),
        active ? color : aMutedCream, scale);

    const int meter_x = rect.x + rect.w - 310;''',
'''        active ? (ru(session) ? "АКТИВЕН" : "ACTIVE") :
            (configured ? (ru(session) ? "ВЫКЛ" : "OFF") :
                (ru(session) ? "ПУСТО" : "EMPTY")),
        active ? color : aMutedCream, scale);

    add_hit(state, rect, Action::actor_fx_select, index);
    if (configured) add_hit(state, state_pill, Action::actor_fx_toggle, index);

    const int meter_x = rect.x + rect.w - 310;''')
replace_once(fx_exact,
'''    add_hit(state, rect, Action::actor_fx_select, index);
}''',
'''}''')
replace_once(fx_exact,
'''    const SDL_Color color = aActor[static_cast<std::size_t>(index)];
    const bool active = effect.kind != cd::EffectKind::bypass;
    const int parameter_count = effect_parameter_count(effect.kind);''',
'''    const SDL_Color color = aActor[static_cast<std::size_t>(index)];
    const bool configured = effect.kind != cd::EffectKind::bypass;
    const bool active = configured && effect.enabled;
    const int parameter_count = effect_parameter_count(effect.kind);''')
replace_once(fx_exact,
'''        active ? (ru(session) ? "АКТИВЕН" : "ACTIVE") :
            (ru(session) ? "ОБХОД" : "BYPASS"),
        active ? color : aMutedCream, scale);

    SDL_Rect type_panel''',
'''        active ? (ru(session) ? "АКТИВЕН" : "ACTIVE") :
            (configured ? (ru(session) ? "ВЫКЛ" : "OFF") :
                (ru(session) ? "ПУСТО" : "EMPTY")),
        active ? color : aMutedCream, scale);
    if (configured) add_hit(state, state_pill, Action::actor_fx_toggle, index);

    SDL_Rect type_panel''')

master_exact = ROOT / "android/app/src/main/cpp/approved_ui_master_exact.inc"
replace_once(master_exact,
'''    const bool selected = state.master_fx == index;
    const bool active = effect.kind != cd::EffectKind::bypass;''',
'''    const bool selected = state.master_fx == index;
    const bool configured = effect.kind != cd::EffectKind::bypass;
    const bool active = configured && effect.enabled;''')
replace_once(master_exact,
'''        active ? (ru(session) ? "АКТИВЕН" : "ACTIVE") :
            (ru(session) ? "ПУСТО" : "EMPTY"),
        active ? color : aMutedCream, scale);
    add_hit(state, rect, Action::master_fx_select, index);''',
'''        active ? (ru(session) ? "АКТИВЕН" : "ACTIVE") :
            (configured ? (ru(session) ? "ВЫКЛ" : "OFF") :
                (ru(session) ? "ПУСТО" : "EMPTY")),
        active ? color : aMutedCream, scale);
    add_hit(state, rect, Action::master_fx_select, index);
    if (configured) add_hit(state, pill, Action::master_fx_toggle, index);''')
replace_once(master_exact,
'''    const SDL_Color color = aActor[static_cast<std::size_t>(index)];
    const bool active = effect.kind != cd::EffectKind::bypass;
    const int parameter_count = effect_parameter_count(effect.kind);''',
'''    const SDL_Color color = aActor[static_cast<std::size_t>(index)];
    const bool configured = effect.kind != cd::EffectKind::bypass;
    const bool active = configured && effect.enabled;
    const int parameter_count = effect_parameter_count(effect.kind);''')
replace_once(master_exact,
'''        active ? (ru(session) ? "АКТИВЕН" : "ACTIVE") :
            (ru(session) ? "ОБХОД" : "BYPASS"),
        active ? color : aMutedCream, scale);

    const int header_h''',
'''        active ? (ru(session) ? "АКТИВЕН" : "ACTIVE") :
            (configured ? (ru(session) ? "ВЫКЛ" : "OFF") :
                (ru(session) ? "ПУСТО" : "EMPTY")),
        active ? color : aMutedCream, scale);
    if (configured) add_hit(state, state_pill, Action::master_fx_toggle, index);

    const int header_h''')

tests = ROOT / "tests/test_main.cpp"
replace_once(tests,
'''    original.slots[2].effects[1].amount = 0.731F;''',
'''    original.slots[2].effects[1].amount = 0.731F;
    original.slots[2].effects[1].enabled = false;''')
replace_once(tests,
'''    expect(loaded.schema_version == 11, "session should upgrade to schema 11");''',
'''    expect(loaded.schema_version == 12, "session should upgrade to schema 12");''')
replace_once(tests,
'''    expect(loaded.slots[2].effects[1].kind == cd::EffectKind::ringmod,''',
'''    expect(!loaded.slots[2].effects[1].enabled, "effect enabled state should roundtrip");
    expect(loaded.slots[2].effects[1].kind == cd::EffectKind::ringmod,''')

print("Android safe area, effect toggles and control delivery fixes applied")

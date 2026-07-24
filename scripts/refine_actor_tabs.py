#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "android/app/src/main/cpp"


def replace_once(path: Path, before: str, after: str) -> None:
    source = path.read_text(encoding="utf-8")
    if before not in source:
        raise SystemExit(f"missing expected fragment in {path}: {before[:100]!r}")
    path.write_text(source.replace(before, after, 1), encoding="utf-8")


actor = CPP / "approved_ui_actor_exact.inc"
source = actor.read_text(encoding="utf-8")

sidebar_card = r'''
void x_actor_sidebar_card_exact(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, const cd::AudioTelemetry& telemetry, SDL_Rect rect,
    int actor, int scale, bool selected) {
    const auto& slot = session.slots[static_cast<std::size_t>(actor)];
    const SDL_Color color = aActor[static_cast<std::size_t>(actor)];
    const SDL_Color border = selected ? color :
        (slot.enabled ? color : aBorderSoft);
    a_card(renderer, rect, aTop, border, 8, selected);
    a_text(renderer, rect.x + 11, rect.y + 9,
        std::to_string(actor + 1), color, scale + 1);
    a_text(renderer, rect.x + 34, rect.y + 9,
        engine_name(slot.engine, ru(session)),
        slot.enabled ? aCream : aMutedCream, scale + 1);

    const int signal_y = rect.y + 34;
    a_meter(renderer, {rect.x + 11, signal_y, rect.w - 22, 7},
        telemetry.slot_rms[static_cast<std::size_t>(actor)] * 4.2F, color);
    a_text(renderer, rect.x + 11, signal_y + 12,
        ru(session) ? "УРОВЕНЬ" : "LEVEL", aMutedCream, scale);
    const std::string level = percent(slot.level);
    a_text(renderer, rect.x + rect.w - 11 - a_text_width(level, scale),
        signal_y + 12, level, color, scale);

    // Keep the draggable control clearly separated from the MUTE button.
    const int level_y = rect.y + 64;
    SDL_Rect level_bar{rect.x + 11, level_y, rect.w - 22, 8};
    a_slider_track(renderer, level_bar, slot.level,
        slot.enabled ? color : aMutedCream);
    add_hit(state, {level_bar.x - 4, level_y - 10,
        level_bar.w + 8, 26}, Action::slider, actor, 0,
        SliderKind::actor_level);

    SDL_Rect mute{rect.x + 11, rect.y + rect.h - 31,
        rect.w - 22, 21};
    a_card(renderer, mute, slot.enabled ? aPanel : aRed,
        slot.enabled ? border : aRed, 5, false);
    a_centered_text(renderer, mute,
        slot.enabled ? "MUTE" : (ru(session) ? "ВКЛЮЧИТЬ" : "UNMUTE"),
        aCream, scale);
    add_hit(state, mute, Action::actor_toggle, actor);
    add_hit(state, {rect.x, rect.y, rect.w,
        std::max(1, level_y - rect.y - 7)}, Action::actor_select,
        actor, 1);
}

'''
marker = "void x_actor_sidebar_exact(SDL_Renderer* renderer, cd::Session& session,"
if "void x_actor_sidebar_card_exact" not in source:
    source = source.replace(marker, sidebar_card + marker, 1)
source = source.replace(
    "        a_actor_card(renderer, session, state, telemetry,\n"
    "            {area.x + 9, top + i * (card_h + gap), area.w - 18, card_h},\n"
    "            i, scale, state.actor == i, false);",
    "        x_actor_sidebar_card_exact(renderer, session, state, telemetry,\n"
    "            {area.x + 9, top + i * (card_h + gap), area.w - 18, card_h},\n"
    "            i, scale, state.actor == i);",
    1,
)

old_header = r'''    a_text(renderer, main.x + 18, main.y + 16,
        std::to_string(state.actor + 1), color, scale + 2);
    a_text(renderer, main.x + 55, main.y + 16,
        engine_name(slot.engine, ru(session)), aCream, scale + 2);

    if (cd::supports_event_rate(slot.engine)) {
        SDL_Rect badge{main.x + 145, main.y + 13, 96, 32};
        a_card(renderer, badge, a_mix(aPanel2, color, 0.10F), color, 7, false);
        a_centered_text(renderer, badge,
            ru(session) ? "СОБЫТИЕ" : "EVENT", color, scale);
    }

    const int source_x = main.x + 268;
'''
new_header = r'''    // Actor identity and all actionable controls share one horizontal axis.
    const int header_control_y = main.y + 29;
    const int header_control_h = 38;
    a_text(renderer, main.x + 18, header_control_y + 5,
        std::to_string(state.actor + 1), color, scale + 2);
    a_text(renderer, main.x + 55, header_control_y + 5,
        engine_name(slot.engine, ru(session)), aCream, scale + 2);

    if (cd::supports_event_rate(slot.engine)) {
        SDL_Rect badge{main.x + 145, header_control_y, 96, header_control_h};
        a_card(renderer, badge, a_mix(aPanel2, color, 0.10F), color, 7, false);
        a_centered_text(renderer, badge,
            ru(session) ? "СОБЫТИЕ" : "EVENT", color, scale);
    }

    const int source_x = main.x + 268;
'''
if old_header not in source:
    raise SystemExit("Actor header fragment not found")
source = source.replace(old_header, new_header, 1)
source = source.replace(
    "SDL_Rect landscape{source_x, main.y + 29, 112, 38};",
    "SDL_Rect landscape{source_x, header_control_y, 112, header_control_h};",
    1,
)
source = source.replace(
    "SDL_Rect engine{engine_x, main.y + 29, 162, 38};",
    "SDL_Rect engine{engine_x, header_control_y, 162, header_control_h};",
    1,
)
source = source.replace(
    "            {trigger_x, main.y + 18, trigger_w, 48},",
    "            {trigger_x, header_control_y, trigger_w, header_control_h},",
    1,
)

exact_tabs = r'''
void x_actor_events_exact(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, const cd::AudioTelemetry& telemetry, SDL_Rect area,
    int scale) {
    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    const SDL_Color color = aActor[static_cast<std::size_t>(state.actor)];
    const bool has_rate = cd::supports_event_rate(slot.engine);
    const bool can_trigger = cd::supports_manual_trigger(slot.engine);
    const int gap = 10;
    const int left_w = static_cast<int>(area.w * 0.54F);
    SDL_Rect left{area.x, area.y, left_w - gap, area.h};
    SDL_Rect right{area.x + left_w, area.y, area.w - left_w, area.h};
    a_card(renderer, left, aTop, aBorderSoft, 8, false);
    a_card(renderer, right, aTop, aBorderSoft, 8, false);

    a_text(renderer, left.x + 14, left.y + 11,
        ru(session) ? "СОБЫТИЯ АКТЁРА" : "ACTOR EVENTS", aCream, scale + 1);
    SDL_Rect pulse{left.x + left.w - 126, left.y + 8, 112, 30};
    const bool firing = telemetry.slot_event[static_cast<std::size_t>(state.actor)] > 0.04F;
    a_card(renderer, pulse,
        firing ? a_mix(aPanel2, color, 0.24F) : aPanel2,
        firing ? color : aBorderSoft, 7, false);
    a_centered_text(renderer, pulse,
        ru(session) ? "СОБЫТИЕ" : "EVENT",
        firing ? color : aMutedCream, scale);

    if (has_rate) {
        const float density = cd::effective_event_density(slot.event_density,
            session.performance.events);
        const float rate = cd::event_rate_hz(session.tempo_bpm, density,
            slot.motion);
        const float wait = cd::event_max_wait_seconds(session.tempo_bpm,
            density, slot.motion);
        x_slider(renderer, state,
            {left.x + 14, left.y + 48, left.w - 28, 66},
            ru(session) ? "ЧАСТОТА СОБЫТИЙ" : "EVENT RATE",
            decimal(rate, "/S"), slot.event_density,
            SliderKind::actor_event_density, 0, 0, scale + 1, color,
            ru(session) ? "РЕДКО" : "SPARSE",
            ru(session) ? "ЧАСТО" : "BUSY");

        const int stat_gap = 8;
        const int stat_y = left.y + 125;
        const int stat_w = (left.w - 28 - stat_gap) / 2;
        SDL_Rect rate_card{left.x + 14, stat_y, stat_w, 57};
        SDL_Rect wait_card{rate_card.x + rate_card.w + stat_gap,
            stat_y, stat_w, 57};
        a_card(renderer, rate_card, aPanel2, aBorderSoft, 7, false);
        a_card(renderer, wait_card, aPanel2, aBorderSoft, 7, false);
        a_text(renderer, rate_card.x + 11, rate_card.y + 8,
            ru(session) ? "ТЕКУЩАЯ ЧАСТОТА" : "CURRENT RATE",
            aMutedCream, scale);
        a_text(renderer, rate_card.x + 11, rate_card.y + 28,
            decimal(rate, "/S"), color, scale + 1);
        a_text(renderer, wait_card.x + 11, wait_card.y + 8,
            ru(session) ? "МАКС. ОЖИДАНИЕ" : "MAX WAIT",
            aMutedCream, scale);
        a_text(renderer, wait_card.x + 11, wait_card.y + 28,
            decimal(wait, "S"), color, scale + 1);
    } else {
        SDL_Rect info{left.x + 14, left.y + 48, left.w - 28, 134};
        a_card(renderer, info, aPanel2, aBorderSoft, 7, false);
        a_text(renderer, info.x + 14, info.y + 16,
            can_trigger ? (ru(session) ? "МУЗЫКАЛЬНЫЙ ТРИГГЕР" :
                "MUSICAL TRIGGER") :
                (ru(session) ? "НЕПРЕРЫВНЫЙ АКТЁР" : "CONTINUOUS ACTOR"),
            aCream, scale + 1);
        a_text(renderer, info.x + 14, info.y + 48,
            can_trigger ? (ru(session) ? "ТАЙМИНГ ЗАДАЁТ EUCLIDEAN" :
                "TIMING COMES FROM EUCLIDEAN") :
                (ru(session) ? "ЧАСТОТА СОБЫТИЙ НЕ ПРИМЕНЯЕТСЯ" :
                "EVENT RATE DOES NOT APPLY"), aMutedCream, scale);
    }

    SDL_Rect trigger{left.x + 14, left.y + left.h - 58,
        left.w - 28, 44};
    if (can_trigger) {
        x_button(renderer, state, trigger,
            ru(session) ? "ЗАПУСТИТЬ СОБЫТИЕ >" : "TRIGGER EVENT >",
            Action::actor_trigger, state.actor, 0, scale + 1, true, color);
    } else {
        a_card(renderer, trigger, aPanel2, aBorderSoft, 7, false);
        a_centered_text(renderer, trigger,
            ru(session) ? "НЕПРЕРЫВНЫЙ ДВИЖОК" : "CONTINUOUS ENGINE",
            aMutedCream, scale);
    }

    a_text(renderer, right.x + 14, right.y + 11,
        "EUCLIDEAN", aCream, scale + 1);
    SDL_Rect toggle{right.x + right.w - 144, right.y + 8, 130, 30};
    if (can_trigger) {
        x_button(renderer, state, toggle,
            slot.euclidean.enabled ?
                (ru(session) ? "ВКЛЮЧЕНО" : "ENABLED") :
                (ru(session) ? "ВЫКЛЮЧЕНО" : "DISABLED"),
            Action::euclidean_toggle, 0, 0, scale,
            slot.euclidean.enabled, aGreen);
        const int rows_y = right.y + 48;
        const int row_h = (right.h - 62) / 4;
        x_slider(renderer, state,
            {right.x + 14, rows_y, right.w - 28, row_h},
            ru(session) ? "ШАГИ" : "STEPS",
            std::to_string(slot.euclidean.steps),
            static_cast<float>(slot.euclidean.steps - 1) / 31.0F,
            SliderKind::euclidean_steps, 0, 0, scale + 1, aGreen,
            "1", "32");
        x_slider(renderer, state,
            {right.x + 14, rows_y + row_h, right.w - 28, row_h},
            ru(session) ? "ИМПУЛЬСЫ" : "PULSES",
            std::to_string(slot.euclidean.pulses),
            static_cast<float>(slot.euclidean.pulses) /
                static_cast<float>(std::max(1, slot.euclidean.steps)),
            SliderKind::euclidean_pulses, 0, 0, scale + 1, color,
            "0", std::to_string(slot.euclidean.steps));
        x_slider(renderer, state,
            {right.x + 14, rows_y + 2 * row_h, right.w - 28, row_h},
            ru(session) ? "СДВИГ" : "ROTATION",
            std::to_string(slot.euclidean.rotation),
            static_cast<float>(slot.euclidean.rotation) /
                static_cast<float>(std::max(1, slot.euclidean.steps - 1)),
            SliderKind::euclidean_rotation, 0, 0, scale + 1, aBlue,
            "0", std::to_string(std::max(0, slot.euclidean.steps - 1)));
        x_slider(renderer, state,
            {right.x + 14, rows_y + 3 * row_h, right.w - 28, row_h},
            ru(session) ? "ВЕРОЯТНОСТЬ" : "PROBABILITY",
            percent(slot.euclidean.probability), slot.euclidean.probability,
            SliderKind::euclidean_probability, 0, 0, scale + 1, aPurple,
            "0%", "100%");
    } else {
        SDL_Rect info{right.x + 14, right.y + 48,
            right.w - 28, right.h - 62};
        a_card(renderer, info, aPanel2, aBorderSoft, 7, false);
        a_centered_text(renderer, info,
            ru(session) ? "ДЛЯ ЭТОГО ДВИЖКА НЕ ПРИМЕНЯЕТСЯ" :
                "NOT USED BY THIS ENGINE", aMutedCream, scale + 1);
    }
}

void x_actor_mod_exact(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect area, int scale) {
    auto& mod = session.slots[static_cast<std::size_t>(state.actor)]
        .modulators[static_cast<std::size_t>(state.modulator)];
    const int gap = 8;
    const int selector_h = 42;
    const int selector_w = (area.w - 3 * gap) / 4;
    for (int i = 0; i < 4; ++i) {
        x_button(renderer, state,
            {area.x + i * (selector_w + gap), area.y,
                selector_w, selector_h},
            std::string{"MOD "} + std::to_string(i + 1),
            Action::mod_select, i, 0, scale + 1,
            i == state.modulator, aActor[static_cast<std::size_t>(i)]);
    }

    const int top = area.y + selector_h + 9;
    const int left_w = static_cast<int>(area.w * 0.38F);
    SDL_Rect left{area.x, top, left_w - 9, area.y + area.h - top};
    SDL_Rect right{area.x + left_w, top,
        area.w - left_w, area.y + area.h - top};
    a_card(renderer, left, aTop, aBorderSoft, 8, false);
    a_card(renderer, right, aTop, aBorderSoft, 8, false);

    const SDL_Color color = aActor[static_cast<std::size_t>(state.modulator)];
    a_text(renderer, left.x + 14, left.y + 11,
        std::string{"MODULATOR "} + std::to_string(state.modulator + 1),
        aCream, scale + 1);
    SDL_Rect enabled{left.x + left.w - 132, left.y + 8, 118, 30};
    x_button(renderer, state, enabled,
        mod.enabled ? (ru(session) ? "ВКЛЮЧЕН" : "ENABLED") :
            (ru(session) ? "ВЫКЛЮЧЕН" : "DISABLED"),
        Action::mod_toggle, 0, 0, scale, mod.enabled, aGreen);

    const int control_x = left.x + 14;
    const int control_w = left.w - 28;
    a_text(renderer, control_x, left.y + 52,
        ru(session) ? "ФОРМА" : "WAVE", aMutedCream, scale);
    x_button(renderer, state,
        {control_x, left.y + 72, control_w, 42},
        wave_name(mod.wave, ru(session)), Action::actor_section,
        95, 0, scale + 1, false, color);
    a_text(renderer, control_x, left.y + 128,
        ru(session) ? "НАЗНАЧЕНИЕ" : "DESTINATION", aMutedCream, scale);
    x_button(renderer, state,
        {control_x, left.y + 148, control_w, 42},
        destination_name(mod.destination, ru(session)),
        Action::actor_section, 96, 0, scale + 1, false, aBlue);

    const std::string source = mod.rate_mod_source < 0 ?
        (ru(session) ? "ИСТОЧНИК: НЕТ" : "SOURCE: NONE") :
        std::string{ru(session) ? "ИСТОЧНИК: MOD " : "SOURCE: MOD "} +
            std::to_string(mod.rate_mod_source + 1);
    x_button(renderer, state,
        {control_x, left.y + left.h - 58, control_w, 44},
        source, Action::mod_source_cycle, 0, 0, scale + 1,
        mod.rate_mod_source >= 0, aPurple);

    const int rows_y = right.y + 8;
    const int row_h = (right.h - 16) / 4;
    x_slider(renderer, state,
        {right.x + 14, rows_y, right.w - 28, row_h},
        ru(session) ? "СКОРОСТЬ" : "RATE",
        decimal(mod.rate_hz, "HZ"),
        cd::mapping::normalized_mod_rate(mod.rate_hz),
        SliderKind::mod_rate, 0, 0, scale + 1, aGreen,
        "0.01 HZ", "40 HZ");
    x_slider(renderer, state,
        {right.x + 14, rows_y + row_h, right.w - 28, row_h},
        ru(session) ? "ГЛУБИНА" : "DEPTH",
        signed_percent(mod.depth), (mod.depth + 1.0F) * 0.5F,
        SliderKind::mod_depth, 0, 0, scale + 1, color,
        "-100%", "+100%");
    x_slider(renderer, state,
        {right.x + 14, rows_y + 2 * row_h, right.w - 28, row_h},
        ru(session) ? "СМЕЩЕНИЕ" : "OFFSET",
        signed_percent(mod.offset), (mod.offset + 1.0F) * 0.5F,
        SliderKind::mod_offset, 0, 0, scale + 1, aBlue,
        "-100%", "+100%");
    x_slider(renderer, state,
        {right.x + 14, rows_y + 3 * row_h, right.w - 28, row_h},
        ru(session) ? "КРОСС-МОД" : "CROSS MOD",
        signed_percent(mod.rate_mod_amount),
        (mod.rate_mod_amount + 1.0F) * 0.5F,
        SliderKind::mod_cross, 0, 0, scale + 1, aPurple,
        "-100%", "+100%");
}

'''
marker = "void a_actor(SDL_Renderer* renderer, cd::Session& session, UiState& state,"
if "void x_actor_events_exact" not in source:
    source = source.replace(marker, exact_tabs + marker, 1)
source = source.replace(
    "        a_actor_events(renderer, session, state, body, scale);",
    "        x_actor_events_exact(renderer, session, state, telemetry, body, scale);",
    1,
)
source = source.replace(
    "        a_actor_mod(renderer, session, state, body, scale);",
    "        x_actor_mod_exact(renderer, session, state, body, scale);",
    1,
)
actor.write_text(source, encoding="utf-8")

snapshots = ROOT / "tools/android_ui_snapshots.cpp"
source = snapshots.read_text(encoding="utf-8")
source = source.replace(
    "bool render_page(const std::filesystem::path& output_directory, Page page) {",
    "bool render_page(const std::filesystem::path& output_directory, Page page,\n"
    "    ActorSection actor_section = ActorSection::sound,\n"
    "    const char* filename_override = nullptr) {",
    1,
)
source = source.replace(
    "    state.page = page;\n    state.actor = 1;",
    "    state.page = page;\n    state.actor_section = actor_section;\n    state.actor = 1;",
    1,
)
source = source.replace(
    "    const std::filesystem::path output = output_directory / page_filename(page);",
    "    const std::filesystem::path output = output_directory /\n"
    "        (filename_override != nullptr ? filename_override : page_filename(page));",
    1,
)
source = source.replace(
    "    for (Page page : pages) {\n"
    "        success = render_page(output_directory, page) && success;\n"
    "    }",
    "    for (Page page : pages) {\n"
    "        success = render_page(output_directory, page) && success;\n"
    "    }\n"
    "    success = render_page(output_directory, Page::actor,\n"
    "        ActorSection::events, \"android-actor-events.bmp\") && success;\n"
    "    success = render_page(output_directory, Page::actor,\n"
    "        ActorSection::modulation, \"android-actor-modulation.bmp\") && success;",
    1,
)
snapshots.write_text(source, encoding="utf-8")

workflow = ROOT / ".github/workflows/build.yml"
source = workflow.read_text(encoding="utf-8")
source = source.replace(
    "test \"$(find dist/android-ui-snapshots -name 'android-*.bmp' | wc -l)\" -eq 5",
    "test \"$(find dist/android-ui-snapshots -name 'android-*.bmp' | wc -l)\" -eq 7",
    1,
)
workflow.write_text(source, encoding="utf-8")

print("Actor alignment, Events and Modulation redesign applied")

// Exact approved Place screen overrides. The legacy implementations are
// retained under renamed symbols for the remaining screens during Sprint 1.

void x_gradient_fill(SDL_Renderer* renderer, SDL_Rect rect, int radius,
    SDL_Color top, SDL_Color bottom) {
    a_round_fill(renderer, rect, radius, bottom);
    const int upper = std::max(1, rect.h / 2);
    for (int row = 0; row < upper; ++row) {
        const float amount = static_cast<float>(row) /
            static_cast<float>(std::max(1, upper - 1));
        const SDL_Color color = a_mix(top, bottom, amount);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        const int inset = radius > 0 && row < radius
            ? radius - static_cast<int>(std::sqrt(static_cast<float>(
                std::max(0, radius * radius - (radius - row) *
                    (radius - row))))) : 0;
        SDL_RenderDrawLine(renderer, rect.x + inset, rect.y + row,
            rect.x + rect.w - inset - 1, rect.y + row);
    }
}

void x_button(SDL_Renderer* renderer, UiState& state, SDL_Rect rect,
    std::string label, Action action, int a, int b, int scale,
    bool active = false, SDL_Color accent = kPurple) {
    const bool page = action == Action::page;
    const SDL_Color surface = page && active ? aActive :
        (active ? a_mix(aPanel2, accent, 0.12F) : aPanel2);
    const SDL_Color border = active ? accent : aBorderSoft;
    a_card(renderer, rect, surface, border, 8, false);
    if (page && active) {
        x_gradient_fill(renderer, {rect.x + 1, rect.y + 1,
            rect.w - 2, rect.h - 2}, 7,
            a_mix(aActive, aCream, 0.06F), aActive);
    }
    a_centered_text(renderer, rect, std::move(label),
        active ? aCream : aMutedCream, scale);
    add_hit(state, rect, action, a, b);
}

void x_slider(SDL_Renderer* renderer, UiState& state, SDL_Rect rect,
    std::string label, std::string value, float normalized, SliderKind kind,
    int a, int b, int scale, SDL_Color color,
    std::string low = {}, std::string high = {}) {
    normalized = std::clamp(normalized, 0.0F, 1.0F);
    const int label_scale = std::max(1, scale - 1);
    const int label_w = 192;
    a_text(renderer, rect.x, rect.y + 5, label, aCream, scale);
    a_text(renderer, rect.x + label_w - a_text_width(value, scale),
        rect.y + 5, value, color, scale);
    SDL_Rect bar{rect.x + label_w + 26, rect.y + 12,
        rect.w - label_w - 34, 9};
    a_slider_track(renderer, bar, normalized, color);
    if (!low.empty()) {
        a_text(renderer, bar.x, rect.y + 28,
            low, aMutedCream, label_scale);
    }
    if (!high.empty()) {
        a_text(renderer, bar.x + bar.w - a_text_width(high, label_scale),
            rect.y + 28, high, aMutedCream, label_scale);
    }
    a_hline(renderer, rect.x, rect.y + 48, rect.w, aBorderSoft);
    add_hit(state, rect, Action::slider, a, b, kind);
}

void a_header(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, float cpu, int width, int height,
    int scale, int& content_y) {
    (void)height;
    fill(renderer, {0, 0, width, 118}, aTop);
    a_text(renderer, 27, 20, "CURSED DRONE", aCream, scale + 2);
    a_text(renderer, 232, 27,
        std::string{CURSED_DRONE_VERSION} + "  •  ANDROID CONCEPT",
        aMutedCream, scale);

    SDL_Rect fade{width - 406, 13, 165, 35};
    a_card(renderer, fade, a_mix(aPanel2, kPurple, 0.15F),
        kPurple, 7, false);
    a_text(renderer, fade.x + 13, fade.y + 7,
        ru(session) ? "ФЕЙД" : "FADE", aCream, scale + 1);
    const std::string fade_value = percent(session.performance.fade);
    a_text(renderer, fade.x + fade.w - 13 -
        a_text_width(fade_value, scale + 1), fade.y + 7,
        fade_value, aGreen, scale + 1);
    add_hit(state, fade, Action::fade);

    const int dsp_x = width - 215;
    a_text(renderer, dsp_x, 10, "DSP", aMutedCream, scale);
    const std::string cpu_text = std::to_string(
        static_cast<int>(std::lround(cpu * 100.0F))) + "%";
    a_text(renderer, dsp_x + 32, 10, cpu_text, aMutedCream, scale);
    constexpr int segments = 12;
    for (int i = 0; i < segments; ++i) {
        const float threshold = static_cast<float>(i + 1) /
            static_cast<float>(segments);
        const bool on = threshold <= std::clamp(
            telemetry.master_rms * 4.0F, 0.0F, 1.0F);
        const SDL_Color segment = on ? aGreen : aTrack;
        a_round_fill(renderer, {dsp_x + i * 16, 31, 13, 10}, 2, segment);
        if (!on && i < 2) {
            // A pair of outlined peak markers mirrors the approved meter.
            a_card(renderer, {dsp_x + i * 16, 31, 13, 10},
                aTop, aCream, 2, false);
        }
    }

    constexpr std::array<Page, 5> pages{{Page::place, Page::actor,
        Page::fx, Page::master, Page::memory}};
    const int margin = 27;
    const int gap = 7;
    const int nav_y = 66;
    const int nav_h = 42;
    const int tab_w = (width - 2 * margin - 4 * gap) / 5;
    for (int i = 0; i < 5; ++i) {
        SDL_Rect tab{margin + i * (tab_w + gap), nav_y, tab_w, nav_h};
        const bool active = pages[static_cast<std::size_t>(i)] == state.page;
        x_button(renderer, state, tab,
            page_name(pages[static_cast<std::size_t>(i)], ru(session)),
            Action::page, static_cast<int>(pages[static_cast<std::size_t>(i)]),
            0, scale + 1, active, active ? aCream : kPurple);
    }
    content_y = 118;
}

void a_actor_card(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, const cd::AudioTelemetry& telemetry, SDL_Rect rect,
    int actor, int scale, bool selected, bool open_actor) {
    const auto& slot = session.slots[static_cast<std::size_t>(actor)];
    const SDL_Color color = aActor[static_cast<std::size_t>(actor)];
    const SDL_Color border = selected ? color :
        (slot.enabled ? color : aBorderSoft);
    a_card(renderer, rect, aTop, border, 8, selected);
    a_text(renderer, rect.x + 11, rect.y + 10,
        std::to_string(actor + 1), color, scale + 1);
    a_text(renderer, rect.x + 34, rect.y + 10,
        engine_name(slot.engine, ru(session)),
        slot.enabled ? aCream : aMutedCream, scale + 1);
    if (telemetry.slot_event[static_cast<std::size_t>(actor)] > 0.04F) {
        SDL_Rect event{rect.x + rect.w - 30, rect.y + 9, 20, 20};
        a_round_fill(renderer, event, 10, color);
        a_round_fill(renderer, {event.x + 3, event.y + 3,
            event.w - 6, event.h - 6}, 7, aTop);
    }

    const int signal_y = rect.y + 36;
    a_meter(renderer, {rect.x + 11, signal_y, rect.w - 22, 8},
        telemetry.slot_rms[static_cast<std::size_t>(actor)] * 4.2F, color);
    a_text(renderer, rect.x + 11, signal_y + 17,
        ru(session) ? "УРОВЕНЬ" : "LEVEL", aMutedCream, scale);
    const std::string level = percent(slot.level);
    a_text(renderer, rect.x + rect.w - 11 - a_text_width(level, scale),
        signal_y + 13, level, color, scale);
    const int level_y = rect.y + 74;
    SDL_Rect level_bar{rect.x + 11, level_y, rect.w - 22, 9};
    a_slider_track(renderer, level_bar, slot.level,
        slot.enabled ? color : aMutedCream);
    add_hit(state, {level_bar.x - 4, level_y - 10,
        level_bar.w + 8, 30}, Action::slider, actor, 0,
        SliderKind::actor_level);

    SDL_Rect mute{rect.x + 11, rect.y + rect.h - 33,
        rect.w - 22, 22};
    a_card(renderer, mute, slot.enabled ? aPanel : aRed,
        slot.enabled ? border : aRed, 6, false);
    a_centered_text(renderer, mute,
        slot.enabled ? (ru(session) ? "MUTE" : "MUTE") :
            (ru(session) ? "ВКЛЮЧИТЬ" : "UNMUTE"),
        aCream, scale);
    add_hit(state, mute, Action::actor_toggle, actor);
    add_hit(state, {rect.x, rect.y, rect.w,
        std::max(1, level_y - rect.y - 8)}, Action::actor_select,
        actor, open_actor ? 1 : 0);
}

void a_place(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, SDL_Rect area, int scale) {
    // Exact Pixel 8 Pro reference: 54 px sides and 42 px bottom.
    area.x -= 1;
    area.w += 2;
    area.h += 7;
    a_card(renderer, area, aPanel2, aBorderSoft, 10, false);
    const int pad = 12;
    const int gap = 12;
    SDL_Rect scene{area.x + pad, area.y + pad, area.w - 2 * pad, 56};
    x_gradient_fill(renderer, scene, 8,
        a_mix(aPanel2, kPurple, 0.18F), a_mix(aPanel2, kPurple, 0.08F));
    SDL_SetRenderDrawColor(renderer, aBorderSoft.r, aBorderSoft.g,
        aBorderSoft.b, aBorderSoft.a);
    SDL_Rect scene_outline{scene.x, scene.y, scene.w - 1, scene.h - 1};
    SDL_RenderDrawRect(renderer, &scene_outline);
    a_text(renderer, scene.x + 15, scene.y + 8,
        ru(session) ? "ЛАНДШАФТ" : "LANDSCAPE", aMutedCream, scale);
    a_text(renderer, scene.x + 15, scene.y + 25,
        scene_name(session.scene, ru(session)) +
            (session.scene_modified ? " *" : ""), aCream, scale + 2);
    const std::string choose = ru(session) ?
        "НАЖАТЬ ДЛЯ ВЫБОРА" : "TAP TO CHOOSE";
    a_text(renderer, scene.x + scene.w - 48 -
        a_text_width(choose, scale), scene.y + 36,
        choose, aMutedCream, scale);
    // Downward triangle without relying on a font glyph.
    SDL_Point arrow[4]{{scene.x + scene.w - 30, scene.y + 22},
        {scene.x + scene.w - 20, scene.y + 22},
        {scene.x + scene.w - 25, scene.y + 31},
        {scene.x + scene.w - 30, scene.y + 22}};
    SDL_SetRenderDrawColor(renderer, aCream.r, aCream.g, aCream.b, aCream.a);
    SDL_RenderDrawLines(renderer, arrow, 4);
    add_hit(state, scene, Action::scene_picker);

    const int top = scene.y + scene.h + 12;
    const int body_h = area.y + area.h - top - pad;
    const int left_w = static_cast<int>(area.w * 0.62F);
    SDL_Rect macros{area.x + pad, top, left_w - pad - gap, body_h};
    SDL_Rect actors{area.x + left_w, top,
        area.x + area.w - pad - (area.x + left_w), body_h};
    a_card(renderer, macros, aTop, aBorderSoft, 8, false);
    a_card(renderer, actors, aTop, aBorderSoft, 8, false);
    a_text(renderer, macros.x + 15, macros.y + 9,
        ru(session) ? "ХАРАКТЕР МЕСТА" : "PLACE CHARACTER",
        aCream, scale + 1);
    const std::string drag_hint = ru(session) ?
        "ТАП ИЛИ ПРОВЕДИТЕ ПАЛЬЦЕМ" : "TAP OR DRAG";
    a_text(renderer, macros.x + macros.w - 15 -
        a_text_width(drag_hint, scale), macros.y + 11,
        drag_hint, aMutedCream, scale);
    a_text(renderer, actors.x + 15, actors.y + 9,
        ru(session) ? "АКТЁРЫ ЛАНДШАФТА" : "LANDSCAPE ACTORS",
        aCream, scale + 1);
    a_text(renderer, actors.x + actors.w - 18 - a_text_width("4", scale + 1),
        actors.y + 9, "4", aMutedCream, scale + 1);

    const std::array<float, 5> values{{session.performance.texture,
        session.performance.pulse, session.performance.chaos,
        session.performance.space, session.performance.events}};
    const std::array<SliderKind, 5> kinds{{SliderKind::place_texture,
        SliderKind::place_pulse, SliderKind::place_chaos,
        SliderKind::place_space, SliderKind::place_events}};
    const std::array<SDL_Color, 5> colors{{aGreen, aOrange, aRed,
        aBlue, aPurple}};
    const std::array<std::string, 5> low_ru{{"ГЛАДКО", "РЕДКО",
        "СПОКОЙНО", "БЛИЗКО", "СТАТИЧНО"}};
    const std::array<std::string, 5> high_ru{{"ГРУБО", "ПЛОТНО",
        "НЕСТАБИЛЬНО", "ДАЛЕКО", "МЕНЯЕТСЯ"}};
    const int start = macros.y + 39;
    constexpr int row_h = 58;
    for (int i = 0; i < 5; ++i) {
        x_slider(renderer, state,
            {macros.x + 15, start + i * row_h,
                macros.w - 30, row_h},
            macro_name(i, ru(session)),
            percent(values[static_cast<std::size_t>(i)]),
            values[static_cast<std::size_t>(i)],
            kinds[static_cast<std::size_t>(i)], 0, 0, scale + 1,
            colors[static_cast<std::size_t>(i)],
            ru(session) ? low_ru[static_cast<std::size_t>(i)] : "LOW",
            ru(session) ? high_ru[static_cast<std::size_t>(i)] : "HIGH");
    }

    const int card_gap = 9;
    const int cards_top = actors.y + 35;
    const int card_w = (actors.w - 27 - card_gap) / 2;
    const int card_h = (actors.y + actors.h - cards_top - 9 - card_gap) / 2;
    for (int i = 0; i < 4; ++i) {
        a_actor_card(renderer, session, state, telemetry,
            {actors.x + 13 + (i % 2) * (card_w + card_gap),
                cards_top + (i / 2) * (card_h + card_gap),
                card_w, card_h}, i, scale, state.actor == i, true);
    }

    const std::string footer = ru(session) ?
        "КОНЦЕПТ ЭКРАНА «МЕСТО»  •  ВСЕ ЭЛЕМЕНТЫ ВИДИМЫ И НАЖИМАЕМЫ  •  БЕЗ СКРЫТОГО ФОКУСА" :
        "PLACE SCREEN  •  EVERY CONTROL IS VISIBLE AND DIRECTLY TOUCHABLE";
    a_text(renderer, area.x + 15, area.y + area.h - 12,
        footer, aMutedCream, scale);
}

// Exact Android Master screen. Exposes only the real v0.13 global controls
// and four-slot serial master effect chain.

void x_master_global_slider(SDL_Renderer* renderer, UiState& state,
    SDL_Rect rect, std::string label, std::string value, float normalized,
    SliderKind kind, int scale, SDL_Color color,
    std::string low, std::string high) {
    a_card(renderer, rect, aTop, aBorderSoft, 8, false);
    a_text(renderer, rect.x + 14, rect.y + 12, std::move(label),
        aMutedCream, scale);
    a_text(renderer, rect.x + rect.w - 14 - a_text_width(value, scale + 1),
        rect.y + 9, value, color, scale + 1);
    SDL_Rect track{rect.x + 14, rect.y + 49, rect.w - 28, 10};
    a_slider_track(renderer, track, normalized, color);
    a_text(renderer, track.x, rect.y + rect.h - 22,
        std::move(low), aMutedCream, scale);
    a_text(renderer, track.x + track.w - a_text_width(high, scale),
        rect.y + rect.h - 22, std::move(high), aMutedCream, scale);
    add_hit(state, rect, Action::slider, 0, 0, kind);
}

void x_master_fade_card(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect rect, int scale) {
    const float fade = std::clamp(session.performance.fade, 0.0F, 1.0F);
    a_card(renderer, rect, aTop, aPurple, 8, false);
    a_text(renderer, rect.x + 14, rect.y + 12,
        ru(session) ? "ВЫХОД / ФЕЙД" : "OUTPUT / FADE",
        aMutedCream, scale);
    const std::string value = percent(fade);
    a_text(renderer, rect.x + rect.w - 14 - a_text_width(value, scale + 1),
        rect.y + 9, value, aGreen, scale + 1);
    SDL_Rect track{rect.x + 14, rect.y + 49, rect.w - 28, 10};
    a_meter(renderer, track, fade, aGreen);
    const std::string state_text = fade > 0.995F ?
        (ru(session) ? "ВЫХОД ОТКРЫТ" : "OUTPUT OPEN") :
        (fade < 0.005F ? (ru(session) ? "ТИШИНА" : "SILENT") :
            (ru(session) ? "ИДЁТ ФЕЙД" : "FADING"));
    a_text(renderer, track.x, rect.y + rect.h - 22,
        state_text, fade > 0.995F ? aGreen : aMutedCream, scale);
    add_hit(state, rect, Action::fade);
}

void x_master_chain_card(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect rect, int index, int scale) {
    const auto& effect = session.master_effects[static_cast<std::size_t>(index)];
    const bool selected = state.master_fx == index;
    const bool configured = effect.kind != cd::EffectKind::bypass;
    const bool active = configured && effect.enabled;
    const SDL_Color color = aActor[static_cast<std::size_t>(index)];
    a_card(renderer, rect, selected ? aPanel2 : aTop,
        selected ? color : (active ? color : aBorderSoft),
        8, selected);
    a_text(renderer, rect.x + 13, rect.y + 13,
        std::to_string(index + 1), color, scale + 1);
    a_text(renderer, rect.x + 41, rect.y + 14,
        "MASTER FX", aMutedCream, scale);
    a_text(renderer, rect.x + 135, rect.y + 11,
        effect_name(effect.kind, ru(session)),
        active ? aCream : aMutedCream, scale + 1);
    SDL_Rect pill{rect.x + rect.w - 112, rect.y + 10, 98, 30};
    a_card(renderer, pill,
        active ? a_mix(aPanel2, color, 0.12F) : aPanel2,
        active ? color : aBorderSoft, 7, false);
    a_centered_text(renderer, pill,
        active ? (ru(session) ? "АКТИВЕН" : "ACTIVE") :
            (configured ? (ru(session) ? "ВЫКЛ" : "OFF") :
                (ru(session) ? "ПУСТО" : "EMPTY")),
        active ? color : aMutedCream, scale);
    add_hit(state, rect, Action::master_fx_select, index);
    if (configured) add_hit(state, pill, Action::master_fx_toggle, index);
}

void x_master_editor_exact(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect rect, int scale) {
    const int index = std::clamp(state.master_fx, 0, 3);
    auto& effect = session.master_effects[static_cast<std::size_t>(index)];
    const SDL_Color color = aActor[static_cast<std::size_t>(index)];
    const bool configured = effect.kind != cd::EffectKind::bypass;
    const bool active = configured && effect.enabled;
    const int parameter_count = effect_parameter_count(effect.kind);
    a_card(renderer, rect, aPanel2, color, 9, true);

    a_text(renderer, rect.x + 16, rect.y + 13,
        std::to_string(index + 1), color, scale + 2);
    a_text(renderer, rect.x + 51, rect.y + 15,
        "MASTER FX", aMutedCream, scale + 1);
    a_text(renderer, rect.x + 164, rect.y + 12,
        effect_name(effect.kind, ru(session)), aCream, scale + 2);

    SDL_Rect state_pill{rect.x + rect.w - 142, rect.y + 10, 128, 32};
    a_card(renderer, state_pill,
        active ? a_mix(aPanel2, color, 0.14F) : aTop,
        active ? color : aBorderSoft, 7, false);
    a_centered_text(renderer, state_pill,
        active ? (ru(session) ? "АКТИВЕН" : "ACTIVE") :
            (configured ? (ru(session) ? "ВЫКЛ" : "OFF") :
                (ru(session) ? "ПУСТО" : "EMPTY")),
        active ? color : aMutedCream, scale);
    if (configured) add_hit(state, state_pill, Action::master_fx_toggle, index);

    const int header_h = 52;
    SDL_Rect type_panel{rect.x + 14, rect.y + header_h,
        264, rect.h - header_h - 14};
    a_card(renderer, type_panel, aTop, aBorderSoft, 8, false);
    a_text(renderer, type_panel.x + 13, type_panel.y + 12,
        ru(session) ? "ТИП ЭФФЕКТА" : "EFFECT TYPE",
        aMutedCream, scale);
    SDL_Rect picker{type_panel.x + 13, type_panel.y + 36,
        type_panel.w - 26, 45};
    x_button(renderer, state, picker, effect_name(effect.kind, ru(session)),
        Action::master_fx_picker, 0, 0, scale + 1, true, color);
    a_text(renderer, type_panel.x + 13, type_panel.y + 98,
        ru(session) ? "ПОЗИЦИЯ В ЦЕПИ" : "CHAIN POSITION",
        aMutedCream, scale);
    a_text(renderer, type_panel.x + 13, type_panel.y + 121,
        std::to_string(index + 1) + " / 4", color, scale + 2);
    a_text(renderer, type_panel.x + 13,
        type_panel.y + type_panel.h - 23,
        ru(session) ? "ОБРАБОТКА ПОСЛЕ МИКСА" : "POST-MIX PROCESSING",
        aMutedCream, scale);

    const int controls_x = type_panel.x + type_panel.w + 18;
    const int controls_w = rect.x + rect.w - controls_x - 14;
    const int row_h = (rect.h - header_h - 12) / 3;
    const std::array<float, 3> values{{effect.amount, effect.tone,
        effect.feedback}};
    const std::array<SliderKind, 3> kinds{{
        SliderKind::master_fx_amount, SliderKind::master_fx_tone,
        SliderKind::master_fx_feedback}};
    const std::array<SDL_Color, 3> colors{{color, aBlue, aPurple}};
    for (int i = 0; i < 3; ++i) {
        SDL_Rect row{controls_x, rect.y + header_h + i * row_h,
            controls_w, row_h};
        if (i < parameter_count) {
            x_slider(renderer, state, row,
                x_fx_parameter_label(i, ru(session)),
                percent(values[static_cast<std::size_t>(i)]),
                values[static_cast<std::size_t>(i)],
                kinds[static_cast<std::size_t>(i)], 0, 0, scale + 1,
                colors[static_cast<std::size_t>(i)], "0%", "100%");
        } else {
            a_text(renderer, row.x, row.y + 5,
                x_fx_parameter_label(i, ru(session)),
                aMutedCream, scale + 1);
            a_text(renderer, row.x + 192 - a_text_width("—", scale + 1),
                row.y + 5, "—", aMutedCream, scale + 1);
            SDL_Rect track{row.x + 218, row.y + 12, row.w - 226, 9};
            a_round_fill(renderer, track, 4, aTrack);
            a_text(renderer, track.x, row.y + 28,
                ru(session) ? "НЕ ИСПОЛЬЗУЕТСЯ" : "NOT USED",
                aMutedCream, scale);
            a_hline(renderer, row.x, row.y + row.h - 1, row.w);
        }
    }
}

void a_master_exact(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect area, int scale) {
    area.x -= 1;
    area.w += 2;
    area.h += 7;
    a_card(renderer, area, aPanel2, aBorderSoft, 9, false);

    const int pad = 12;
    const int gap = 10;
    const int top_h = 105;
    const int global_w = (area.w - 2 * pad - 2 * gap) / 3;
    x_master_global_slider(renderer, state,
        {area.x + pad, area.y + pad, global_w, top_h},
        ru(session) ? "ОБЩИЙ УРОВЕНЬ" : "MASTER LEVEL",
        percent(session.master_level), session.master_level,
        SliderKind::master_level, scale + 1, aGreen, "MUTE", "100%");
    x_master_global_slider(renderer, state,
        {area.x + pad + global_w + gap, area.y + pad, global_w, top_h},
        ru(session) ? "ТЕМП" : "TEMPO",
        std::to_string(static_cast<int>(std::lround(session.tempo_bpm))) + " BPM",
        (session.tempo_bpm - 10.0F) / 290.0F,
        SliderKind::tempo, scale + 1, aOrange, "10 BPM", "300 BPM");
    x_master_fade_card(renderer, session, state,
        {area.x + pad + 2 * (global_w + gap), area.y + pad,
            global_w, top_h}, scale + 1);

    const int body_y = area.y + pad + top_h + gap;
    const int body_h = area.y + area.h - pad - body_y;
    const int chain_w = 430;
    SDL_Rect chain{area.x + pad, body_y, chain_w, body_h};
    SDL_Rect editor{chain.x + chain.w + gap, body_y,
        area.x + area.w - pad - (chain.x + chain.w + gap), body_h};
    a_card(renderer, chain, aTop, aBorderSoft, 8, false);
    a_text(renderer, chain.x + 14, chain.y + 11,
        ru(session) ? "ЦЕПЬ MASTER FX" : "MASTER FX CHAIN",
        aCream, scale + 1);
    const std::string order = "1 > 2 > 3 > 4";
    a_text(renderer, chain.x + chain.w - 14 - a_text_width(order, scale),
        chain.y + 14, order, aMutedCream, scale);

    const int list_top = chain.y + 45;
    const int card_gap = 8;
    const int card_h = (chain.y + chain.h - list_top - 12 - 3 * card_gap) / 4;
    for (int i = 0; i < 4; ++i) {
        SDL_Rect card{chain.x + 12,
            list_top + i * (card_h + card_gap), chain.w - 24, card_h};
        x_master_chain_card(renderer, session, state, card, i, scale);
        if (i < 3) {
            x_fx_chain_arrow(renderer, chain.x + chain.w / 2,
                card.y + card.h + 1, aBorderSoft);
        }
    }
    x_master_editor_exact(renderer, session, state, editor, scale);
}

// Exact Android FX screen. Uses the approved Place/Actor visual system and
// the real v0.13 four-slot serial effect model.

std::string x_fx_parameter_label(int index, bool russian) {
    constexpr std::array<std::string_view, 3> en{{"MIX", "CHARACTER", "FEEDBACK"}};
    constexpr std::array<std::string_view, 3> rr{{"СМЕСЬ", "ХАРАКТЕР", "ОБРАТНАЯ СВЯЗЬ"}};
    return std::string{(russian ? rr : en)[static_cast<std::size_t>(index)]};
}

void x_fx_chain_arrow(SDL_Renderer* renderer, int x, int y, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(renderer, x, y, x, y + 7);
    SDL_RenderDrawLine(renderer, x - 4, y + 4, x, y + 8);
    SDL_RenderDrawLine(renderer, x + 4, y + 4, x, y + 8);
}

void x_fx_parameter_exact(SDL_Renderer* renderer, UiState& state, SDL_Rect rect,
    std::string label, float value, SliderKind kind, int scale,
    SDL_Color color, bool enabled) {
    if (enabled) {
        x_slider(renderer, state, rect, std::move(label), percent(value), value,
            kind, 0, 0, scale + 1, color, "0%", "100%");
        return;
    }
    a_text(renderer, rect.x, rect.y + 5, std::move(label), aMutedCream, scale + 1);
    a_text(renderer, rect.x + 192 - a_text_width("—", scale + 1),
        rect.y + 5, "—", aMutedCream, scale + 1);
    SDL_Rect track{rect.x + 218, rect.y + 12, rect.w - 226, 9};
    a_round_fill(renderer, track, 4, aTrack);
    a_text(renderer, track.x, rect.y + 28,
        "NOT USED", aMutedCream, scale);
    a_hline(renderer, rect.x, rect.y + rect.h - 1, rect.w);
}

void x_fx_compact_card_exact(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect rect, int index, int scale) {
    const auto& effect = session.slots[static_cast<std::size_t>(state.actor)]
        .effects[static_cast<std::size_t>(index)];
    const SDL_Color color = aActor[static_cast<std::size_t>(index)];
    const bool active = effect.kind != cd::EffectKind::bypass;
    a_card(renderer, rect, aTop, active ? color : aBorderSoft, 8, false);

    a_text(renderer, rect.x + 13, rect.y + 13,
        std::to_string(index + 1), color, scale + 1);
    a_text(renderer, rect.x + 42, rect.y + 13,
        std::string{"FX "} + std::to_string(index + 1), aMutedCream, scale);
    a_text(renderer, rect.x + 112, rect.y + 10,
        effect_name(effect.kind, ru(session)),
        active ? aCream : aMutedCream, scale + 1);

    SDL_Rect state_pill{rect.x + rect.w - 136, rect.y + 10, 122, 30};
    a_card(renderer, state_pill,
        active ? a_mix(aPanel2, color, 0.12F) : aPanel2,
        active ? color : aBorderSoft, 7, false);
    a_centered_text(renderer, state_pill,
        active ? (ru(session) ? "АКТИВЕН" : "ACTIVE") :
            (ru(session) ? "ПУСТО" : "EMPTY"),
        active ? color : aMutedCream, scale);

    const int meter_x = rect.x + rect.w - 310;
    a_text(renderer, meter_x, rect.y + 12,
        ru(session) ? "СМЕСЬ" : "MIX", aMutedCream, scale);
    a_meter(renderer, {meter_x + 72, rect.y + 17, 88, 7},
        active ? effect.amount : 0.0F, active ? color : aMutedCream);
    add_hit(state, rect, Action::actor_fx_select, index);
}

void x_fx_expanded_card_exact(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect rect, int index, int scale) {
    auto& effect = session.slots[static_cast<std::size_t>(state.actor)]
        .effects[static_cast<std::size_t>(index)];
    const SDL_Color color = aActor[static_cast<std::size_t>(index)];
    const bool active = effect.kind != cd::EffectKind::bypass;
    const int parameter_count = effect_parameter_count(effect.kind);
    a_card(renderer, rect, aPanel2, color, 9, true);

    const int header_h = 52;
    a_text(renderer, rect.x + 14, rect.y + 13,
        std::to_string(index + 1), color, scale + 2);
    a_text(renderer, rect.x + 49, rect.y + 15,
        std::string{"FX "} + std::to_string(index + 1), aMutedCream, scale + 1);
    a_text(renderer, rect.x + 116, rect.y + 12,
        effect_name(effect.kind, ru(session)), aCream, scale + 2);

    SDL_Rect state_pill{rect.x + rect.w - 142, rect.y + 10, 128, 32};
    a_card(renderer, state_pill,
        active ? a_mix(aPanel2, color, 0.14F) : aTop,
        active ? color : aBorderSoft, 7, false);
    a_centered_text(renderer, state_pill,
        active ? (ru(session) ? "АКТИВЕН" : "ACTIVE") :
            (ru(session) ? "ОБХОД" : "BYPASS"),
        active ? color : aMutedCream, scale);

    SDL_Rect type_panel{rect.x + 14, rect.y + header_h,
        278, rect.h - header_h - 14};
    a_card(renderer, type_panel, aTop, aBorderSoft, 8, false);
    a_text(renderer, type_panel.x + 13, type_panel.y + 12,
        ru(session) ? "ТИП ЭФФЕКТА" : "EFFECT TYPE", aMutedCream, scale);
    SDL_Rect picker{type_panel.x + 13, type_panel.y + 36,
        type_panel.w - 26, 46};
    x_button(renderer, state, picker, effect_name(effect.kind, ru(session)),
        Action::actor_fx_picker, 0, 0, scale + 1, true, color);
    a_text(renderer, type_panel.x + 13, type_panel.y + 98,
        ru(session) ? "ПОРЯДОК В ЦЕПИ" : "CHAIN POSITION",
        aMutedCream, scale);
    a_text(renderer, type_panel.x + 13, type_panel.y + 120,
        std::to_string(index + 1) + " / 4", color, scale + 2);
    const std::string hint = active ?
        (ru(session) ? "НАЖМИТЕ, ЧТОБЫ СМЕНИТЬ" : "TAP TO CHANGE") :
        (ru(session) ? "ВЫБЕРИТЕ ЭФФЕКТ" : "CHOOSE AN EFFECT");
    a_text(renderer, type_panel.x + 13,
        type_panel.y + type_panel.h - 23, hint, aMutedCream, scale);

    const int controls_x = type_panel.x + type_panel.w + 18;
    const int controls_w = rect.x + rect.w - controls_x - 14;
    const int row_h = (rect.h - header_h - 12) / 3;
    const std::array<float, 3> values{{effect.amount, effect.tone,
        effect.feedback}};
    const std::array<SliderKind, 3> kinds{{
        SliderKind::actor_fx_amount, SliderKind::actor_fx_tone,
        SliderKind::actor_fx_feedback}};
    const std::array<SDL_Color, 3> colors{{color, aBlue, aPurple}};
    for (int i = 0; i < 3; ++i) {
        const SDL_Rect row{controls_x, rect.y + header_h + i * row_h,
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

void a_fx_exact(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, SDL_Rect area, int scale) {
    area.x -= 1;
    area.w += 2;
    area.h += 7;
    const int gap = 10;
    const int side_w = 300;
    SDL_Rect side{area.x, area.y, side_w, area.h};
    SDL_Rect main{area.x + side_w + gap, area.y,
        area.w - side_w - gap, area.h};
    x_actor_sidebar_exact(renderer, session, state, telemetry, side, scale);
    a_card(renderer, main, aPanel2, aBorderSoft, 9, false);

    const auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    a_text(renderer, main.x + 18, main.y + 13,
        (ru(session) ? "FX АКТЁРА " : "ACTOR FX ") +
            std::to_string(state.actor + 1), aCream, scale + 1);
    a_text(renderer, main.x + 151, main.y + 13,
        engine_name(slot.engine, ru(session)),
        aActor[static_cast<std::size_t>(state.actor)], scale + 1);
    const std::string chain = ru(session) ?
        "СЕРИЙНАЯ ЦЕПЬ  1 > 2 > 3 > 4" :
        "SERIAL CHAIN  1 > 2 > 3 > 4";
    a_text(renderer, main.x + main.w - 18 - a_text_width(chain, scale),
        main.y + 15, chain, aMutedCream, scale);

    const int top = main.y + 48;
    const int card_gap = 8;
    const int compact_h = 58;
    const int available_h = main.y + main.h - top - 12;
    const int expanded_h = available_h - 3 * compact_h - 3 * card_gap;
    int y = top;
    for (int i = 0; i < 4; ++i) {
        const bool selected = state.actor_fx == i;
        const int height = selected ? expanded_h : compact_h;
        SDL_Rect card{main.x + 12, y, main.w - 24, height};
        if (selected) {
            x_fx_expanded_card_exact(renderer, session, state,
                card, i, scale);
        } else {
            x_fx_compact_card_exact(renderer, session, state,
                card, i, scale);
        }
        y += height;
        if (i < 3) {
            x_fx_chain_arrow(renderer, main.x + main.w / 2,
                y + 1, aBorderSoft);
            y += card_gap;
        }
    }
}

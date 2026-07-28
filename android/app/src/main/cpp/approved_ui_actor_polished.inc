// Focused Actor/Sound surface: expose the parameters that materially change
// the Musical/Plaits engine and keep pitch quantising to two understandable
// choices (scale and root).

void x_actor_quantiser_compact(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect area, int scale, SDL_Color accent) {
    a_text(renderer, area.x, area.y + 5,
        ru(session) ? "КВАНТОВАНИЕ ВЫСОТЫ" : "PITCH QUANTISER",
        aCream, scale + 1);

    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    SDL_Rect enabled{area.x + area.w - 106, area.y, 106, 32};
    x_button(renderer, state, enabled,
        slot.tuning.enabled ? (ru(session) ? "ВКЛ" : "ON") :
            (ru(session) ? "ВЫКЛ" : "OFF"),
        Action::tuning_toggle, 0, 0, scale, slot.tuning.enabled,
        slot.tuning.enabled ? aGreen : aRed);

    SDL_Rect scale_pick{area.x, area.y + 42, area.w, 42};
    x_button(renderer, state, scale_pick,
        std::string{ru(session) ? "ГАММА: " : "SCALE: "} +
            std::string{slot.tuning.name.data()} + "  ▼",
        Action::actor_section, 99, 0, scale + 1, slot.tuning.enabled,
        aPurple);

    a_text(renderer, area.x, area.y + 94,
        ru(session) ? "КОРЕНЬ" : "ROOT", aCream, scale);
    const int controls_y = area.y + 112;
    SDL_Rect previous{area.x, controls_y, 48, 40};
    SDL_Rect next{area.x + area.w - 48, controls_y, 48, 40};
    SDL_Rect note{previous.x + previous.w + 7, controls_y,
        area.w - previous.w - next.w - 14, 40};
    x_button(renderer, state, previous, "<", Action::actor_root_step,
        -1, 0, scale + 1, false, aPurple);
    a_card(renderer, note, aPanel2,
        slot.tuning.enabled ? accent : aBorderSoft, 7, false);
    a_centered_text(renderer, note,
        x_actor_midi_note(slot.tuning.root_midi) + "   MIDI " +
            std::to_string(slot.tuning.root_midi),
        slot.tuning.enabled ? aGreen : aMutedCream, scale + 1);
    x_button(renderer, state, next, ">", Action::actor_root_step,
        1, 0, scale + 1, false, aPurple);
}

void x_actor_sound_exact(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect area, int scale) {
    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    const bool plaits = slot.engine == cd::EngineKind::plaits;
    const int gap = 10;
    const int left_w = static_cast<int>(area.w * 0.64F);
    SDL_Rect left{area.x, area.y, left_w - gap, area.h};
    SDL_Rect right{area.x + left_w, area.y, area.w - left_w, area.h};
    a_card(renderer, left, aTop, aBorderSoft, 8, false);
    a_card(renderer, right, aTop, aBorderSoft, 8, false);

    const std::array<std::string, 7> labels = plaits
        ? std::array<std::string, 7>{{
            ru(session) ? "ЧАСТОТА" : "FREQUENCY",
            ru(session) ? "ГАРМОНИКИ" : "HARMONICS",
            ru(session) ? "ТЕМБР" : "TIMBRE",
            ru(session) ? "МОРФ" : "MORPH",
            ru(session) ? "ЗАТУХАНИЕ" : "DECAY",
            ru(session) ? "УРОВЕНЬ" : "LEVEL",
            "PAN"}}
        : std::array<std::string, 7>{{
            ru(session) ? "ЧАСТОТА" : "FREQUENCY",
            ru(session) ? "ТЕМБР" : "TIMBRE",
            ru(session) ? "ЦВЕТ" : "COLOR",
            ru(session) ? "ДВИЖЕНИЕ" : "MOTION",
            ru(session) ? "ТЕКСТУРА" : "TEXTURE",
            ru(session) ? "УРОВЕНЬ" : "LEVEL",
            "PAN"}};

    const std::array<float, 7> norms{{
        cd::mapping::normalized_frequency(slot.frequency_hz),
        slot.timbre, slot.color, slot.motion, slot.texture, slot.level,
        (slot.pan + 1.0F) * 0.5F}};
    const std::array<SliderKind, 7> kinds{{
        SliderKind::actor_frequency, SliderKind::actor_timbre,
        SliderKind::actor_color, SliderKind::actor_motion,
        SliderKind::actor_texture, SliderKind::actor_level,
        SliderKind::actor_pan}};
    const std::array<SDL_Color, 7> colors{{
        aGreen, aOrange, aYellow, aBlue, aPurple, aGreen, aMutedCream}};
    const std::array<std::string, 7> values{{
        decimal(slot.frequency_hz, "HZ"),
        percent(slot.timbre), percent(slot.color), percent(slot.motion),
        percent(slot.texture),
        decimal(20.0F * std::log10(std::max(slot.level, 0.0001F)), "DB"),
        signed_percent(slot.pan)}};

    const std::array<std::string, 7> low = plaits
        ? std::array<std::string, 7>{{
            "20 HZ", ru(session) ? "ЧИСТО" : "PURE",
            ru(session) ? "ТЁМНЫЙ" : "DARK", "A",
            ru(session) ? "КОРОТКО" : "SHORT", "MUTE",
            ru(session) ? "ЛЕВО" : "LEFT"}}
        : std::array<std::string, 7>{{
            "20 HZ", ru(session) ? "МЯГКО" : "SOFT",
            ru(session) ? "ТЁМНЫЙ" : "DARK",
            ru(session) ? "СТАТИЧНО" : "STILL",
            ru(session) ? "ГЛАДКАЯ" : "SMOOTH", "MUTE",
            ru(session) ? "ЛЕВО" : "LEFT"}};
    const std::array<std::string, 7> high = plaits
        ? std::array<std::string, 7>{{
            "2 KHZ", ru(session) ? "БОГАТО" : "RICH",
            ru(session) ? "ЯРКИЙ" : "BRIGHT", "B",
            ru(session) ? "ДОЛГО" : "LONG", "0 DB",
            ru(session) ? "ПРАВО" : "RIGHT"}}
        : std::array<std::string, 7>{{
            "2 KHZ", ru(session) ? "ЯРКО" : "BRIGHT",
            ru(session) ? "СВЕТЛЫЙ" : "LIGHT",
            ru(session) ? "ДВИЖЕТСЯ" : "MOVING",
            ru(session) ? "ЗЕРНИСТАЯ" : "GRAINY", "0 DB",
            ru(session) ? "ПРАВО" : "RIGHT"}};

    const int start = left.y + 8;
    const int row_h = (left.h - 16) / 7;
    for (int i = 0; i < 7; ++i) {
        x_slider(renderer, state,
            {left.x + 12, start + i * row_h, left.w - 24, row_h},
            labels[static_cast<std::size_t>(i)],
            values[static_cast<std::size_t>(i)],
            norms[static_cast<std::size_t>(i)],
            kinds[static_cast<std::size_t>(i)], 0, 0, scale + 1,
            colors[static_cast<std::size_t>(i)],
            low[static_cast<std::size_t>(i)],
            high[static_cast<std::size_t>(i)]);
    }

    const int pad = 12;
    SDL_Rect panel{right.x + pad, right.y + pad,
        right.w - 2 * pad, right.h - 2 * pad};
    const SDL_Color accent = aActor[static_cast<std::size_t>(state.actor)];

    int quantiser_y = panel.y;
    if (plaits) {
        a_text(renderer, panel.x, panel.y + 4,
            ru(session) ? "МОДЕЛЬ PLAITS" : "PLAITS MODEL",
            aCream, scale + 1);
        SDL_Rect model{panel.x, panel.y + 25, panel.w, 48};
        x_button(renderer, state, model,
            plaits_model_name(slot.plaits_model) + "  ▼",
            Action::actor_section, 97, 0, scale + 1, true, aOrange);

        a_text(renderer, panel.x, panel.y + 84,
            ru(session) ? "МАРШРУТИЗАЦИЯ ВЫХОДА" : "OUTPUT ROUTING",
            aCream, scale);
        SDL_Rect output{panel.x, panel.y + 104, panel.w, 44};
        x_button(renderer, state, output,
            std::string{"MAIN / AUX: "} + output_name(slot.plaits_output) +
                "  ▼",
            Action::actor_section, 98, 0, scale + 1, true, aBlue);

        a_hline(renderer, panel.x, panel.y + 160, panel.w, aBorderSoft);
        quantiser_y = panel.y + 174;
    }

    SDL_Rect quantiser{panel.x, quantiser_y, panel.w,
        panel.y + panel.h - quantiser_y};
    x_actor_quantiser_compact(renderer, session, state, quantiser, scale,
        accent);
}

void a_actor(SDL_Renderer* renderer, cd::Session& session, UiState& state,
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

    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    const SDL_Color color = aActor[static_cast<std::size_t>(state.actor)];
    const int header_h = 92;
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
    a_text(renderer, source_x, main.y + 8,
        ru(session) ? "ИСТОЧНИК" : "SOURCE", aMutedCream, scale);
    SDL_Rect landscape{source_x, header_control_y, 112, header_control_h};
    SDL_Rect musical{landscape.x + landscape.w + 5, landscape.y, 124, 38};
    x_button(renderer, state, landscape,
        ru(session) ? "ЛАНДШАФТ" : "LANDSCAPE",
        Action::actor_section, kSourceLandscape, 0, scale,
        slot.engine != cd::EngineKind::plaits, color);
    x_button(renderer, state, musical,
        ru(session) ? "МУЗЫКАЛЬНЫЙ" : "MUSICAL",
        Action::actor_section, kSourceMusical, 0, scale,
        slot.engine == cd::EngineKind::plaits, color);

    const int engine_x = musical.x + musical.w + 22;
    a_text(renderer, engine_x, main.y + 8,
        ru(session) ? "ДВИЖОК" : "ENGINE", aMutedCream, scale);
    SDL_Rect engine{engine_x, header_control_y, 162, header_control_h};
    x_button(renderer, state, engine, engine_name(slot.engine, ru(session)),
        Action::engine_picker, 0, 0, scale, false, color);

    const int trigger_w = 138;
    const int trigger_x = main.x + main.w - trigger_w - 14;
    if (cd::supports_event_rate(slot.engine)) {
        const float density = cd::effective_event_density(
            slot.event_density, session.performance.events);
        const std::string rate = decimal(cd::event_rate_hz(
            session.tempo_bpm, density, slot.motion), "/S");
        const std::string wait = decimal(cd::event_max_wait_seconds(
            session.tempo_bpm, density, slot.motion), "S");
        a_text(renderer, trigger_x - 220, main.y + 8,
            "EVENT RATE", aMutedCream, scale);
        a_text(renderer, trigger_x - 220, main.y + 31,
            rate, color, scale + 1);
        a_text(renderer, trigger_x - 105, main.y + 8,
            ru(session) ? "МАКС. ОЖИДАНИЕ" : "MAX WAIT", aMutedCream, scale);
        a_text(renderer, trigger_x - 105, main.y + 31,
            wait, color, scale + 1);
    }
    if (cd::supports_manual_trigger(slot.engine)) {
        x_button(renderer, state,
            {trigger_x, header_control_y, trigger_w, header_control_h},
            ru(session) ? "ЗАПУСК >" : "TRIGGER >",
            Action::actor_trigger, state.actor, 0, scale + 1, true, color);
    }

    constexpr std::array<ActorSection, 3> sections{{
        ActorSection::sound, ActorSection::events, ActorSection::modulation}};
    const std::array<std::string, 3> section_labels{{
        ru(session) ? "ЗВУК" : "SOUND",
        ru(session) ? "СОБЫТИЯ" : "EVENTS",
        ru(session) ? "МОДУЛЯЦИЯ" : "MODULATION"}};
    const int tabs_y = main.y + header_h;
    const int tab_gap = 7;
    const int tab_w = (main.w - 24 - 2 * tab_gap) / 3;
    for (int i = 0; i < 3; ++i) {
        x_button(renderer, state,
            {main.x + 12 + i * (tab_w + tab_gap), tabs_y, tab_w, 43},
            section_labels[static_cast<std::size_t>(i)], Action::actor_section,
            static_cast<int>(sections[static_cast<std::size_t>(i)]), 0,
            scale + 1,
            state.actor_section == sections[static_cast<std::size_t>(i)],
            aPurple);
    }

    SDL_Rect body{main.x + 12, tabs_y + 53,
        main.w - 24, main.y + main.h - tabs_y - 65};
    switch (state.actor_section) {
    case ActorSection::sound:
        x_actor_sound_exact(renderer, session, state, body, scale);
        break;
    case ActorSection::events:
        x_actor_events_exact(renderer, session, state, telemetry, body, scale);
        break;
    case ActorSection::modulation:
        x_actor_mod_exact(renderer, session, state, body, scale);
        break;
    }
}

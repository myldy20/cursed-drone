void a_actor_approved(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, const cd::AudioTelemetry& telemetry, SDL_Rect area,
    int scale) {
    const int gap = 8;
    const int side_w = std::clamp(area.w / 5, 250, 286);
    SDL_Rect side{area.x, area.y, side_w, area.h};
    SDL_Rect main{area.x + side_w + gap, area.y,
        area.w - side_w - gap, area.h};
    a_actor_sidebar(renderer, session, state, telemetry, side, scale);
    a_panel(renderer, main);

    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    const SDL_Color color = aActor[static_cast<std::size_t>(state.actor)];
    const bool supports_rate = cd::supports_event_rate(slot.engine);
    const bool can_trigger = cd::supports_manual_trigger(slot.engine);
    const int header_h = 82;

    text(renderer, main.x + 20, main.y + 18,
        std::to_string(state.actor + 1), color, scale + 1);
    text(renderer, main.x + 58, main.y + 18,
        engine_name(slot.engine, ru(session)), aCream, scale + 1);
    if (can_trigger) {
        SDL_Rect event_badge{main.x + 176, main.y + 16, 94, 28};
        a_card(renderer, event_badge, aPanel2, color, 5, false);
        centered_text(renderer, event_badge,
            ru(session) ? "СОБЫТИЕ" : "EVENT", color,
            std::max(1, scale - 1));
    }

    const int source_x = main.x + 292;
    text(renderer, source_x, main.y + 12,
        ru(session) ? "ИСТОЧНИК" : "SOURCE", aMutedCream,
        std::max(1, scale - 1));
    const int source_w = 112;
    a_button(renderer, state, {source_x, main.y + 31, source_w, 28},
        ru(session) ? "ЛАНДШАФТ" : "LANDSCAPE",
        Action::actor_section, kSourceLandscape, 0,
        std::max(1, scale - 1), slot.engine != cd::EngineKind::plaits,
        color);
    a_button(renderer, state, {source_x + source_w + 5, main.y + 31,
        source_w, 28}, ru(session) ? "МУЗЫКАЛЬНЫЙ" : "MUSICAL",
        Action::actor_section, kSourceMusical, 0,
        std::max(1, scale - 1), slot.engine == cd::EngineKind::plaits,
        color);

    const int engine_x = source_x + 2 * source_w + 30;
    text(renderer, engine_x, main.y + 12,
        ru(session) ? "ДВИЖОК" : "ENGINE", aMutedCream,
        std::max(1, scale - 1));
    a_button(renderer, state, {engine_x, main.y + 31, 160, 28},
        engine_name(slot.engine, ru(session)), Action::engine_picker,
        0, 0, std::max(1, scale - 1), true, color);

    if (supports_rate) {
        const float density = cd::effective_event_density(
            slot.event_density, session.performance.events);
        const std::string rate = decimal(cd::event_rate_hz(
            session.tempo_bpm, density, slot.motion), "/S");
        const std::string wait = decimal(cd::event_max_wait_seconds(
            session.tempo_bpm, density, slot.motion), "S");
        const int rate_x = engine_x + 185;
        text(renderer, rate_x, main.y + 12, "EVENT RATE",
            aMutedCream, std::max(1, scale - 1));
        text(renderer, rate_x, main.y + 35, rate, color, scale + 1);
        text(renderer, rate_x + 112, main.y + 12,
            ru(session) ? "МАКС. ОЖИДАНИЕ" : "MAX WAIT",
            aMutedCream, std::max(1, scale - 1));
        text(renderer, rate_x + 112, main.y + 35, wait, color, scale + 1);
    }
    if (can_trigger) {
        SDL_Rect trigger{main.x + main.w - 142, main.y + 19, 124, 40};
        a_button(renderer, state, trigger,
            ru(session) ? "ЗАПУСК >" : "TRIGGER >",
            Action::actor_trigger, state.actor, 0, scale, true, color);
    }

    const int tabs_y = main.y + header_h;
    const int tab_gap = 5;
    const int tab_w = (main.w - 24 - 2 * tab_gap) / 3;
    constexpr std::array<ActorSection, 3> sections{{ActorSection::sound,
        ActorSection::events, ActorSection::modulation}};
    const std::array<std::string, 3> labels{{
        ru(session) ? "ЗВУК" : "SOUND",
        ru(session) ? "СОБЫТИЯ" : "EVENTS",
        ru(session) ? "МОДУЛЯЦИЯ" : "MODULATION"}};
    for (int i = 0; i < 3; ++i) {
        const bool active = state.actor_section ==
            sections[static_cast<std::size_t>(i)];
        SDL_Rect tab{main.x + 12 + i * (tab_w + tab_gap), tabs_y,
            tab_w, 37};
        a_card(renderer, tab, active ? aActive : aTop,
            active ? kPurple : aBorderSoft, 5, false);
        centered_text(renderer, tab, labels[static_cast<std::size_t>(i)],
            active ? aCream : aMutedCream, scale);
        add_hit(state, tab, Action::actor_section,
            static_cast<int>(sections[static_cast<std::size_t>(i)]));
    }

    SDL_Rect body{main.x + 12, tabs_y + 48, main.w - 24,
        main.y + main.h - tabs_y - 60};
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
}

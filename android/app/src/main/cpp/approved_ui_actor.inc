void a_actor_sidebar(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, const cd::AudioTelemetry& telemetry, SDL_Rect area,
    int scale) {
    a_panel(renderer, area);
    text(renderer, area.x + 14, area.y + 13,
        ru(session) ? "АКТЕРЫ" : "ACTORS", kInk, scale);
    const int gap = 9;
    const int top = area.y + 45;
    const int card_h = (area.h - 54 - 3 * gap) / 4;
    for (int i = 0; i < 4; ++i)
        a_actor_card(renderer, session, state, telemetry,
            {area.x + 9, top + i * (card_h + gap), area.w - 18, card_h},
            i, scale, state.actor == i, false);
}

void a_actor_sound(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect area, int scale) {
    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    const int gap = 10;
    const int left_w = static_cast<int>(area.w * 0.65F);
    SDL_Rect left{area.x, area.y, left_w - gap, area.h};
    SDL_Rect right{area.x + left_w, area.y, area.w - left_w, area.h};
    a_panel(renderer, left, aPanel2); a_panel(renderer, right, aPanel2);
    const std::array<std::string, 7> labels{{
        ru(session) ? "ЧАСТОТА" : "FREQUENCY",
        ru(session) ? "ТЕМБР" : "TIMBRE",
        ru(session) ? "ЦВЕТ" : "COLOR",
        ru(session) ? "ДВИЖЕНИЕ" : "MOTION",
        ru(session) ? "ТЕКСТУРА" : "TEXTURE",
        ru(session) ? "УРОВЕНЬ" : "LEVEL",
        ru(session) ? "ПАНОРАМА" : "PAN"}};
    const std::array<float, 7> norms{{normalized_frequency(slot.frequency_hz),
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
    const int rg = 7;
    const int rh = (left.h - 18 - 6 * rg) / 7;
    for (int i = 0; i < 7; ++i)
        a_slider(renderer, state,
            {left.x + 9, left.y + 9 + i * (rh + rg), left.w - 18, rh},
            labels[static_cast<std::size_t>(i)],
            values[static_cast<std::size_t>(i)],
            norms[static_cast<std::size_t>(i)],
            kinds[static_cast<std::size_t>(i)], 0, 0, scale,
            colors[static_cast<std::size_t>(i)]);

    text(renderer, right.x + 14, right.y + 14,
        ru(session) ? "СТРОЙ" : "TUNING", kInk, scale);
    const int bh = std::max(64, right.h / 9);
    a_button(renderer, state, {right.x + 12, right.y + 48,
        right.w - 24, bh}, std::string{slot.tuning.name.data()},
        Action::actor_section, 99, 0, scale, true, kPurple);
    a_slider(renderer, state, {right.x + 12, right.y + 48 + bh + gap,
        right.w - 24, bh}, ru(session) ? "КОРЕНЬ MIDI" : "ROOT MIDI",
        std::to_string(slot.tuning.root_midi),
        (static_cast<float>(slot.tuning.root_midi) - 24.0F) / 72.0F,
        SliderKind::tuning_root, 0, 0, scale, kGreen);
    int y = right.y + 48 + 2 * (bh + gap);
    if (slot.engine == cd::EngineKind::plaits) {
        a_button(renderer, state, {right.x + 12, y, right.w - 24, bh},
            plaits_model_name(slot.plaits_model), Action::actor_section,
            97, 0, scale, true, kActorColors[1]);
        y += bh + gap;
        a_button(renderer, state, {right.x + 12, y, right.w - 24, bh},
            std::string{"OUTPUT: "} + output_name(slot.plaits_output),
            Action::actor_section, 98, 0, scale, true, kActorColors[3]);
    } else {
        a_panel(renderer, {right.x + 12, y, right.w - 24,
            right.y + right.h - y - 12}, aPanel, kBorder);
        text(renderer, right.x + 28, y + 22,
            ru(session) ? "ЛАНДШАФТНЫЙ ДВИЖОК" : "LANDSCAPE ENGINE",
            kDim, std::max(1, scale - 1));
        text(renderer, right.x + 28, y + 58,
            engine_name(slot.engine, ru(session)),
            aActor[static_cast<std::size_t>(state.actor)], scale);
    }
}

void a_actor_events(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect area, int scale) {
    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    const int gap = 12;
    const int half = (area.w - gap) / 2;
    SDL_Rect left{area.x, area.y, half, area.h};
    SDL_Rect right{area.x + half + gap, area.y, area.w - half - gap, area.h};
    a_panel(renderer, left, aPanel2); a_panel(renderer, right, aPanel2);
    const float density = cd::effective_event_density(slot.event_density,
        session.performance.events);
    const float rate = cd::event_rate_hz(session.tempo_bpm, density, slot.motion);
    const float wait = cd::event_max_wait_seconds(session.tempo_bpm,
        density, slot.motion);
    text(renderer, left.x + 16, left.y + 16,
        ru(session) ? "СОБЫТИЯ АКТЕРА" : "ACTOR EVENTS", kInk, scale);
    const int sh = std::max(108, left.h / 4);
    a_slider(renderer, state, {left.x + 12, left.y + 52,
        left.w - 24, sh}, ru(session) ? "ЧАСТОТА СОБЫТИЙ" : "EVENT RATE",
        decimal(rate, "/S"), slot.event_density,
        SliderKind::actor_event_density, 0, 0, scale, kActorColors[1],
        ru(session) ? "РЕДКО" : "SPARSE",
        ru(session) ? "ЧАСТО" : "BUSY");
    SDL_Rect wait_box{left.x + 12, left.y + 52 + sh + gap,
        left.w - 24, std::max(72, left.h / 8)};
    a_panel(renderer, wait_box, aPanel, kBorder);
    text(renderer, wait_box.x + 16, wait_box.y + 20,
        ru(session) ? "МАКС. ОЖИДАНИЕ" : "MAX WAIT", kDim, scale);
    const std::string wait_text = decimal(wait, "S");
    text(renderer, wait_box.x + wait_box.w - 16 -
        cd::ui::text_width(wait_text, scale), wait_box.y + 20,
        wait_text, kActorColors[1], scale);
    if (cd::supports_manual_trigger(slot.engine))
        a_button(renderer, state, {left.x + 12,
            left.y + left.h - std::max(82, left.h / 7) - 12,
            left.w - 24, std::max(82, left.h / 7)},
            ru(session) ? "ЗАПУСТИТЬ СОБЫТИЕ" : "TRIGGER EVENT",
            Action::actor_trigger, state.actor, 0, scale + 1,
            true, kActorColors[1]);

    text(renderer, right.x + 16, right.y + 16, "EUCLIDEAN", kInk, scale);
    const int toggle_h = std::max(64, right.h / 9);
    a_button(renderer, state, {right.x + 12, right.y + 52,
        right.w - 24, toggle_h}, slot.euclidean.enabled ?
            (ru(session) ? "ВКЛЮЧЕНО" : "ENABLED") :
            (ru(session) ? "ВЫКЛЮЧЕНО" : "DISABLED"),
        Action::euclidean_toggle, 0, 0, scale,
        slot.euclidean.enabled, kGreen);
    const int top = right.y + 52 + toggle_h + gap;
    const int rh = (right.y + right.h - top - 12 - 3 * gap) / 4;
    a_slider(renderer, state, {right.x + 12, top, right.w - 24, rh},
        ru(session) ? "ШАГИ" : "STEPS", std::to_string(slot.euclidean.steps),
        static_cast<float>(slot.euclidean.steps - 1) / 31.0F,
        SliderKind::euclidean_steps, 0, 0, scale, kGreen);
    a_slider(renderer, state, {right.x + 12, top + rh + gap,
        right.w - 24, rh}, ru(session) ? "ИМПУЛЬСЫ" : "PULSES",
        std::to_string(slot.euclidean.pulses),
        static_cast<float>(slot.euclidean.pulses) /
            static_cast<float>(std::max(1, slot.euclidean.steps)),
        SliderKind::euclidean_pulses, 0, 0, scale, kActorColors[1]);
    a_slider(renderer, state, {right.x + 12, top + 2 * (rh + gap),
        right.w - 24, rh}, ru(session) ? "СДВИГ" : "ROTATION",
        std::to_string(slot.euclidean.rotation),
        static_cast<float>(slot.euclidean.rotation) /
            static_cast<float>(std::max(1, slot.euclidean.steps - 1)),
        SliderKind::euclidean_rotation, 0, 0, scale, kActorColors[3]);
    a_slider(renderer, state, {right.x + 12, top + 3 * (rh + gap),
        right.w - 24, rh}, ru(session) ? "ВЕРОЯТНОСТЬ" : "PROBABILITY",
        percent(slot.euclidean.probability), slot.euclidean.probability,
        SliderKind::euclidean_probability, 0, 0, scale, kPurple);
}

void a_actor_mod(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect area, int scale) {
    auto& mod = session.slots[static_cast<std::size_t>(state.actor)]
        .modulators[static_cast<std::size_t>(state.modulator)];
    const int gap = 12;
    const int select_h = std::max(66, area.h / 9);
    const int sw = (area.w - 3 * gap) / 4;
    for (int i = 0; i < 4; ++i)
        a_button(renderer, state, {area.x + i * (sw + gap), area.y,
            sw, select_h}, std::string{"MOD "} + std::to_string(i + 1),
            Action::mod_select, i, 0, scale, i == state.modulator,
            aActor[static_cast<std::size_t>(i)]);
    const int top = area.y + select_h + gap;
    const int left_w = static_cast<int>(area.w * 0.38F);
    SDL_Rect left{area.x, top, left_w - gap, area.y + area.h - top};
    SDL_Rect right{area.x + left_w, top, area.w - left_w,
        area.y + area.h - top};
    a_panel(renderer, left, aPanel2); a_panel(renderer, right, aPanel2);
    const int bh = std::max(62, left.h / 8);
    a_button(renderer, state, {left.x + 12, left.y + 12,
        left.w - 24, bh}, mod.enabled ?
            (ru(session) ? "МОДУЛЯТОР ВКЛ" : "MODULATOR ON") :
            (ru(session) ? "МОДУЛЯТОР ВЫКЛ" : "MODULATOR OFF"),
        Action::mod_toggle, 0, 0, scale, mod.enabled, kGreen);
    a_button(renderer, state, {left.x + 12, left.y + 12 + bh + gap,
        left.w - 24, bh}, wave_name(mod.wave, ru(session)),
        Action::actor_section, 95, 0, scale, true, kActorColors[1]);
    a_button(renderer, state, {left.x + 12, left.y + 12 + 2 * (bh + gap),
        left.w - 24, bh}, destination_name(mod.destination, ru(session)),
        Action::actor_section, 96, 0, scale, true, kActorColors[3]);
    const std::string source = mod.rate_mod_source < 0 ?
        (ru(session) ? "ИСТОЧНИК: НЕТ" : "SOURCE: NONE") :
        std::string{"SOURCE: MOD "} + std::to_string(mod.rate_mod_source + 1);
    a_button(renderer, state, {left.x + 12,
        left.y + left.h - std::max(66, left.h / 7) - 12,
        left.w - 24, std::max(66, left.h / 7)}, source,
        Action::mod_source_cycle, 0, 0, scale, true, kPurple);
    const int rh = (right.h - 24 - 3 * gap) / 4;
    a_slider(renderer, state, {right.x + 12, right.y + 12,
        right.w - 24, rh}, ru(session) ? "СКОРОСТЬ" : "RATE",
        decimal(mod.rate_hz, "HZ"), normalized_rate(mod.rate_hz),
        SliderKind::mod_rate, 0, 0, scale, kGreen);
    a_slider(renderer, state, {right.x + 12, right.y + 12 + rh + gap,
        right.w - 24, rh}, ru(session) ? "ГЛУБИНА" : "DEPTH",
        signed_percent(mod.depth), (mod.depth + 1.0F) * 0.5F,
        SliderKind::mod_depth, 0, 0, scale, kActorColors[1]);
    a_slider(renderer, state, {right.x + 12, right.y + 12 + 2 * (rh + gap),
        right.w - 24, rh}, ru(session) ? "СМЕЩЕНИЕ" : "OFFSET",
        signed_percent(mod.offset), (mod.offset + 1.0F) * 0.5F,
        SliderKind::mod_offset, 0, 0, scale, kActorColors[3]);
    a_slider(renderer, state, {right.x + 12, right.y + 12 + 3 * (rh + gap),
        right.w - 24, rh}, ru(session) ? "КРОСС-МОД" : "CROSS MOD",
        signed_percent(mod.rate_mod_amount),
        (mod.rate_mod_amount + 1.0F) * 0.5F,
        SliderKind::mod_cross, 0, 0, scale, kPurple);
}

void a_actor(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, SDL_Rect area, int scale) {
    const int gap = 12;
    const int side_w = std::max(310, area.w / 5);
    SDL_Rect side{area.x, area.y, side_w, area.h};
    SDL_Rect main{area.x + side_w + gap, area.y,
        area.w - side_w - gap, area.h};
    a_actor_sidebar(renderer, session, state, telemetry, side, scale);
    a_panel(renderer, main);
    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    const SDL_Color color = aActor[static_cast<std::size_t>(state.actor)];
    const int header_h = std::max(112, main.h / 7);
    text(renderer, main.x + 20, main.y + 20,
        std::to_string(state.actor + 1), color, scale + 1);
    text(renderer, main.x + 20 + 4 * 8 * (scale + 1), main.y + 20,
        engine_name(slot.engine, ru(session)), kInk, scale + 1);

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
        const float density = cd::effective_event_density(slot.event_density,
            session.performance.events);
        const std::string rate = decimal(cd::event_rate_hz(session.tempo_bpm,
            density, slot.motion), "/S");
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
        a_button(renderer, state, {main.x + main.w - 126,
            main.y + 15, 108, 54}, ru(session) ? "ЗАПУСК" : "TRIGGER",
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
    for (int i = 0; i < 3; ++i)
        a_button(renderer, state, {main.x + 12 + i * tab_w,
            tabs_y, tab_w - 8, 58}, labels[static_cast<std::size_t>(i)],
            Action::actor_section, static_cast<int>(sections[static_cast<std::size_t>(i)]),
            0, scale, state.actor_section == sections[static_cast<std::size_t>(i)],
            kPurple);
    SDL_Rect body{main.x + 12, tabs_y + 70, main.w - 24,
        main.y + main.h - tabs_y - 82};
    switch (state.actor_section) {
    case ActorSection::sound: a_actor_sound(renderer, session, state, body, scale); break;
    case ActorSection::events: a_actor_events(renderer, session, state, body, scale); break;
    case ActorSection::modulation: a_actor_mod(renderer, session, state, body, scale); break;
    }
}

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
}

void a_actor_events(SDL_Renderer* renderer, cd::Session& session,
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
}

void a_actor_mod(SDL_Renderer* renderer, cd::Session& session,
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
}

void a_actor(SDL_Renderer* renderer, cd::Session& session, UiState& state,
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
}

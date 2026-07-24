// Exact Android Actor screen. The shared header, typography and actor cards
// come from the approved Place design system.

std::string x_actor_midi_note(int midi) {
    static constexpr std::array<std::string_view, 12> names{{
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}};
    midi = std::clamp(midi, 0, 127);
    return std::string{names[static_cast<std::size_t>(midi % 12)]} +
        std::to_string(midi / 12 - 1);
}


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

void x_actor_sidebar_exact(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, const cd::AudioTelemetry& telemetry, SDL_Rect area,
    int scale) {
    a_card(renderer, area, aPanel2, aBorderSoft, 9, false);
    a_text(renderer, area.x + 14, area.y + 11,
        ru(session) ? "АКТЁРЫ" : "ACTORS", aCream, scale + 1);
    a_text(renderer, area.x + area.w - 14 - a_text_width("4/4", scale),
        area.y + 13, "4/4", aMutedCream, scale);
    const int gap = 8;
    const int top = area.y + 42;
    const int card_h = (area.h - 52 - 3 * gap) / 4;
    for (int i = 0; i < 4; ++i) {
        x_actor_sidebar_card_exact(renderer, session, state, telemetry,
            {area.x + 9, top + i * (card_h + gap), area.w - 18, card_h},
            i, scale, state.actor == i);
    }
}

void x_actor_scale_preview(SDL_Renderer* renderer, SDL_Rect rect,
    int root, SDL_Color accent) {
    constexpr int count = 16;
    const int gap = 8;
    const int bar_w = std::max(5, (rect.w - (count - 1) * gap) / count);
    for (int i = 0; i < count; ++i) {
        const bool root_bar = i == 0 || i == 8;
        const int height = root_bar ? rect.h : rect.h - 8;
        const int x = rect.x + i * (bar_w + gap);
        const int y = rect.y + rect.h - height;
        a_round_fill(renderer, {x, y, bar_w, height}, 2,
            root_bar ? accent : a_mix(aPanel2, accent, 0.34F));
        if (root_bar) {
            a_card(renderer, {x - 2, y - 2, bar_w + 4, height + 4},
                a_alpha(aTop, 0), aGreen, 3, false);
        }
    }
    const std::string root_text = x_actor_midi_note(root);
    a_text(renderer, rect.x, rect.y - 20, root_text, accent, 1);
}

void x_actor_sound_exact(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect area, int scale) {
    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    const int gap = 10;
    const int left_w = static_cast<int>(area.w * 0.64F);
    SDL_Rect left{area.x, area.y, left_w - gap, area.h};
    SDL_Rect right{area.x + left_w, area.y, area.w - left_w, area.h};
    a_card(renderer, left, aTop, aBorderSoft, 8, false);
    a_card(renderer, right, aTop, aBorderSoft, 8, false);

    const std::array<std::string, 7> labels{{
        ru(session) ? "ЧАСТОТА" : "FREQUENCY",
        ru(session) ? "ТЕМБР" : "TIMBRE",
        ru(session) ? "ЦВЕТ" : "COLOR",
        ru(session) ? "ДВИЖЕНИЕ" : "MOTION",
        ru(session) ? "ТЕКСТУРА" : "TEXTURE",
        ru(session) ? "УРОВЕНЬ" : "LEVEL",
        ru(session) ? "PAN" : "PAN"}};
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
        decimal(slot.frequency_hz, "HZ"), decimal(slot.timbre, ""),
        decimal(slot.color, ""), decimal(slot.motion, ""),
        decimal(slot.texture, ""), decimal(20.0F * std::log10(std::max(slot.level, 0.0001F)), "DB"),
        signed_percent(slot.pan)}};
    const std::array<std::string, 7> low_ru{{
        "20 ГЦ", "МЯГКО", "ТЁМНЫЙ", "СТАТИЧНО", "ГЛАДКАЯ", "MUTE", "ЛЕВО"}};
    const std::array<std::string, 7> high_ru{{
        "2 КГЦ", "ЯРКО", "СВЕТЛЫЙ", "ДВИЖЕТСЯ", "ЗЕРНИСТАЯ", "0 ДБ", "ПРАВО"}};
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
            ru(session) ? low_ru[static_cast<std::size_t>(i)] : "LOW",
            ru(session) ? high_ru[static_cast<std::size_t>(i)] : "HIGH");
    }

    const int pad = 12;
    const int tuning_h = static_cast<int>(right.h * 0.58F);
    SDL_Rect tuning{right.x + pad, right.y + pad, right.w - 2 * pad, tuning_h - pad};
    SDL_Rect root{right.x + pad, right.y + tuning_h + 2,
        right.w - 2 * pad, right.h - tuning_h - pad - 2};
    a_text(renderer, tuning.x, tuning.y,
        ru(session) ? "СТРОЙ" : "TUNING", aCream, scale + 1);
    SDL_Rect scale_pick{tuning.x, tuning.y + 28, tuning.w * 3 / 5, 43};
    x_button(renderer, state, scale_pick, std::string{slot.tuning.name.data()},
        Action::actor_section, 99, 0, scale + 1, false, aPurple);
    SDL_Rect editor{scale_pick.x + scale_pick.w + 8, scale_pick.y,
        tuning.x + tuning.w - (scale_pick.x + scale_pick.w + 8), 43};
    x_button(renderer, state, editor,
        ru(session) ? "РЕДАКТОР СТРОЯ" : "TUNING EDITOR",
        Action::actor_section, 99, 0, scale, false, aPurple);
    x_actor_scale_preview(renderer,
        {tuning.x + 8, tuning.y + 105, tuning.w - 16, 31},
        slot.tuning.root_midi, aActor[static_cast<std::size_t>(state.actor)]);

    a_text(renderer, root.x, root.y,
        ru(session) ? "КОРЕНЬ" : "ROOT", aCream, scale + 1);
    const int controls_y = root.y + 28;
    SDL_Rect previous{root.x, controls_y, 48, 45};
    SDL_Rect next{root.x + root.w - 48, controls_y, 48, 45};
    SDL_Rect note{previous.x + previous.w + 7, controls_y,
        root.w - previous.w - next.w - 14, 45};
    x_button(renderer, state, previous, "<", Action::actor_root_step,
        -1, 0, scale + 1, false, aPurple);
    a_card(renderer, note, aPanel2, aBorderSoft, 7, false);
    a_centered_text(renderer, note,
        x_actor_midi_note(slot.tuning.root_midi) + "   " +
            std::to_string(slot.tuning.root_midi),
        aGreen, scale + 2);
    add_hit(state, note, Action::slider, 0, 0, SliderKind::tuning_root);
    x_button(renderer, state, next, ">", Action::actor_root_step,
        1, 0, scale + 1, false, aPurple);
    SDL_Rect snap{root.x, controls_y + 54, root.w, 40};
    x_button(renderer, state, snap,
        ru(session) ? "ПРИВЯЗКА К ЛАДУ" : "SNAP TO SCALE",
        Action::actor_section, 99, 0, scale, false, aPurple);
}


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
    // Actor identity and all actionable controls share one horizontal axis.
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
            scale + 1, state.actor_section == sections[static_cast<std::size_t>(i)],
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

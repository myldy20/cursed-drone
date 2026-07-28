// Exact Android Memory screen. Uses only the persistence and settings that
// exist in v0.13: eight local session slots, autosave, locale and fade times.

void x_memory_slot_exact(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect rect, int index, int scale) {
    const bool saved = state.memory_present[static_cast<std::size_t>(index)];
    const bool selected = state.memory_slot == index;
    const SDL_Color color = aActor[static_cast<std::size_t>(index % 4)];
    const SDL_Color border = selected ? aPurple :
        (saved ? a_mix(aBorderSoft, color, 0.55F) : aBorderSoft);
    const SDL_Color surface = selected ? a_mix(aPanel2, aPurple, 0.13F) : aTop;
    a_card(renderer, rect, surface, border, 8, selected);

    a_text(renderer, rect.x + 13, rect.y + 11,
        ru(session) ? "СЛОТ" : "SLOT", aMutedCream, scale);
    a_text(renderer, rect.x + 13, rect.y + 31,
        std::to_string(index + 1), selected ? aCream : color, scale + 3);

    SDL_Rect status{rect.x + rect.w - 116, rect.y + 11, 102, 29};
    a_card(renderer, status,
        saved ? a_mix(aPanel2, color, 0.12F) : aPanel2,
        saved ? color : aBorderSoft, 6, false);
    a_centered_text(renderer, status,
        saved ? (ru(session) ? "СОХРАНЁН" : "SAVED") :
            (ru(session) ? "ПУСТО" : "EMPTY"),
        saved ? color : aMutedCream, scale);

    const int line_y = rect.y + rect.h - 39;
    a_hline(renderer, rect.x + 13, line_y, rect.w - 26,
        selected ? aPurple : aBorderSoft);
    a_text(renderer, rect.x + 13, line_y + 11,
        selected ? (ru(session) ? "ВЫБРАН" : "SELECTED") :
            (saved ? (ru(session) ? "ЛОКАЛЬНАЯ СЕССИЯ" : "LOCAL SESSION") :
                (ru(session) ? "ГОТОВ К СОХРАНЕНИЮ" : "READY TO SAVE")),
        selected ? aPurple : aMutedCream, scale);

    add_hit(state, rect, Action::memory_select, index);
}

void x_memory_setting_button(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect rect, std::string label, std::string value,
    Action action, SDL_Color color, int scale) {
    a_card(renderer, rect, aTop, aBorderSoft, 7, false);
    a_text(renderer, rect.x + 13, rect.y + 10,
        std::move(label), aMutedCream, scale);
    a_text(renderer, rect.x + 13, rect.y + 31,
        std::move(value), color, scale + 1);
    SDL_Point arrow[4]{{rect.x + rect.w - 29, rect.y + 24},
        {rect.x + rect.w - 19, rect.y + 24},
        {rect.x + rect.w - 24, rect.y + 33},
        {rect.x + rect.w - 29, rect.y + 24}};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLines(renderer, arrow, 4);
    add_hit(state, rect, action);
}

void a_memory_exact(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, SDL_Rect area, int scale) {
    area.x -= 1;
    area.w += 2;
    area.h += 7;
    const int gap = 10;
    const int left_w = static_cast<int>(area.w * 0.67F);
    SDL_Rect left{area.x, area.y, left_w - gap, area.h};
    SDL_Rect right{area.x + left_w, area.y, area.w - left_w, area.h};
    a_card(renderer, left, aPanel2, aBorderSoft, 9, false);
    a_card(renderer, right, aPanel2, aBorderSoft, 9, false);

    a_text(renderer, left.x + 16, left.y + 12,
        ru(session) ? "ПАМЯТЬ" : "MEMORY", aCream, scale + 2);
    const std::string count = ru(session) ? "8 ЛОКАЛЬНЫХ СЛОТОВ" :
        "8 LOCAL SLOTS";
    a_text(renderer, left.x + left.w - 16 - a_text_width(count, scale),
        left.y + 17, count, aMutedCream, scale);

    const int grid_x = left.x + 13;
    const int grid_y = left.y + 49;
    const int slot_gap = 8;
    const int slot_w = (left.w - 26 - 3 * slot_gap) / 4;
    const int slot_h = 111;
    for (int i = 0; i < 8; ++i) {
        x_memory_slot_exact(renderer, session, state,
            {grid_x + (i % 4) * (slot_w + slot_gap),
                grid_y + (i / 4) * (slot_h + slot_gap), slot_w, slot_h},
            i, scale);
    }

    const int actions_y = grid_y + 2 * slot_h + slot_gap + 10;
    const int action_h = 45;
    const int action_gap = 8;
    const int action_w = (left.w - 26 - 2 * action_gap) / 3;
    const bool load_enabled =
        state.memory_present[static_cast<std::size_t>(state.memory_slot)];
    SDL_Rect load{left.x + 13, actions_y, action_w, action_h};
    if (load_enabled) {
        x_button(renderer, state, load,
            ru(session) ? "ЗАГРУЗИТЬ" : "LOAD",
            Action::memory_load, 0, 0, scale + 1, true, aPurple);
    } else {
        a_card(renderer, load, aTop, aBorderSoft, 7, false);
        a_centered_text(renderer, load,
            ru(session) ? "СЛОТ ПУСТ" : "EMPTY SLOT",
            aMutedCream, scale + 1);
    }
    x_button(renderer, state,
        {load.x + action_w + action_gap, actions_y, action_w, action_h},
        ru(session) ? "СОХРАНИТЬ" : "SAVE",
        Action::memory_save, 0, 0, scale + 1, true, aGreen);
    x_button(renderer, state,
        {load.x + 2 * (action_w + action_gap), actions_y,
            action_w, action_h},
        ru(session) ? "ВОССТАНОВИТЬ МЕСТО" : "RESTORE PLACE",
        Action::landscape_reset, 0, 0, scale + 1, true, aOrange);

    SDL_Rect autosave{left.x + 13, actions_y + action_h + 10,
        left.w - 26, left.y + left.h - (actions_y + action_h + 10) - 13};
    a_card(renderer, autosave, aTop, aBorderSoft, 8, false);
    SDL_Rect indicator{autosave.x + 14, autosave.y + 15, 13, 13};
    a_round_fill(renderer, indicator, 6, aGreen);
    a_text(renderer, autosave.x + 38, autosave.y + 10,
        ru(session) ? "АВТОСОХРАНЕНИЕ ВКЛЮЧЕНО" : "AUTOSAVE ENABLED",
        aGreen, scale + 1);
    a_text(renderer, autosave.x + 14, autosave.y + 39,
        ru(session) ?
            "ИЗМЕНЕНИЯ СОХРАНЯЮТСЯ АВТОМАТИЧЕСКИ ПОСЛЕ РЕДАКТИРОВАНИЯ" :
            "CHANGES ARE SAVED AUTOMATICALLY AFTER EDITING",
        aMutedCream, scale);
    a_text(renderer, autosave.x + 14, autosave.y + 59,
        ru(session) ?
            "ПОСЛЕДНЕЕ СОСТОЯНИЕ ВОССТАНАВЛИВАЕТСЯ ПРИ ЗАПУСКЕ" :
            "THE LAST STATE IS RESTORED ON STARTUP",
        aMutedCream, scale);

    a_text(renderer, right.x + 15, right.y + 12,
        ru(session) ? "НАСТРОЙКИ" : "SETTINGS", aCream, scale + 2);
    const int settings_x = right.x + 12;
    const int settings_w = right.w - 24;
    const int settings_y = right.y + 49;
    const int setting_gap = 8;
    const int language_h = 64;
    x_memory_setting_button(renderer, session, state,
        {settings_x, settings_y, settings_w, language_h},
        ru(session) ? "ЯЗЫК" : "LANGUAGE",
        ru(session) ? "РУССКИЙ" : "ENGLISH",
        Action::locale_toggle, aPurple, scale);

    const int fade_h = 72;
    x_slider(renderer, state,
        {settings_x, settings_y + language_h + setting_gap,
            settings_w, fade_h},
        ru(session) ? "ФЕЙД ВХОДА" : "FADE IN",
        decimal(session.fade_in_seconds, "S"),
        cd::mapping::normalized_fade_seconds(session.fade_in_seconds),
        SliderKind::fade_in, 0, 0, scale + 1, aGreen,
        "0.25 S", "30 S");
    x_slider(renderer, state,
        {settings_x, settings_y + language_h + setting_gap + fade_h + setting_gap,
            settings_w, fade_h},
        ru(session) ? "ФЕЙД ВЫХОДА" : "FADE OUT",
        decimal(session.fade_out_seconds, "S"),
        cd::mapping::normalized_fade_seconds(session.fade_out_seconds),
        SliderKind::fade_out, 0, 0, scale + 1, aOrange,
        "0.25 S", "30 S");

    const int about_y = settings_y + language_h + setting_gap +
        2 * (fade_h + setting_gap);
    SDL_Rect about{settings_x, about_y, settings_w,
        right.y + right.h - about_y - 12};
    a_card(renderer, about, aTop, aBorderSoft, 8, false);
    a_text(renderer, about.x + 14, about.y + 11,
        ru(session) ? "О ПРИЛОЖЕНИИ" : "ABOUT", aMutedCream, scale);
    a_text(renderer, about.x + 14, about.y + 34,
        "CURSED DRONE", aCream, scale + 2);
    a_text(renderer, about.x + 14, about.y + 62,
        std::string{"V"} + CURSED_DRONE_VERSION + "  •  ANDROID",
        aPurple, scale + 1);
    a_text(renderer, about.x + 14, about.y + 89,
        "MYLDY DESIGN  @MYLDY20", aMutedCream, scale);
    a_text(renderer, about.x + 14, about.y + about.h - 25,
        ru(session) ? "СЕССИИ ХРАНЯТСЯ В ПАМЯТИ ПРИЛОЖЕНИЯ" :
            "SESSIONS ARE STORED IN APP STORAGE",
        aMutedCream, scale);
}

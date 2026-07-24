void a_fx_card(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    SDL_Rect rect, int index, int scale, bool master) {
    auto& effect = master ? session.master_effects[static_cast<std::size_t>(index)] :
        session.slots[static_cast<std::size_t>(state.actor)]
            .effects[static_cast<std::size_t>(index)];
    const bool selected = master ? state.master_fx == index : state.actor_fx == index;
    const SDL_Color color = aActor[static_cast<std::size_t>(index)];
    a_panel(renderer, rect, selected ? aPanel2 : aPanel,
        selected ? color : kBorder);
    const int select_w = rect.w < 1'000 ? std::max(150, rect.w / 5) : std::max(210, rect.w / 5);
    SDL_Rect select{rect.x + 10, rect.y + 10, select_w, rect.h - 20};
    a_panel(renderer, select, selected ? aActive : aPanel2,
        selected ? color : kBorder);
    text(renderer, select.x + 10, select.y + 9,
        (master ? "MASTER FX " : "FX ") + std::to_string(index + 1),
        color, scale);
    a_button(renderer, state, {select.x + 8, select.y + select.h / 2,
        select.w - 16, select.h / 2 - 8}, effect_name(effect.kind, ru(session)),
        selected ? (master ? Action::master_fx_picker : Action::actor_fx_picker)
                 : (master ? Action::master_fx_select : Action::actor_fx_select),
        selected ? 0 : index, 0, std::max(1, scale - 1), true, color);
    add_hit(state, {select.x, select.y, select.w, select.h / 2},
        master ? Action::master_fx_select : Action::actor_fx_select, index);

    const int count = effect_parameter_count(effect.kind);
    const int px = select.x + select.w + 16;
    const int pg = 18;
    const int pw = (rect.x + rect.w - px - 14 - 2 * pg) / 3;
    const std::array<std::string, 3> labels{{
        ru(session) ? "СМЕСЬ" : "MIX",
        ru(session) ? "ХАРАКТЕР" : "TONE",
        ru(session) ? "ОБРАТНО" : "FEEDBACK"}};
    const std::array<float, 3> values{{effect.amount, effect.tone,
        effect.feedback}};
    const std::array<SliderKind, 3> actor_kinds{{
        SliderKind::actor_fx_amount, SliderKind::actor_fx_tone,
        SliderKind::actor_fx_feedback}};
    const std::array<SliderKind, 3> master_kinds{{
        SliderKind::master_fx_amount, SliderKind::master_fx_tone,
        SliderKind::master_fx_feedback}};
    for (int i = 0; i < 3; ++i)
        a_mini_slider(renderer, state, {px + i * (pw + pg),
            rect.y + 22, pw, rect.h - 44}, labels[static_cast<std::size_t>(i)],
            percent(values[static_cast<std::size_t>(i)]),
            values[static_cast<std::size_t>(i)],
            master ? master_kinds[static_cast<std::size_t>(i)] :
                actor_kinds[static_cast<std::size_t>(i)],
            scale, color, selected && i < count);
}

void a_fx(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, SDL_Rect area, int scale) {
    a_fx_exact(renderer, session, state, telemetry, area, scale);
}

void a_master(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    SDL_Rect area, int scale) {
    a_master_exact(renderer, session, state, area, scale);
}

void a_memory(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    SDL_Rect area, int scale) {
    const int gap = 12;
    const int left_w = static_cast<int>(area.w * 0.66F);
    SDL_Rect left{area.x, area.y, left_w - gap, area.h};
    SDL_Rect right{area.x + left_w, area.y, area.w - left_w, area.h};
    a_panel(renderer, left); a_panel(renderer, right);
    text(renderer, left.x + 16, left.y + 14,
        ru(session) ? "ПАМЯТЬ - 8 СЛОТОВ" : "MEMORY - 8 SLOTS",
        kInk, scale);
    const int top = left.y + 49;
    const int sg = 9;
    const int slot_w = (left.w - 26 - 3 * sg) / 4;
    const int slot_h = std::max(116, left.h / 4);
    for (int i = 0; i < 8; ++i) {
        SDL_Rect slot{left.x + 13 + (i % 4) * (slot_w + sg),
            top + (i / 4) * (slot_h + sg), slot_w, slot_h};
        const bool saved = state.memory_present[static_cast<std::size_t>(i)];
        a_panel(renderer, slot, state.memory_slot == i ? aPanel2 : aPanel,
            state.memory_slot == i ? kPurple : kBorder);
        centered_text(renderer, {slot.x, slot.y + 10, slot.w, 30},
            (ru(session) ? "СЛОТ " : "SLOT ") + std::to_string(i + 1),
            kInk, scale);
        centered_text(renderer, {slot.x, slot.y + slot.h / 2, slot.w, 28},
            saved ? (ru(session) ? "СОХРАНЕН" : "SAVED") :
                (ru(session) ? "ПУСТ" : "EMPTY"),
            saved ? aActor[static_cast<std::size_t>(i % 4)] : kDim,
            std::max(1, scale - 1));
        add_hit(state, slot, Action::memory_select, i);
    }
    const int ay = top + 2 * (slot_h + sg) + 6;
    const int ah = std::max(70, left.h / 9);
    const int aw = (left.w - 26 - 2 * gap) / 3;
    a_button(renderer, state, {left.x + 13, ay, aw, ah},
        ru(session) ? "ЗАГРУЗИТЬ" : "LOAD", Action::memory_load,
        0, 0, scale, true, kPurple);
    a_button(renderer, state, {left.x + 13 + aw + gap, ay, aw, ah},
        ru(session) ? "СОХРАНИТЬ" : "SAVE", Action::memory_save,
        0, 0, scale, true, kGreen);
    a_button(renderer, state, {left.x + 13 + 2 * (aw + gap), ay, aw, ah},
        ru(session) ? "ВОССТАНОВИТЬ МЕСТО" : "RESTORE PLACE",
        Action::landscape_reset, 0, 0, scale, true, kActorColors[1]);
    SDL_Rect autosave{left.x + 13, ay + ah + gap, left.w - 26,
        left.y + left.h - ay - ah - 2 * gap};
    a_panel(renderer, autosave, aPanel2, kBorder);
    text(renderer, autosave.x + 16, autosave.y + 16,
        ru(session) ? "АВТОСОХРАНЕНИЕ ВКЛЮЧЕНО" : "AUTOSAVE ENABLED",
        kGreen, scale);
    text(renderer, autosave.x + 16, autosave.y + 16 + 12 * scale,
        ru(session) ? "ПОСЛЕДНЕЕ СОСТОЯНИЕ ВОССТАНАВЛИВАЕТСЯ ПРИ ЗАПУСКЕ" :
            "LAST STATE RESUMES ON START", kDim, std::max(1, scale - 1));

    text(renderer, right.x + 16, right.y + 14,
        ru(session) ? "НАСТРОЙКИ" : "SETTINGS", kInk, scale);
    const int bh = std::max(78, right.h / 9);
    a_button(renderer, state, {right.x + 12, right.y + 50,
        right.w - 24, bh}, ru(session) ? "ЯЗЫК: РУССКИЙ" :
            "LANGUAGE: ENGLISH", Action::locale_toggle, 0, 0, scale,
        true, kPurple);
    a_slider(renderer, state, {right.x + 12, right.y + 50 + bh + gap,
        right.w - 24, bh}, ru(session) ? "ФЕЙД ВХОДА" : "FADE IN",
        decimal(session.fade_in_seconds, "S"),
        cd::mapping::normalized_fade_seconds(session.fade_in_seconds),
        SliderKind::fade_in, 0, 0, scale, kGreen);
    a_slider(renderer, state, {right.x + 12,
        right.y + 50 + 2 * (bh + gap), right.w - 24, bh},
        ru(session) ? "ФЕЙД ВЫХОДА" : "FADE OUT",
        decimal(session.fade_out_seconds, "S"),
        cd::mapping::normalized_fade_seconds(session.fade_out_seconds),
        SliderKind::fade_out, 0, 0, scale, kActorColors[1]);
    SDL_Rect about{right.x + 12, right.y + 50 + 3 * (bh + gap),
        right.w - 24, right.y + right.h -
            (right.y + 50 + 3 * (bh + gap)) - 12};
    a_panel(renderer, about, aPanel2, kBorder);
    text(renderer, about.x + 16, about.y + 16,
        ru(session) ? "О ПРИЛОЖЕНИИ" : "ABOUT", kDim,
        std::max(1, scale - 1));
    text(renderer, about.x + 16, about.y + 49,
        "CURSED DRONE", kInk, scale + 1);
    text(renderer, about.x + 16, about.y + 49 + 13 * scale,
        std::string{"V"} + CURSED_DRONE_VERSION, kPurple, scale);
    text(renderer, about.x + 16, about.y + 49 + 25 * scale,
        "MYLDY DESIGN  @MYLDY20", kDim, std::max(1, scale - 1));
}

void approved_draw(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, const cd::AudioTelemetry& telemetry, float cpu,
    int width, int height) {
    state.hits.clear();
    fill(renderer, {0, 0, width, height}, kBackground);
    const int scale = font_scale_for(height);
    int content_y = 0;
    a_header(renderer, session, state, telemetry, cpu, width, height,
        scale, content_y);
    const int outer = a_compact_layout(width, height) ? 16 : 28;
    SDL_Rect content{outer, content_y, width - 2 * outer,
        height - content_y - outer};
    switch (state.page) {
    case Page::place: a_place(renderer, session, state, telemetry, content, scale); break;
    case Page::actor: a_actor(renderer, session, state, telemetry, content, scale); break;
    case Page::fx: a_fx(renderer, session, state, telemetry, content, scale); break;
    case Page::master: a_master(renderer, session, state, content, scale); break;
    case Page::memory: a_memory(renderer, session, state, content, scale); break;
    }
    if (state.picker != PickerKind::none)
        draw_picker(renderer, session, state, width, height, scale);
    if (!state.toast.empty() && SDL_GetTicks() < state.toast_until) {
        SDL_Rect toast{width / 4, height - 100, width / 2, 70};
        a_panel(renderer, toast, aTop, kGreen);
        centered_text(renderer, toast, state.toast, kInk, scale);
    }
    SDL_RenderPresent(renderer);
}

bool approved_press(cd::Session& session, UiState& state, HitTarget hit,
    int x) {
    if (hit.action == Action::slider && hit.slider == SliderKind::actor_level &&
        hit.a >= 0 && hit.a < 4) state.actor = hit.a;
    return handle_press(session, state, hit, x);
}

bool approved_release(cd::Session& session, UiState& state, int x, int y) {
    state.slider_active = false;
    if (state.pressed.action == Action::none ||
        !contains(state.pressed.rect, x, y)) {
        state.pressed = {};
        return false;
    }
    const HitTarget hit = state.pressed;
    state.pressed = {};
    if (hit.action == Action::actor_section && hit.a == kActionPanic) {
        g_panic_requested = true;
        return false;
    }
    if (hit.action == Action::actor_section && hit.a == kSourceLandscape) {
        restore_landscape_actor(session, state.actor);
        return true;
    }
    if (hit.action == Action::actor_section && hit.a == kSourceMusical) {
        auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
        slot.engine = cd::EngineKind::plaits;
        slot.enabled = true;
        session.scene_modified = true;
        return true;
    }
    if (hit.action == Action::actor_section && hit.a >= 95) {
        open_special_picker(state, hit.a);
        return false;
    }
    return execute_action(session, state, hit);
}

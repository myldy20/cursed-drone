#include "approved_ui_memory_exact.inc"

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
    a_memory_exact(renderer, session, state, area, scale);
}

void approved_draw(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, const cd::AudioTelemetry& telemetry, float cpu,
    int width, int height) {
    state.hits.clear();
    fill(renderer, {0, 0, width, height}, kBackground);
    // Pixel 8 Pro landscape cutout: reserve the same space on both sides so
    // rotation never moves controls underneath the camera.
    g_ui_safe_side = width >= 1'200 && width > height ? 48 : 0;
    const int scale = font_scale_for(height);
    int content_y = 0;
    a_header(renderer, session, state, telemetry, cpu, width, height,
        scale, content_y);
    const int outer = a_compact_layout(width, height) ? 16 : 28;
    const int side = g_ui_safe_side + outer;
    SDL_Rect content{side, content_y, width - 2 * side,
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

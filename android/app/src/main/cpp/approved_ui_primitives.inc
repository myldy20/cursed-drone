constexpr SDL_Color aTop{13, 11, 18, 255};
constexpr SDL_Color aPanel{25, 21, 33, 255};
constexpr SDL_Color aPanel2{31, 26, 41, 255};
constexpr SDL_Color aRaised{39, 32, 51, 255};
constexpr SDL_Color aActive{73, 46, 104, 255};
constexpr SDL_Color aTrack{54, 46, 68, 255};
constexpr SDL_Color aYellow{215, 181, 61, 255};
constexpr std::array<SDL_Color, 4> aActor{{kGreen, kActorColors[1], kRed, kActorColors[3]}};
bool a_compact_layout(int width, int height) noexcept {
    return width < 1'600 || height < 850;
}

void a_panel(SDL_Renderer* renderer, SDL_Rect rect, SDL_Color color = aPanel,
    SDL_Color border = kBorder) {
    fill(renderer, rect, color);
    outline(renderer, rect, border);
}

void a_button(SDL_Renderer* renderer, UiState& state, SDL_Rect rect,
    std::string label, Action action, int a, int b, int scale,
    bool active = false, SDL_Color accent = kPurple) {
    a_panel(renderer, rect, active ? aActive : aPanel2, active ? accent : kBorder);
    centered_text(renderer, rect, std::move(label), active ? kInk : kDim, scale);
    add_hit(state, rect, action, a, b);
}

void a_meter(SDL_Renderer* renderer, SDL_Rect rect, float value, SDL_Color color) {
    value = std::clamp(value, 0.0F, 1.0F);
    fill(renderer, rect, aTrack);
    fill(renderer, {rect.x, rect.y,
        static_cast<int>(std::lround(static_cast<float>(rect.w) * value)), rect.h}, color);
}

void a_slider(SDL_Renderer* renderer, UiState& state, SDL_Rect rect,
    std::string label, std::string value, float normalized, SliderKind kind,
    int a, int b, int scale, SDL_Color color,
    std::string low = {}, std::string high = {}) {
    a_panel(renderer, rect, aPanel2, kBorder);
    const int pad = std::max(12, rect.w / 80);
    text(renderer, rect.x + pad, rect.y + 10, label, kInk, scale);
    text(renderer, rect.x + rect.w - pad - cd::ui::text_width(value, scale),
        rect.y + 10, value, color, scale);
    const int bar_h = std::max(12, rect.h / 8);
    SDL_Rect bar{rect.x + pad, rect.y + rect.h / 2,
        rect.w - 2 * pad, bar_h};
    a_meter(renderer, bar, normalized, color);
    const int knob = bar.x + static_cast<int>(std::lround(
        static_cast<float>(bar.w) * std::clamp(normalized, 0.0F, 1.0F)));
    fill(renderer, {knob - bar_h / 2, bar.y - bar_h / 2,
        bar_h, bar_h * 2}, kInk);
    if (!low.empty()) text(renderer, rect.x + pad,
        rect.y + rect.h - 8 * std::max(1, scale - 1) - 6,
        low, kDim, std::max(1, scale - 1));
    if (!high.empty()) text(renderer,
        rect.x + rect.w - pad - cd::ui::text_width(high, std::max(1, scale - 1)),
        rect.y + rect.h - 8 * std::max(1, scale - 1) - 6,
        high, kDim, std::max(1, scale - 1));
    add_hit(state, rect, Action::slider, a, b, kind);
}

void a_mini_slider(SDL_Renderer* renderer, UiState& state, SDL_Rect rect,
    std::string label, std::string value, float normalized, SliderKind kind,
    int scale, SDL_Color color, bool enabled) {
    text(renderer, rect.x, rect.y, label, enabled ? kInk : kDim,
        std::max(1, scale - 1));
    text(renderer, rect.x + rect.w - cd::ui::text_width(value,
        std::max(1, scale - 1)), rect.y, value, enabled ? color : kDim,
        std::max(1, scale - 1));
    SDL_Rect bar{rect.x, rect.y + 10 * scale, rect.w,
        std::max(10, rect.h - 10 * scale)};
    a_meter(renderer, bar, normalized, enabled ? color : aTrack);
    if (enabled) add_hit(state, {rect.x - 5, rect.y - 4,
        rect.w + 10, rect.h + 8}, Action::slider, 0, 0, kind);
}

void a_header(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, float cpu, int width, int height,
    int scale, int& content_y) {
    const bool compact = a_compact_layout(width, height);
    const int brand_h = compact ? 64 : std::max(78, height / 12);
    const int nav_h = compact ? 54 : std::max(66, height / 15);
    const int margin = compact ? 18 : 30;
    fill(renderer, {0, 0, width, brand_h + nav_h + 8}, aTop);
    const int brand_x = compact ? 20 : 34;
    text(renderer, brand_x, compact ? 16 : 20, "CURSED DRONE", kInk,
        scale + 1);
    text(renderer, brand_x + cd::ui::text_width("CURSED DRONE", scale + 1) +
        (compact ? 14 : 26), compact ? 23 : 30,
        std::string{"V"} + CURSED_DRONE_VERSION, kDim,
        std::max(1, scale - 1));

    const int status_w = compact ? std::min(450, width / 2) :
        std::max(460, width / 4);
    const int status_x = width - status_w - margin;
    const int gap = compact ? 8 : 12;
    const int fade_w = compact ? std::max(150, status_w * 38 / 100) :
        std::max(190, status_w * 2 / 5);
    const int kill_w = compact ? 90 : 116;
    SDL_Rect fade{status_x, compact ? 10 : 14, fade_w,
        brand_h - (compact ? 20 : 28)};
    a_panel(renderer, fade, state.fade_active ? aActive : aPanel2, kPurple);
    text(renderer, fade.x + (compact ? 10 : 18), fade.y + 12,
        ru(session) ? "ФЕЙД" : "FADE", kInk, scale);
    const std::string fade_value = percent(session.performance.fade);
    text(renderer, fade.x + fade.w - (compact ? 10 : 18) -
        cd::ui::text_width(fade_value, scale), fade.y + 12,
        fade_value, kGreen, scale);
    add_hit(state, fade, Action::fade);

    SDL_Rect kill{fade.x + fade.w + gap, fade.y, kill_w, fade.h};
    a_button(renderer, state, kill, "KILL", Action::actor_section,
        kActionPanic, 0, scale, true, kRed);

    const int mx = kill.x + kill.w + gap;
    text(renderer, mx, compact ? 12 : 18, "DSP", kDim,
        std::max(1, scale - 1));
    const std::string cpu_text = std::to_string(
        static_cast<int>(std::lround(cpu * 100.0F))) + "%";
    text(renderer, width - margin - cd::ui::text_width(cpu_text,
        std::max(1, scale - 1)), compact ? 12 : 18, cpu_text, kDim,
        std::max(1, scale - 1));
    constexpr int segments = 12;
    const int meter_gap = compact ? 3 : 5;
    const int meter_w = std::max(segments,
        width - mx - margin);
    const int seg_w = std::max(1,
        (meter_w - meter_gap * (segments - 1)) / segments);
    const float out = std::clamp(telemetry.master_rms * 4.0F, 0.0F, 1.0F);
    for (int i = 0; i < segments; ++i) {
        const float t = static_cast<float>(i + 1) / segments;
        const SDL_Color color = t <= out ? (t > 0.82F ? kRed :
            (t > 0.65F ? kActorColors[1] : kGreen)) : aTrack;
        fill(renderer, {mx + i * (seg_w + meter_gap),
            compact ? 42 : 49, seg_w, compact ? 13 : 17}, color);
    }

    constexpr std::array<Page, 5> pages{{Page::place, Page::actor,
        Page::fx, Page::master, Page::memory}};
    const int left = compact ? 18 : 30;
    const int tab_gap = compact ? 8 : 12;
    const int tab_w = (width - 2 * left - 4 * tab_gap) / 5;
    for (int i = 0; i < 5; ++i) {
        SDL_Rect tab{left + i * (tab_w + tab_gap), brand_h, tab_w, nav_h};
        const bool active = pages[static_cast<std::size_t>(i)] == state.page;
        a_panel(renderer, tab, active ? aActive : aPanel,
            active ? kInk : kBorder);
        centered_text(renderer, tab,
            page_name(pages[static_cast<std::size_t>(i)], ru(session)),
            active ? kInk : kDim, scale);
        add_hit(state, tab, Action::page,
            static_cast<int>(pages[static_cast<std::size_t>(i)]));
    }
    content_y = brand_h + nav_h + 14;
}

void a_actor_card(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, const cd::AudioTelemetry& telemetry, SDL_Rect rect,
    int actor, int scale, bool selected, bool open_actor) {
    const auto& slot = session.slots[static_cast<std::size_t>(actor)];
    const SDL_Color color = aActor[static_cast<std::size_t>(actor)];
    a_panel(renderer, rect, selected ? aPanel2 : aPanel,
        selected ? color : kBorder);
    text(renderer, rect.x + 14, rect.y + 12, std::to_string(actor + 1),
        color, scale);
    text(renderer, rect.x + 14 + 3 * 8 * scale, rect.y + 12,
        engine_name(slot.engine, ru(session)), slot.enabled ? kInk : kDim,
        scale);
    if (telemetry.slot_event[static_cast<std::size_t>(actor)] > 0.04F)
        fill(renderer, {rect.x + rect.w - 30, rect.y + 14, 16, 16}, color);

    const bool compact = rect.h < 132;
    const int mute_h = compact ? std::max(34, rect.h / 3) :
        std::max(40, rect.h / 5);
    SDL_Rect mute{rect.x + 12, rect.y + rect.h - mute_h - 10,
        rect.w - 24, mute_h};
    if (compact) {
        const int meter_y = rect.y + std::max(38, rect.h / 2 - 8);
        a_meter(renderer, {rect.x + 14, meter_y, rect.w - 28, 10},
            telemetry.slot_rms[static_cast<std::size_t>(actor)] * 4.2F,
            color);
        a_panel(renderer, mute, slot.enabled ? aPanel : kRed,
            slot.enabled ? color : kRed);
        centered_text(renderer, mute,
            slot.enabled ? (ru(session) ? "ЗАГЛУШИТЬ" : "MUTE") :
                (ru(session) ? "ВКЛЮЧИТЬ" : "UNMUTE"),
            slot.enabled ? kDim : kInk, std::max(1, scale - 1));
        add_hit(state, mute, Action::actor_toggle, actor);
        add_hit(state, {rect.x, rect.y, rect.w,
            std::max(1, mute.y - rect.y - 4)}, Action::actor_select,
            actor, open_actor ? 1 : 0);
        return;
    }

    const int level_y = rect.y + rect.h - mute_h - 58;
    text(renderer, rect.x + 14, level_y,
        ru(session) ? "УРОВЕНЬ" : "LEVEL", kDim,
        std::max(1, scale - 1));
    const std::string level = percent(slot.level);
    text(renderer, rect.x + rect.w - 14 - cd::ui::text_width(level,
        std::max(1, scale - 1)), level_y, level, color,
        std::max(1, scale - 1));
    SDL_Rect bar{rect.x + 14, level_y + 10 * scale, rect.w - 28, 12};
    a_meter(renderer, bar, slot.level, color);
    add_hit(state, {bar.x - 4, level_y - 4, bar.w + 8, 42},
        Action::slider, actor, 0, SliderKind::actor_level);

    a_panel(renderer, mute, slot.enabled ? aPanel : kRed,
        slot.enabled ? color : kRed);
    centered_text(renderer, mute,
        slot.enabled ? (ru(session) ? "ЗАГЛУШИТЬ" : "MUTE") :
            (ru(session) ? "ВКЛЮЧИТЬ" : "UNMUTE"),
        slot.enabled ? kDim : kInk, std::max(1, scale - 1));
    add_hit(state, mute, Action::actor_toggle, actor);
    add_hit(state, {rect.x, rect.y, rect.w,
        std::max(1, level_y - rect.y - 5)}, Action::actor_select,
        actor, open_actor ? 1 : 0);
}

void a_place(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, SDL_Rect area, int scale) {
    const int pad = std::max(18, area.h / 42);
    a_panel(renderer, area);
    SDL_Rect scene{area.x + pad, area.y + pad, area.w - 2 * pad,
        std::max(92, area.h / 9)};
    a_panel(renderer, scene, aPanel2, kPurple);
    text(renderer, scene.x + 22, scene.y + 12,
        ru(session) ? "ЛАНДШАФТ" : "LANDSCAPE", kDim,
        std::max(1, scale - 1));
    text(renderer, scene.x + 22, scene.y + 44,
        scene_name(session.scene, ru(session)) +
        (session.scene_modified ? " *" : ""), kInk, scale + 1);
    text(renderer, scene.x + scene.w - 22 - cd::ui::text_width(
        ru(session) ? "ВЫБРАТЬ" : "CHOOSE", scale), scene.y + 48,
        ru(session) ? "ВЫБРАТЬ" : "CHOOSE", kPurple, scale);
    add_hit(state, scene, Action::scene_picker);

    const int top = scene.y + scene.h + pad;
    const int body_h = area.y + area.h - top - pad;
    const int left_w = static_cast<int>(area.w * 0.60F);
    SDL_Rect macros{area.x + pad, top, left_w - 2 * pad, body_h};
    SDL_Rect actors{area.x + left_w, top, area.w - left_w - pad, body_h};
    a_panel(renderer, macros, aPanel2); a_panel(renderer, actors, aPanel2);
    text(renderer, macros.x + 16, macros.y + 14,
        ru(session) ? "ХАРАКТЕР МЕСТА" : "PLACE CHARACTER", kInk, scale);
    text(renderer, actors.x + 16, actors.y + 14,
        ru(session) ? "АКТЕРЫ ЛАНДШАФТА" : "LANDSCAPE ACTORS", kInk, scale);

    const std::array<float, 5> values{{session.performance.texture,
        session.performance.pulse, session.performance.chaos,
        session.performance.space, session.performance.events}};
    const std::array<SliderKind, 5> kinds{{SliderKind::place_texture,
        SliderKind::place_pulse, SliderKind::place_chaos,
        SliderKind::place_space, SliderKind::place_events}};
    const std::array<SDL_Color, 5> colors{{kGreen, kActorColors[1],
        kRed, kActorColors[3], kPurple}};
    const std::array<std::string, 5> low{{"ГЛАДКО", "РЕДКО",
        "СПОКОЙНО", "БЛИЗКО", "СТАТИЧНО"}};
    const std::array<std::string, 5> high{{"ГРУБО", "ПЛОТНО",
        "НЕСТАБИЛЬНО", "ДАЛЕКО", "МЕНЯЕТСЯ"}};
    const int row_gap = 9;
    const int start = macros.y + 48;
    const int row_h = (macros.y + macros.h - start - 12 - 4 * row_gap) / 5;
    for (int i = 0; i < 5; ++i)
        a_slider(renderer, state,
            {macros.x + 10, start + i * (row_h + row_gap),
                macros.w - 20, row_h},
            macro_name(i, ru(session)), percent(values[static_cast<std::size_t>(i)]),
            values[static_cast<std::size_t>(i)], kinds[static_cast<std::size_t>(i)],
            0, 0, scale, colors[static_cast<std::size_t>(i)],
            ru(session) ? low[static_cast<std::size_t>(i)] : "LOW",
            ru(session) ? high[static_cast<std::size_t>(i)] : "HIGH");

    const int card_gap = 10;
    const int card_w = (actors.w - 22 - card_gap) / 2;
    const int card_h = (actors.h - 58 - card_gap) / 2;
    for (int i = 0; i < 4; ++i)
        a_actor_card(renderer, session, state, telemetry,
            {actors.x + 11 + (i % 2) * (card_w + card_gap),
                actors.y + 47 + (i / 2) * (card_h + card_gap),
                card_w, card_h}, i, scale, state.actor == i, true);
}

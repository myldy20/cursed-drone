constexpr SDL_Color aTop{10, 8, 15, 255};
constexpr SDL_Color aBackgroundDeep{7, 6, 11, 255};
constexpr SDL_Color aPanel{20, 17, 28, 255};
constexpr SDL_Color aPanel2{27, 22, 37, 255};
constexpr SDL_Color aRaised{36, 29, 49, 255};
constexpr SDL_Color aActive{69, 43, 99, 255};
constexpr SDL_Color aTrack{51, 43, 63, 255};
constexpr SDL_Color aBorderSoft{73, 62, 84, 170};
constexpr SDL_Color aYellow{224, 183, 70, 255};
constexpr SDL_Color aCream{242, 229, 202, 255};
constexpr SDL_Color aMutedCream{167, 156, 169, 255};
constexpr std::array<SDL_Color, 4> aActor{{
    kGreen, kActorColors[1], kRed, kActorColors[3]}};

bool a_compact_layout(int width, int height) noexcept {
    return width < 1'600 || height < 850;
}

SDL_Color a_alpha(SDL_Color color, Uint8 alpha) noexcept {
    color.a = alpha;
    return color;
}

SDL_Color a_mix(SDL_Color left, SDL_Color right, float amount) noexcept {
    amount = std::clamp(amount, 0.0F, 1.0F);
    const auto channel = [amount](Uint8 a, Uint8 b) {
        return static_cast<Uint8>(std::lround(
            static_cast<float>(a) +
            (static_cast<float>(b) - static_cast<float>(a)) * amount));
    };
    return {channel(left.r, right.r), channel(left.g, right.g),
        channel(left.b, right.b), channel(left.a, right.a)};
}

void a_round_fill(SDL_Renderer* renderer, SDL_Rect rect, int radius,
    SDL_Color color) {
    if (rect.w <= 0 || rect.h <= 0) return;
    radius = std::clamp(radius, 0, std::min(rect.w, rect.h) / 2);
    if (radius <= 1) {
        fill(renderer, rect, color);
        return;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect middle{rect.x, rect.y + radius, rect.w,
        std::max(0, rect.h - 2 * radius)};
    SDL_RenderFillRect(renderer, &middle);
    SDL_Rect centre{rect.x + radius, rect.y,
        std::max(0, rect.w - 2 * radius), rect.h};
    SDL_RenderFillRect(renderer, &centre);
    const float rr = static_cast<float>(radius * radius);
    for (int row = 0; row < radius; ++row) {
        const float dy = static_cast<float>(radius - row);
        const int inset = radius - static_cast<int>(std::floor(
            std::sqrt(std::max(0.0F, rr - dy * dy))));
        SDL_Rect top{rect.x + inset, rect.y + row,
            std::max(0, rect.w - 2 * inset), 1};
        SDL_Rect bottom{rect.x + inset, rect.y + rect.h - row - 1,
            std::max(0, rect.w - 2 * inset), 1};
        SDL_RenderFillRect(renderer, &top);
        SDL_RenderFillRect(renderer, &bottom);
    }
}

void a_glow(SDL_Renderer* renderer, SDL_Rect rect, int radius,
    SDL_Color color, bool strong = false) {
    const int layers = strong ? 4 : 3;
    for (int layer = layers; layer >= 1; --layer) {
        const int spread = layer * (strong ? 4 : 3);
        SDL_Rect glow{rect.x - spread, rect.y - spread,
            rect.w + 2 * spread, rect.h + 2 * spread};
        a_round_fill(renderer, glow, radius + spread,
            a_alpha(color, static_cast<Uint8>(strong ? 15 : 9)));
    }
}

void a_card(SDL_Renderer* renderer, SDL_Rect rect, SDL_Color color,
    SDL_Color border, int radius = 14, bool elevated = false) {
    if (elevated) {
        a_round_fill(renderer, {rect.x + 3, rect.y + 7, rect.w, rect.h},
            radius, {0, 0, 0, 80});
    }
    a_round_fill(renderer, rect, radius, border);
    SDL_Rect inner{rect.x + 1, rect.y + 1,
        std::max(0, rect.w - 2), std::max(0, rect.h - 2)};
    a_round_fill(renderer, inner, std::max(1, radius - 1), color);
}

void a_panel(SDL_Renderer* renderer, SDL_Rect rect, SDL_Color color = aPanel,
    SDL_Color border = aBorderSoft) {
    a_card(renderer, rect, color, border, 14, false);
}

void a_button(SDL_Renderer* renderer, UiState& state, SDL_Rect rect,
    std::string label, Action action, int a, int b, int scale,
    bool active = false, SDL_Color accent = kPurple) {
    const int radius = std::min(16, rect.h / 2);
    if (active) a_glow(renderer, rect, radius, accent);
    const SDL_Color surface = active ? a_mix(aRaised, accent, 0.28F) : aPanel2;
    const SDL_Color border = active ? a_alpha(accent, 230) : aBorderSoft;
    a_card(renderer, rect, surface, border, radius, active);
    centered_text(renderer, rect, std::move(label),
        active ? aCream : aMutedCream, scale);
    add_hit(state, rect, action, a, b);
}

void a_meter(SDL_Renderer* renderer, SDL_Rect rect, float value,
    SDL_Color color) {
    value = std::clamp(value, 0.0F, 1.0F);
    const int radius = std::max(2, rect.h / 2);
    a_round_fill(renderer, rect, radius, aTrack);
    const int fill_width = static_cast<int>(std::lround(
        static_cast<float>(rect.w) * value));
    if (fill_width > 0) {
        SDL_Rect active{rect.x, rect.y, fill_width, rect.h};
        a_glow(renderer, active, radius, color);
        a_round_fill(renderer, active, radius, color);
        if (rect.h >= 7) {
            a_round_fill(renderer,
                {active.x + 1, active.y + 1,
                    std::max(0, active.w - 2), std::max(1, active.h / 3)},
                std::max(1, radius - 1), a_alpha(aCream, 38));
        }
    }
}

void a_slider(SDL_Renderer* renderer, UiState& state, SDL_Rect rect,
    std::string label, std::string value, float normalized, SliderKind kind,
    int a, int b, int scale, SDL_Color color,
    std::string low = {}, std::string high = {}) {
    normalized = std::clamp(normalized, 0.0F, 1.0F);
    const int radius = 13;
    a_card(renderer, rect, aPanel2, aBorderSoft, radius, false);
    const int pad = std::max(13, rect.w / 70);
    const int label_scale = std::max(1, scale - 1);
    text(renderer, rect.x + pad, rect.y + 10, label, aCream, scale);
    text(renderer, rect.x + rect.w - pad - cd::ui::text_width(value, scale),
        rect.y + 10, value, color, scale);
    const int bar_h = std::clamp(rect.h / 10, 7, 12);
    SDL_Rect bar{rect.x + pad, rect.y + rect.h / 2 + 2,
        rect.w - 2 * pad, bar_h};
    a_meter(renderer, bar, normalized, color);
    const int knob_size = std::max(14, bar_h + 6);
    const int knob_x = bar.x + static_cast<int>(std::lround(
        static_cast<float>(bar.w) * normalized));
    SDL_Rect knob{knob_x - knob_size / 2,
        bar.y + bar.h / 2 - knob_size / 2, knob_size, knob_size};
    a_glow(renderer, knob, knob_size / 2, color, true);
    a_round_fill(renderer, knob, knob_size / 2, aCream);
    a_round_fill(renderer,
        {knob.x + 3, knob.y + 3, knob.w - 6, knob.h - 6},
        std::max(1, knob_size / 2 - 3), color);
    if (!low.empty()) {
        text(renderer, rect.x + pad,
            rect.y + rect.h - 8 * label_scale - 7,
            low, aMutedCream, label_scale);
    }
    if (!high.empty()) {
        text(renderer,
            rect.x + rect.w - pad - cd::ui::text_width(high, label_scale),
            rect.y + rect.h - 8 * label_scale - 7,
            high, aMutedCream, label_scale);
    }
    add_hit(state, rect, Action::slider, a, b, kind);
}

void a_mini_slider(SDL_Renderer* renderer, UiState& state, SDL_Rect rect,
    std::string label, std::string value, float normalized, SliderKind kind,
    int scale, SDL_Color color, bool enabled) {
    const int label_scale = std::max(1, scale - 1);
    text(renderer, rect.x, rect.y, label,
        enabled ? aCream : aMutedCream, label_scale);
    text(renderer, rect.x + rect.w - cd::ui::text_width(value, label_scale),
        rect.y, value, enabled ? color : aMutedCream, label_scale);
    SDL_Rect bar{rect.x, rect.y + 10 * scale, rect.w,
        std::clamp(rect.h - 10 * scale, 8, 14)};
    a_meter(renderer, bar, normalized, enabled ? color : aTrack);
    if (enabled) {
        add_hit(state, {rect.x - 5, rect.y - 4,
            rect.w + 10, rect.h + 8}, Action::slider, 0, 0, kind);
    }
}

void a_header(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, float cpu, int width, int height,
    int scale, int& content_y) {
    const bool compact = a_compact_layout(width, height);
    const int margin = compact ? 18 : 28;
    const int brand_h = compact ? 62 : 78;
    const int nav_h = compact ? 48 : 58;
    const int nav_y = brand_h + 4;
    for (int row = 0; row < brand_h + nav_h + 14; ++row) {
        const float t = static_cast<float>(row) /
            static_cast<float>(brand_h + nav_h + 14);
        fill(renderer, {0, row, width, 1},
            a_mix(aTop, aBackgroundDeep, t));
    }

    SDL_Rect brand{margin, 10, compact ? 360 : 430, brand_h - 18};
    a_card(renderer, brand, a_mix(aPanel, kPurple, 0.08F),
        a_alpha(kPurple, 105), 18, false);
    text(renderer, brand.x + 18, brand.y + 10, "CURSED DRONE",
        aCream, scale + 1);
    text(renderer, brand.x + 20, brand.y + brand.h - 17,
        "MYLDY DESIGN  /  AMBIENT INSTRUMENT", aMutedCream,
        std::max(1, scale - 1));

    const int right = width - margin;
    const int kill_w = compact ? 82 : 102;
    const int fade_w = compact ? 150 : 184;
    const int status_w = compact ? 210 : 270;
    const int gap = 10;
    SDL_Rect status{right - status_w, 10, status_w, brand_h - 18};
    SDL_Rect kill{status.x - gap - kill_w, 10, kill_w, brand_h - 18};
    SDL_Rect fade{kill.x - gap - fade_w, 10, fade_w, brand_h - 18};

    a_card(renderer, fade,
        state.fade_active ? a_mix(aRaised, kPurple, 0.34F) : aPanel,
        a_alpha(kPurple, 185), 18, state.fade_active);
    text(renderer, fade.x + 15, fade.y + 10,
        ru(session) ? "ФЕЙД" : "FADE", aCream, scale);
    const std::string fade_value = percent(session.performance.fade);
    text(renderer, fade.x + fade.w - 15 - cd::ui::text_width(fade_value, scale),
        fade.y + 10, fade_value, kGreen, scale);
    a_meter(renderer, {fade.x + 15, fade.y + fade.h - 14,
        fade.w - 30, 6}, session.performance.fade, kGreen);
    add_hit(state, fade, Action::fade);

    a_button(renderer, state, kill, "KILL", Action::actor_section,
        kActionPanic, 0, scale, true, kRed);

    a_card(renderer, status, aPanel, aBorderSoft, 18, false);
    text(renderer, status.x + 14, status.y + 9, "DSP",
        aMutedCream, std::max(1, scale - 1));
    const std::string cpu_text = std::to_string(
        static_cast<int>(std::lround(cpu * 100.0F))) + "%";
    text(renderer, status.x + status.w - 14 - cd::ui::text_width(
        cpu_text, std::max(1, scale - 1)), status.y + 9,
        cpu_text, aCream, std::max(1, scale - 1));
    a_meter(renderer, {status.x + 14, status.y + status.h - 16,
        status.w - 28, 7}, telemetry.master_rms * 4.0F, kGreen);

    constexpr std::array<Page, 5> pages{{Page::place, Page::actor,
        Page::fx, Page::master, Page::memory}};
    const int tab_gap = compact ? 9 : 12;
    const int tab_w = (width - 2 * margin - 4 * tab_gap) / 5;
    for (int i = 0; i < 5; ++i) {
        SDL_Rect tab{margin + i * (tab_w + tab_gap), nav_y,
            tab_w, nav_h};
        const bool active = pages[static_cast<std::size_t>(i)] == state.page;
        if (active) a_glow(renderer, tab, 17, kPurple);
        a_card(renderer, tab,
            active ? a_mix(aRaised, kPurple, 0.26F) : aPanel,
            active ? a_alpha(kPurple, 235) : aBorderSoft, 17, active);
        centered_text(renderer, tab,
            page_name(pages[static_cast<std::size_t>(i)], ru(session)),
            active ? aCream : aMutedCream, scale);
        add_hit(state, tab, Action::page,
            static_cast<int>(pages[static_cast<std::size_t>(i)]));
    }
    content_y = nav_y + nav_h + 14;
}

void a_actor_card(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, const cd::AudioTelemetry& telemetry, SDL_Rect rect,
    int actor, int scale, bool selected, bool open_actor) {
    const auto& slot = session.slots[static_cast<std::size_t>(actor)];
    const SDL_Color color = aActor[static_cast<std::size_t>(actor)];
    if (selected) a_glow(renderer, rect, 18, color);
    a_card(renderer, rect,
        selected ? a_mix(aPanel2, color, 0.10F) : aPanel,
        selected ? a_alpha(color, 225) : aBorderSoft, 18, selected);

    SDL_Rect badge{rect.x + 13, rect.y + 13, 38, 38};
    a_round_fill(renderer, badge, 12, a_mix(aRaised, color, 0.28F));
    centered_text(renderer, badge, std::to_string(actor + 1), color, scale);
    text(renderer, rect.x + 61, rect.y + 16,
        engine_name(slot.engine, ru(session)),
        slot.enabled ? aCream : aMutedCream, scale);
    text(renderer, rect.x + 62, rect.y + 16 + 11 * scale,
        slot.enabled ? "ACTIVE SIGNAL" : "MUTED",
        slot.enabled ? aMutedCream : kRed, std::max(1, scale - 1));
    if (telemetry.slot_event[static_cast<std::size_t>(actor)] > 0.04F) {
        SDL_Rect event{rect.x + rect.w - 31, rect.y + 16, 15, 15};
        a_glow(renderer, event, 8, color, true);
        a_round_fill(renderer, event, 8, color);
    }

    const bool compact = rect.h < 145;
    const int mute_h = compact ? 34 : 40;
    SDL_Rect mute{rect.x + 13, rect.y + rect.h - mute_h - 12,
        rect.w - 26, mute_h};
    const int meter_y = compact ? rect.y + rect.h / 2 :
        rect.y + rect.h - mute_h - 48;
    text(renderer, rect.x + 14, meter_y - 17,
        ru(session) ? "УРОВЕНЬ" : "LEVEL", aMutedCream,
        std::max(1, scale - 1));
    const std::string level = percent(slot.level);
    text(renderer, rect.x + rect.w - 14 - cd::ui::text_width(
        level, std::max(1, scale - 1)), meter_y - 17,
        level, color, std::max(1, scale - 1));
    SDL_Rect level_bar{rect.x + 14, meter_y, rect.w - 28, 9};
    a_meter(renderer, level_bar, slot.level, color);
    add_hit(state, {level_bar.x - 5, meter_y - 12,
        level_bar.w + 10, 34}, Action::slider, actor, 0,
        SliderKind::actor_level);

    a_button(renderer, state, mute,
        slot.enabled ? (ru(session) ? "ЗАГЛУШИТЬ" : "MUTE") :
            (ru(session) ? "ВКЛЮЧИТЬ" : "UNMUTE"),
        Action::actor_toggle, actor, 0, std::max(1, scale - 1),
        !slot.enabled, slot.enabled ? color : kRed);
    add_hit(state, {rect.x, rect.y, rect.w,
        std::max(1, meter_y - rect.y - 8)}, Action::actor_select,
        actor, open_actor ? 1 : 0);
}

void a_place(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, SDL_Rect area, int scale) {
    const int pad = 14;
    const int gap = 12;
    SDL_Rect scene{area.x, area.y, area.w, 88};
    a_glow(renderer, scene, 19, kPurple);
    a_card(renderer, scene, a_mix(aPanel2, kPurple, 0.10F),
        a_alpha(kPurple, 210), 19, true);
    text(renderer, scene.x + 20, scene.y + 14,
        ru(session) ? "АКУСТИЧЕСКОЕ МЕСТО" : "ACOUSTIC PLACE",
        aMutedCream, std::max(1, scale - 1));
    text(renderer, scene.x + 20, scene.y + 42,
        scene_name(session.scene, ru(session)) +
            (session.scene_modified ? "  MODIFIED" : ""),
        aCream, scale + 1);
    SDL_Rect choose{scene.x + scene.w - 192, scene.y + 16, 170, 56};
    a_button(renderer, state, choose,
        ru(session) ? "ВЫБРАТЬ" : "CHOOSE PLACE",
        Action::scene_picker, 0, 0, scale, true, kPurple);
    add_hit(state, scene, Action::scene_picker);

    const int top = scene.y + scene.h + gap;
    const int body_h = area.y + area.h - top;
    const int left_w = static_cast<int>(area.w * 0.595F);
    SDL_Rect macros{area.x, top, left_w - gap, body_h};
    SDL_Rect actors{area.x + left_w, top, area.w - left_w, body_h};
    a_card(renderer, macros, aPanel, aBorderSoft, 18, false);
    a_card(renderer, actors, aPanel, aBorderSoft, 18, false);
    text(renderer, macros.x + 18, macros.y + 15,
        ru(session) ? "ХАРАКТЕР МЕСТА" : "PLACE CHARACTER",
        aCream, scale);
    text(renderer, macros.x + 18, macros.y + 15 + 11 * scale,
        ru(session) ? "ПЯТЬ МАКРО-ПАРАМЕТРОВ ЖИВОГО ЗВУКА" :
            "FIVE LIVE PERFORMANCE MACROS",
        aMutedCream, std::max(1, scale - 1));
    text(renderer, actors.x + 18, actors.y + 15,
        ru(session) ? "АКТЕРЫ" : "LANDSCAPE ACTORS", aCream, scale);
    text(renderer, actors.x + 18, actors.y + 15 + 11 * scale,
        ru(session) ? "КАСАНИЕ ОТКРЫВАЕТ РЕДАКТОР" :
            "TAP A CARD TO OPEN ITS EDITOR",
        aMutedCream, std::max(1, scale - 1));

    const std::array<float, 5> values{{session.performance.texture,
        session.performance.pulse, session.performance.chaos,
        session.performance.space, session.performance.events}};
    const std::array<SliderKind, 5> kinds{{SliderKind::place_texture,
        SliderKind::place_pulse, SliderKind::place_chaos,
        SliderKind::place_space, SliderKind::place_events}};
    const std::array<SDL_Color, 5> colors{{kGreen, kActorColors[1],
        kRed, kActorColors[3], kPurple}};
    const std::array<std::string, 5> low_ru{{"ГЛАДКО", "РЕДКО",
        "СПОКОЙНО", "БЛИЗКО", "СТАТИЧНО"}};
    const std::array<std::string, 5> high_ru{{"ГРУБО", "ПЛОТНО",
        "НЕСТАБИЛЬНО", "ДАЛЕКО", "МЕНЯЕТСЯ"}};
    const int start = macros.y + 55;
    const int row_gap = 7;
    const int row_h = std::max(58,
        (macros.y + macros.h - start - pad - 4 * row_gap) / 5);
    for (int i = 0; i < 5; ++i) {
        a_slider(renderer, state,
            {macros.x + 12, start + i * (row_h + row_gap),
                macros.w - 24, row_h},
            macro_name(i, ru(session)),
            percent(values[static_cast<std::size_t>(i)]),
            values[static_cast<std::size_t>(i)],
            kinds[static_cast<std::size_t>(i)], 0, 0, scale,
            colors[static_cast<std::size_t>(i)],
            ru(session) ? low_ru[static_cast<std::size_t>(i)] : "LOW",
            ru(session) ? high_ru[static_cast<std::size_t>(i)] : "HIGH");
    }

    const int card_gap = 10;
    const int cards_top = actors.y + 55;
    const int card_w = (actors.w - 24 - card_gap) / 2;
    const int card_h = (actors.y + actors.h - cards_top - 12 - card_gap) / 2;
    for (int i = 0; i < 4; ++i) {
        a_actor_card(renderer, session, state, telemetry,
            {actors.x + 12 + (i % 2) * (card_w + card_gap),
                cards_top + (i / 2) * (card_h + card_gap),
                card_w, card_h}, i, scale, state.actor == i, true);
    }
}

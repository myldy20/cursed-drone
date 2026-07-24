#include "approved_ui_font.inc"
#define text a_text
#define centered_text a_centered_text
#define a_header a_header_legacy
#define a_actor_card a_actor_card_legacy
#define a_place a_place_legacy

constexpr SDL_Color aTop{8, 7, 12, 255};
constexpr SDL_Color aBackgroundDeep{8, 7, 12, 255};
constexpr SDL_Color aPanel{25, 21, 33, 255};
constexpr SDL_Color aPanel2{31, 25, 43, 255};
constexpr SDL_Color aRaised{39, 31, 52, 255};
constexpr SDL_Color aActive{119, 65, 177, 255};
constexpr SDL_Color aTrack{57, 48, 72, 255};
constexpr SDL_Color aBorderSoft{74, 59, 91, 255};
constexpr SDL_Color aYellow{225, 183, 66, 255};
constexpr SDL_Color aCream{239, 226, 198, 255};
constexpr SDL_Color aMutedCream{151, 143, 158, 255};
constexpr SDL_Color aGreen{87, 204, 174, 255};
constexpr SDL_Color aOrange{238, 164, 57, 255};
constexpr SDL_Color aRed{226, 82, 94, 255};
constexpr SDL_Color aBlue{94, 132, 205, 255};
constexpr SDL_Color aPurple{124, 67, 184, 255};
constexpr std::array<SDL_Color, 4> aActor{{aGreen, aOrange, aRed, aMutedCream}};

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
        return static_cast<Uint8>(std::lround(static_cast<float>(a) +
            (static_cast<float>(b) - static_cast<float>(a)) * amount));
    };
    return {channel(left.r, right.r), channel(left.g, right.g),
        channel(left.b, right.b), channel(left.a, right.a)};
}

void a_round_fill(SDL_Renderer* renderer, SDL_Rect rect, int radius,
    SDL_Color color) {
    if (rect.w <= 0 || rect.h <= 0) return;
    radius = std::clamp(radius, 0, std::min(rect.w, rect.h) / 2);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    if (radius <= 1) {
        SDL_RenderFillRect(renderer, &rect);
        return;
    }
    SDL_Rect middle{rect.x, rect.y + radius, rect.w,
        std::max(0, rect.h - 2 * radius)};
    SDL_Rect centre{rect.x + radius, rect.y,
        std::max(0, rect.w - 2 * radius), rect.h};
    SDL_RenderFillRect(renderer, &middle);
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

void a_card(SDL_Renderer* renderer, SDL_Rect rect, SDL_Color color,
    SDL_Color border, int radius = 8, bool elevated = false) {
    if (elevated) {
        a_round_fill(renderer, {rect.x + 2, rect.y + 3, rect.w, rect.h},
            radius, {0, 0, 0, 70});
    }
    a_round_fill(renderer, rect, radius, border);
    a_round_fill(renderer, {rect.x + 1, rect.y + 1,
        std::max(0, rect.w - 2), std::max(0, rect.h - 2)},
        std::max(1, radius - 1), color);
}

void a_panel(SDL_Renderer* renderer, SDL_Rect rect, SDL_Color color = aPanel,
    SDL_Color border = aBorderSoft) {
    a_card(renderer, rect, color, border, 8, false);
}

void a_hline(SDL_Renderer* renderer, int x, int y, int width,
    SDL_Color color = aBorderSoft) {
    fill(renderer, {x, y, std::max(0, width), 1}, a_alpha(color, 150));
}

void a_button(SDL_Renderer* renderer, UiState& state, SDL_Rect rect,
    std::string label, Action action, int a, int b, int scale,
    bool active = false, SDL_Color accent = kPurple) {
    const bool page = action == Action::page;
    const SDL_Color surface = page && active ? aActive :
        (active ? a_mix(aPanel2, accent, 0.12F) : aPanel2);
    const SDL_Color border = active ? accent : aBorderSoft;
    a_card(renderer, rect, surface, border, 7, false);
    centered_text(renderer, rect, std::move(label),
        active ? aCream : aMutedCream, scale);
    add_hit(state, rect, action, a, b);
}

void a_meter(SDL_Renderer* renderer, SDL_Rect rect, float value,
    SDL_Color color) {
    value = std::clamp(value, 0.0F, 1.0F);
    const int radius = std::max(2, rect.h / 2);
    a_round_fill(renderer, rect, radius, aTrack);
    const int active_width = static_cast<int>(std::lround(
        static_cast<float>(rect.w) * value));
    if (active_width > 0) {
        a_round_fill(renderer, {rect.x, rect.y, active_width, rect.h},
            radius, color);
    }
}

void a_slider_track(SDL_Renderer* renderer, SDL_Rect bar, float normalized,
    SDL_Color color) {
    normalized = std::clamp(normalized, 0.0F, 1.0F);
    a_meter(renderer, bar, normalized, color);
    const int knob = std::max(13, bar.h + 7);
    const int x = bar.x + static_cast<int>(std::lround(
        static_cast<float>(bar.w) * normalized));
    SDL_Rect handle{x - knob / 2, bar.y + bar.h / 2 - knob / 2,
        knob, knob};
    a_round_fill(renderer, handle, knob / 2, {4, 4, 7, 255});
    a_round_fill(renderer, {handle.x + 2, handle.y + 2,
        handle.w - 4, handle.h - 4}, std::max(1, knob / 2 - 2), aCream);
}

void a_slider(SDL_Renderer* renderer, UiState& state, SDL_Rect rect,
    std::string label, std::string value, float normalized, SliderKind kind,
    int a, int b, int scale, SDL_Color color,
    std::string low = {}, std::string high = {}) {
    normalized = std::clamp(normalized, 0.0F, 1.0F);
    const int label_scale = std::max(1, scale - 1);
    const int label_w = std::clamp(rect.w / 4, 120, 210);
    text(renderer, rect.x, rect.y + 9, label, aCream, scale);
    text(renderer, rect.x + label_w - cd::ui::text_width(value, scale),
        rect.y + 9, value, color, scale);
    SDL_Rect bar{rect.x + label_w + 25, rect.y + 16,
        rect.w - label_w - 33, 9};
    a_slider_track(renderer, bar, normalized, color);
    if (!low.empty()) {
        text(renderer, bar.x, rect.y + rect.h - 16,
            low, aMutedCream, label_scale);
    }
    if (!high.empty()) {
        text(renderer, bar.x + bar.w - cd::ui::text_width(high, label_scale),
            rect.y + rect.h - 16, high, aMutedCream, label_scale);
    }
    a_hline(renderer, rect.x, rect.y + rect.h - 1, rect.w);
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
    SDL_Rect bar{rect.x, rect.y + 18, rect.w, 8};
    a_slider_track(renderer, bar, normalized, enabled ? color : aMutedCream);
    if (enabled) {
        add_hit(state, {rect.x - 5, rect.y - 4,
            rect.w + 10, rect.h + 8}, Action::slider, 0, 0, kind);
    }
}

void a_header(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, float cpu, int width, int height,
    int scale, int& content_y) {
    (void)height;
    const int safe = std::clamp(g_ui_safe_side, 0, width / 4);
    const int usable_width = width - 2 * safe;
    fill(renderer, {0, 0, width, 118}, aTop);
    text(renderer, safe + 28, 18, "CURSED DRONE", aCream, scale + 1);
    text(renderer, safe + 230, 26,
        std::string{"V"} + CURSED_DRONE_VERSION + "  •  ANDROID CONCEPT",
        aMutedCream, std::max(1, scale - 1));

    SDL_Rect fade{safe + usable_width - 406, 13, 165, 35};
    a_card(renderer, fade, a_mix(aPanel2, kPurple, 0.15F),
        kPurple, 7, false);
    text(renderer, fade.x + 13, fade.y + 10,
        ru(session) ? "ФЕЙД" : "FADE", aCream, scale);
    const std::string fade_value = percent(session.performance.fade);
    text(renderer, fade.x + fade.w - 13 - cd::ui::text_width(fade_value, scale),
        fade.y + 10, fade_value, aGreen, scale);
    add_hit(state, fade, Action::fade);

    const int dsp_x = safe + usable_width - 215;
    text(renderer, dsp_x, 18, "DSP", aMutedCream,
        std::max(1, scale - 1));
    const std::string cpu_text = std::to_string(
        static_cast<int>(std::lround(cpu * 100.0F))) + "%";
    text(renderer, dsp_x + 32, 18, cpu_text, aMutedCream,
        std::max(1, scale - 1));
    constexpr int segments = 12;
    for (int i = 0; i < segments; ++i) {
        const float threshold = static_cast<float>(i + 1) /
            static_cast<float>(segments);
        const bool on = threshold <= std::clamp(
            telemetry.master_rms * 4.0F, 0.0F, 1.0F);
        const SDL_Color segment = on ? aGreen : aTrack;
        a_round_fill(renderer, {dsp_x + i * 16, 34, 13, 10}, 2, segment);
    }

    constexpr std::array<Page, 5> pages{{Page::place, Page::actor,
        Page::fx, Page::master, Page::memory}};
    const int margin = 27;
    const int gap = 7;
    const int nav_y = 66;
    const int nav_h = 42;
    const int tab_w = (usable_width - 2 * margin - 4 * gap) / 5;
    for (int i = 0; i < 5; ++i) {
        SDL_Rect tab{safe + margin + i * (tab_w + gap), nav_y, tab_w, nav_h};
        const bool active = pages[static_cast<std::size_t>(i)] == state.page;
        a_card(renderer, tab, active ? aActive : aPanel,
            active ? aCream : aBorderSoft, 7, false);
        centered_text(renderer, tab,
            page_name(pages[static_cast<std::size_t>(i)], ru(session)),
            active ? aCream : aMutedCream, scale);
        add_hit(state, tab, Action::page,
            static_cast<int>(pages[static_cast<std::size_t>(i)]));
    }
    content_y = 118;
}

void a_actor_card(SDL_Renderer* renderer, cd::Session& session,
    UiState& state, const cd::AudioTelemetry& telemetry, SDL_Rect rect,
    int actor, int scale, bool selected, bool open_actor) {
    const auto& slot = session.slots[static_cast<std::size_t>(actor)];
    const SDL_Color color = aActor[static_cast<std::size_t>(actor)];
    const SDL_Color border = selected ? color :
        (slot.enabled ? color : aBorderSoft);
    a_card(renderer, rect, aTop, border, 7, false);
    text(renderer, rect.x + 12, rect.y + 12,
        std::to_string(actor + 1), color, scale);
    text(renderer, rect.x + 35, rect.y + 12,
        engine_name(slot.engine, ru(session)),
        slot.enabled ? aCream : aMutedCream, scale);
    if (telemetry.slot_event[static_cast<std::size_t>(actor)] > 0.04F) {
        SDL_Rect event{rect.x + rect.w - 29, rect.y + 10, 18, 18};
        a_round_fill(renderer, event, 9, color);
        a_round_fill(renderer, {event.x + 3, event.y + 3,
            event.w - 6, event.h - 6}, 6, aTop);
    }

    const int signal_y = rect.y + 42;
    a_meter(renderer, {rect.x + 11, signal_y, rect.w - 22, 8},
        telemetry.slot_rms[static_cast<std::size_t>(actor)] * 4.2F, color);
    text(renderer, rect.x + 11, signal_y + 15,
        ru(session) ? "УРОВЕНЬ" : "LEVEL", aMutedCream,
        std::max(1, scale - 1));
    const std::string level = percent(slot.level);
    text(renderer, rect.x + rect.w - 11 - cd::ui::text_width(
        level, std::max(1, scale - 1)), signal_y + 15,
        level, color, std::max(1, scale - 1));
    const int level_y = signal_y + 36;
    SDL_Rect level_bar{rect.x + 11, level_y, rect.w - 22, 9};
    a_slider_track(renderer, level_bar, slot.level,
        slot.enabled ? color : aMutedCream);
    add_hit(state, {level_bar.x - 4, level_y - 10,
        level_bar.w + 8, 30}, Action::slider, actor, 0,
        SliderKind::actor_level);

    SDL_Rect mute{rect.x + 11, rect.y + rect.h - 33,
        rect.w - 22, 23};
    a_card(renderer, mute, slot.enabled ? aPanel : aRed,
        slot.enabled ? border : aRed, 5, false);
    centered_text(renderer, mute,
        slot.enabled ? (ru(session) ? "ЗАГЛУШИТЬ" : "MUTE") :
            (ru(session) ? "ВКЛЮЧИТЬ" : "UNMUTE"),
        slot.enabled ? aCream : aCream, std::max(1, scale - 1));
    add_hit(state, mute, Action::actor_toggle, actor);
    add_hit(state, {rect.x, rect.y, rect.w,
        std::max(1, level_y - rect.y - 8)}, Action::actor_select,
        actor, open_actor ? 1 : 0);
}

void a_place(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, SDL_Rect area, int scale) {
    a_card(renderer, area, aPanel2, aBorderSoft, 9, false);
    const int pad = 12;
    const int gap = 10;
    SDL_Rect scene{area.x + pad, area.y + pad, area.w - 2 * pad, 56};
    a_card(renderer, scene, a_mix(aPanel2, kPurple, 0.10F),
        a_mix(aBorderSoft, kPurple, 0.35F), 7, false);
    text(renderer, scene.x + 15, scene.y + 12,
        ru(session) ? "ЛАНДШАФТ" : "LANDSCAPE", aMutedCream,
        std::max(1, scale - 1));
    text(renderer, scene.x + 15, scene.y + 31,
        scene_name(session.scene, ru(session)) +
            (session.scene_modified ? " *" : ""), aCream, scale + 1);
    const std::string choose = ru(session) ?
        "НАЖАТЬ ДЛЯ ВЫБОРА" : "TAP TO CHOOSE";
    text(renderer, scene.x + scene.w - 40 - cd::ui::text_width(
        choose, std::max(1, scale - 1)), scene.y + 40,
        choose, aMutedCream, std::max(1, scale - 1));
    text(renderer, scene.x + scene.w - 22, scene.y + 22,
        "V", aCream, scale);
    add_hit(state, scene, Action::scene_picker);

    const int top = scene.y + scene.h + 12;
    const int body_h = area.y + area.h - top - pad;
    const int left_w = static_cast<int>(area.w * 0.62F);
    SDL_Rect macros{area.x + pad, top, left_w - pad - gap, body_h};
    SDL_Rect actors{area.x + left_w, top,
        area.x + area.w - pad - (area.x + left_w), body_h};
    a_card(renderer, macros, aTop, aBorderSoft, 7, false);
    a_card(renderer, actors, aTop, aBorderSoft, 7, false);
    text(renderer, macros.x + 15, macros.y + 13,
        ru(session) ? "ХАРАКТЕР МЕСТА" : "PLACE CHARACTER",
        aCream, scale);
    const std::string drag_hint = ru(session) ?
        "ТАП ИЛИ ПРОВЕДИТЕ ПАЛЬЦЕМ" : "TAP OR DRAG";
    text(renderer, macros.x + macros.w - 15 - cd::ui::text_width(
        drag_hint, std::max(1, scale - 1)), macros.y + 15,
        drag_hint, aMutedCream, std::max(1, scale - 1));
    text(renderer, actors.x + 15, actors.y + 13,
        ru(session) ? "АКТЕРЫ ЛАНДШАФТА" : "LANDSCAPE ACTORS",
        aCream, scale);
    text(renderer, actors.x + actors.w - 18, actors.y + 13,
        "4", aMutedCream, scale);

    const std::array<float, 5> values{{session.performance.texture,
        session.performance.pulse, session.performance.chaos,
        session.performance.space, session.performance.events}};
    const std::array<SliderKind, 5> kinds{{SliderKind::place_texture,
        SliderKind::place_pulse, SliderKind::place_chaos,
        SliderKind::place_space, SliderKind::place_events}};
    const std::array<SDL_Color, 5> colors{{aGreen, aOrange, aRed,
        aBlue, kPurple}};
    const std::array<std::string, 5> low_ru{{"ГЛАДКО", "РЕДКО",
        "СПОКОЙНО", "БЛИЗКО", "СТАТИЧНО"}};
    const std::array<std::string, 5> high_ru{{"ГРУБО", "ПЛОТНО",
        "НЕСТАБИЛЬНО", "ДАЛЕКО", "МЕНЯЕТСЯ"}};
    const int start = macros.y + 43;
    const int row_h = (macros.h - 54) / 5;
    for (int i = 0; i < 5; ++i) {
        a_slider(renderer, state,
            {macros.x + 15, start + i * row_h,
                macros.w - 30, row_h},
            macro_name(i, ru(session)),
            percent(values[static_cast<std::size_t>(i)]),
            values[static_cast<std::size_t>(i)],
            kinds[static_cast<std::size_t>(i)], 0, 0, scale,
            colors[static_cast<std::size_t>(i)],
            ru(session) ? low_ru[static_cast<std::size_t>(i)] : "LOW",
            ru(session) ? high_ru[static_cast<std::size_t>(i)] : "HIGH");
    }

    const int card_gap = 8;
    const int cards_top = actors.y + 43;
    const int card_w = (actors.w - 24 - card_gap) / 2;
    const int card_h = (actors.y + actors.h - cards_top - 11 - card_gap) / 2;
    for (int i = 0; i < 4; ++i) {
        a_actor_card(renderer, session, state, telemetry,
            {actors.x + 11 + (i % 2) * (card_w + card_gap),
                cards_top + (i / 2) * (card_h + card_gap),
                card_w, card_h}, i, scale, state.actor == i, true);
    }
}

#undef a_header
#undef a_actor_card
#undef a_place
#include "approved_ui_place_exact.inc"

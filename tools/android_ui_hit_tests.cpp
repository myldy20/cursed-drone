// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.

#include "cursed_drone/parameter_mapping.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <vector>

#define SDL_main cursed_drone_hit_test_unused_main
#include "../android/app/src/main/cpp/android_touch_main.cpp"
#undef SDL_main

namespace {
constexpr int kAndroidUiWidth = 1496;
constexpr int kAndroidUiHeight = 672;
constexpr std::string_view kPlatformLabel{"TEST"};
#include "../android/app/src/main/cpp/approved_ui_compat.inc"
#include "../android/app/src/main/cpp/approved_ui_primitives.inc"
#define a_actor a_actor_legacy
#include "../android/app/src/main/cpp/approved_ui_actor.inc"
#undef a_actor
#include "../android/app/src/main/cpp/approved_ui_actor_bundle.inc"
#include "../android/app/src/main/cpp/approved_ui_fx_exact.inc"
#include "../android/app/src/main/cpp/approved_ui_master_exact.inc"
#include "../android/app/src/main/cpp/approved_ui_fx_memory.inc"

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (condition) return;
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
}

bool same_rect(const SDL_Rect& left, const SDL_Rect& right) noexcept {
    return left.x == right.x && left.y == right.y &&
        left.w == right.w && left.h == right.h;
}

bool same_slider(const HitTarget& left, const HitTarget& right) noexcept {
    const HitTarget expected = slider_value_target(left);
    return left.action == Action::slider && right.action == Action::slider &&
        left.slider == right.slider && left.a == right.a && left.b == right.b &&
        same_rect(expected.rect, right.rect);
}

int intersection_area(const SDL_Rect& left, const SDL_Rect& right) noexcept {
    const int x1 = std::max(left.x, right.x);
    const int y1 = std::max(left.y, right.y);
    const int x2 = std::min(left.x + left.w, right.x + right.w);
    const int y2 = std::min(left.y + left.h, right.y + right.h);
    return std::max(0, x2 - x1) * std::max(0, y2 - y1);
}

void audit_hit_map(const UiState& state, std::string_view screen) {
    std::vector<const HitTarget*> sliders;
    for (const auto& hit : state.hits) {
        if (hit.action != Action::slider) continue;
        sliders.push_back(&hit);
        const HitTarget mapped = slider_value_target(hit);
        const int x = mapped.rect.x + mapped.rect.w / 2;
        const int y = mapped.rect.y + mapped.rect.h / 2;
        const HitTarget resolved = approved_hit_at(state, x, y);
        expect(same_slider(hit, resolved), std::string{screen} +
            ": slider centre is captured by another control");
    }
    for (std::size_t left = 0; left < sliders.size(); ++left) {
        for (std::size_t right = left + 1; right < sliders.size(); ++right) {
            expect(intersection_area(sliders[left]->rect,
                sliders[right]->rect) == 0, std::string{screen} +
                ": slider hit rectangles overlap");
        }
    }
}

void draw(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    Page page, ActorSection section = ActorSection::sound) {
    state.page = page;
    state.actor_section = section;
    state.picker = PickerKind::none;
    approved_draw(renderer, session, state, {}, 0.10F,
        kAndroidUiWidth, kAndroidUiHeight);
}

const HitTarget* find_slider(const UiState& state, SliderKind kind) {
    for (const auto& hit : state.hits) {
        if (hit.action == Action::slider && hit.slider == kind) return &hit;
    }
    return nullptr;
}

const HitTarget* detailed_actor_level(const UiState& state) {
    for (const auto& hit : state.hits) {
        if (hit.action == Action::slider &&
            hit.slider == SliderKind::actor_level &&
            !actor_card_level_hit(hit)) return &hit;
    }
    return nullptr;
}

const HitTarget* actor_card_level(const UiState& state, int actor) {
    for (const auto& hit : state.hits) {
        if (actor_card_level_hit(hit) && hit.a == actor) return &hit;
    }
    return nullptr;
}

const HitTarget* actor_toggle(const UiState& state, int actor) {
    for (const auto& hit : state.hits) {
        if (hit.action == Action::actor_toggle && hit.a == actor) return &hit;
    }
    return nullptr;
}

bool has_action(const UiState& state, Action action, int a) {
    for (const auto& hit : state.hits) {
        if (hit.action == action && hit.a == a) return true;
    }
    return false;
}

void press_slider(cd::Session& session, UiState& state,
    const HitTarget& hit, float normalized) {
    const HitTarget mapped = slider_value_target(hit);
    const int x = mapped.rect.x + static_cast<int>(std::lround(
        std::clamp(normalized, 0.0F, 1.0F) *
            static_cast<float>(mapped.rect.w)));
    static_cast<void>(approved_press(session, state, mapped, x));
    state.slider_active = false;
    state.pressed = {};
}

void test_slider_track_mapping(SDL_Renderer* renderer) {
    cd::Session session = cd::make_default_session();
    UiState state{};
    draw(renderer, session, state, Page::place);
    const HitTarget* texture = find_slider(state, SliderKind::place_texture);
    expect(texture != nullptr, "Place must expose the TEXTURE slider");
    if (texture == nullptr) return;

    const HitTarget mapped = slider_value_target(*texture);
    expect(mapped.rect.x > texture->rect.x + 180,
        "Wide slider value range must start at the drawn track");
    expect(mapped.rect.w < texture->rect.w - 180,
        "Wide slider value range must exclude the label column");
    press_slider(session, state, *texture, 0.73F);
    expect(std::abs(session.performance.texture - 0.73F) < 0.02F,
        "Slider value must follow the finger position on its track");
}

void test_plaits_surface(SDL_Renderer* renderer) {
    cd::Session session = cd::make_default_session();
    session.slots[0].engine = cd::EngineKind::plaits;
    UiState state{};
    state.actor = 0;
    draw(renderer, session, state, Page::actor, ActorSection::sound);

    expect(has_action(state, Action::actor_section, 97),
        "Plaits Sound must expose model selection");
    expect(has_action(state, Action::actor_section, 98),
        "Plaits Sound must expose MAIN/AUX output routing");
    expect(has_action(state, Action::actor_section, 99),
        "Compact quantiser must retain scale selection");
    expect(has_action(state, Action::tuning_toggle, 0),
        "Compact quantiser must retain its on/off switch");
    expect(has_action(state, Action::actor_root_step, -1) &&
        has_action(state, Action::actor_root_step, 1),
        "Compact quantiser must retain root controls");

    for (const SliderKind kind : {SliderKind::actor_timbre,
            SliderKind::actor_color, SliderKind::actor_motion,
            SliderKind::actor_texture}) {
        expect(find_slider(state, kind) != nullptr,
            "Plaits must retain all four sound-shaping parameters");
    }
    expect(find_slider(state, SliderKind::tuning_degrees) == nullptr,
        "Fine degree editing must not crowd the main Plaits surface");
    expect(find_slider(state, SliderKind::tuning_period) == nullptr,
        "Fine period editing must not crowd the main Plaits surface");
}

void test_actor_level_routing(SDL_Renderer* renderer) {
    cd::Session session = cd::make_default_session();
    UiState state{};
    for (int actor = 0; actor < 4; ++actor) {
        state.actor = actor;
        for (int slot = 0; slot < 4; ++slot) {
            session.slots[static_cast<std::size_t>(slot)].level =
                0.15F + static_cast<float>(slot) * 0.10F;
        }
        draw(renderer, session, state, Page::actor, ActorSection::sound);
        const HitTarget* detail = detailed_actor_level(state);
        expect(detail != nullptr,
            "Actor editor must expose one detailed LEVEL slider");
        if (detail == nullptr) continue;
        const auto before = session.slots;
        press_slider(session, state, *detail, 0.83F);
        expect(state.actor == actor,
            "Detailed LEVEL must preserve the selected actor");
        expect(session.slots[static_cast<std::size_t>(actor)].level > 0.78F,
            "Detailed LEVEL must edit the selected actor");
        for (int slot = 0; slot < 4; ++slot) {
            if (slot == actor) continue;
            expect(std::abs(session.slots[static_cast<std::size_t>(slot)].level -
                before[static_cast<std::size_t>(slot)].level) < 0.0001F,
                "Detailed LEVEL must not edit another actor");
        }
    }

    state.actor = 0;
    draw(renderer, session, state, Page::actor, ActorSection::sound);
    for (int actor = 0; actor < 4; ++actor) {
        const HitTarget* card = actor_card_level(state, actor);
        expect(card != nullptr,
            "Each actor card must expose an independent LEVEL slider");
        if (card == nullptr) continue;
        press_slider(session, state, *card, 0.37F + actor * 0.10F);
        expect(state.actor == actor,
            "Actor-card LEVEL must route to its own actor");
        draw(renderer, session, state, Page::actor, ActorSection::sound);
    }
}

void test_compact_button_touch_slop(SDL_Renderer* renderer) {
    cd::Session session = cd::make_default_session();
    UiState state{};
    for (const Page page : {Page::place, Page::actor}) {
        draw(renderer, session, state, page, ActorSection::sound);
        for (int actor = 0; actor < 4; ++actor) {
            const HitTarget* toggle = actor_toggle(state, actor);
            expect(toggle != nullptr,
                "Each actor card must expose MUTE/UNMUTE");
            if (toggle == nullptr) continue;
            const int x = toggle->rect.x + toggle->rect.w / 2;
            const int y = toggle->rect.y - 6;
            const HitTarget resolved = approved_hit_at(state, x, y);
            expect(resolved.action == Action::actor_toggle &&
                resolved.a == actor,
                "A small miss above MUTE must still route to that actor");
        }
    }

    draw(renderer, session, state, Page::actor, ActorSection::sound);
    for (int actor = 0; actor < 4; ++actor) {
        const HitTarget* level = actor_card_level(state, actor);
        expect(level != nullptr,
            "Each sidebar actor card must retain its LEVEL slider");
        if (level == nullptr) continue;
        const HitTarget mapped = slider_value_target(*level);
        const int x = mapped.rect.x + mapped.rect.w / 2;
        const int y = mapped.rect.y + mapped.rect.h / 2;
        const HitTarget resolved = approved_hit_at(state, x, y);
        expect(same_slider(*level, resolved),
            "MUTE touch slop must not steal the centre of LEVEL");
    }
}

void test_all_hit_maps(SDL_Renderer* renderer) {
    cd::Session session = cd::make_default_session();
    UiState state{};
    g_scales = cd::load_scala_scales({});

    draw(renderer, session, state, Page::place);
    audit_hit_map(state, "Place");

    for (int actor = 0; actor < 4; ++actor) {
        state.actor = actor;
        draw(renderer, session, state, Page::actor, ActorSection::sound);
        audit_hit_map(state, "Actor/Sound " + std::to_string(actor + 1));
        draw(renderer, session, state, Page::actor, ActorSection::events);
        audit_hit_map(state, "Actor/Events " + std::to_string(actor + 1));
        draw(renderer, session, state, Page::actor, ActorSection::modulation);
        audit_hit_map(state, "Actor/Modulation " + std::to_string(actor + 1));
        draw(renderer, session, state, Page::fx);
        audit_hit_map(state, "FX actor " + std::to_string(actor + 1));
    }

    draw(renderer, session, state, Page::master);
    audit_hit_map(state, "Master");
    draw(renderer, session, state, Page::memory);
    audit_hit_map(state, "Memory");
}

} // namespace

int main() {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init: " << SDL_GetError() << '\n';
        return 1;
    }
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(
        0, 2992, 1344, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer* renderer = surface != nullptr
        ? SDL_CreateSoftwareRenderer(surface) : nullptr;
    if (surface == nullptr || renderer == nullptr) {
        std::cerr << "SDL software renderer: " << SDL_GetError() << '\n';
        if (renderer != nullptr) SDL_DestroyRenderer(renderer);
        if (surface != nullptr) SDL_FreeSurface(surface);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderSetLogicalSize(renderer, kAndroidUiWidth, kAndroidUiHeight);

    test_slider_track_mapping(renderer);
    test_plaits_surface(renderer);
    test_actor_level_routing(renderer);
    test_compact_button_touch_slop(renderer);
    test_all_hit_maps(renderer);

    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
    SDL_Quit();
    if (failures != 0) {
        std::cerr << failures << " Android UI hit-target test(s) failed\n";
        return 1;
    }
    std::cout << "Android UI hit-target audit passed\n";
    return 0;
}

// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.

#include "cursed_drone/parameter_mapping.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <vector>

#define SDL_main cursed_drone_interaction_unused_main
#include "../android/app/src/main/cpp/android_touch_main.cpp"
#undef SDL_main

namespace {
constexpr int kAndroidUiWidth = 1496;
constexpr int kAndroidUiHeight = 672;
#include "../android/app/src/main/cpp/approved_ui_compat.inc"
#include "../android/app/src/main/cpp/approved_ui_primitives.inc"
#define a_actor a_actor_legacy
#include "../android/app/src/main/cpp/approved_ui_actor.inc"
#undef a_actor
#include "../android/app/src/main/cpp/approved_ui_actor_exact.inc"
#include "../android/app/src/main/cpp/approved_ui_fx_exact.inc"
#include "../android/app/src/main/cpp/approved_ui_master_exact.inc"
#include "../android/app/src/main/cpp/approved_ui_fx_memory.inc"
} // namespace

#include <algorithm>
#include <cstdlib>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

const HitTarget* find_action(const UiState& state, Action action) {
    for (auto it = state.hits.rbegin(); it != state.hits.rend(); ++it) {
        if (it->action == action) return &*it;
    }
    return nullptr;
}

const HitTarget* find_slider(const UiState& state, SliderKind kind) {
    for (auto it = state.hits.rbegin(); it != state.hits.rend(); ++it) {
        if (it->action == Action::slider && it->slider == kind) return &*it;
    }
    return nullptr;
}

void tap(cd::Session& session, UiState& state, const HitTarget& hit) {
    const int x = hit.rect.x + hit.rect.w / 2;
    const int y = hit.rect.y + hit.rect.h / 2;
    static_cast<void>(approved_press(session, state, hit, x));
    static_cast<void>(approved_release(session, state, x, y));
}

void set_slider(cd::Session& session, UiState& state,
    const HitTarget& hit, float normalized) {
    const int x = hit.rect.x + static_cast<int>(std::lround(
        normalized * static_cast<float>(hit.rect.w)));
    static_cast<void>(approved_press(session, state, hit, x));
    state.slider_active = false;
    state.pressed = {};
}

void draw(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    Page page, ActorSection section = ActorSection::sound) {
    state.page = page;
    state.actor_section = section;
    approved_draw(renderer, session, state, {}, 0.12F,
        kAndroidUiWidth, kAndroidUiHeight);
}

double render_difference(cd::Session left_session, cd::Session right_session) {
    cd::AudioGraph left;
    cd::AudioGraph right;
    left.prepare({48'000.0F, 256U}, left_session);
    right.prepare({48'000.0F, 256U}, right_session);
    std::array<cd::StereoFrame, 256> left_block{};
    std::array<cd::StereoFrame, 256> right_block{};
    double difference = 0.0;
    for (int block = 0; block < 180; ++block) {
        left.process({left_block.data(), left_block.size()});
        right.process({right_block.data(), right_block.size()});
        for (std::size_t sample = 0; sample < left_block.size(); ++sample) {
            difference += std::abs(static_cast<double>(
                left_block[sample].left - right_block[sample].left));
            difference += std::abs(static_cast<double>(
                left_block[sample].right - right_block[sample].right));
        }
    }
    return difference;
}

void test_audio_wiring() {
    cd::Session base = cd::make_default_session();
    base.scene_modified = true;
    base.performance.texture = 0.0F;
    base.performance.pulse = 0.0F;
    base.performance.chaos = 0.0F;
    base.performance.space = 0.0F;
    base.performance.events = 0.0F;
    for (std::size_t slot = 0; slot < cd::kSlotCount; ++slot) {
        base.slots[slot].enabled = slot == 0U;
        for (auto& effect : base.slots[slot].effects) effect = {};
        for (auto& mod : base.slots[slot].modulators) mod = {};
    }
    base.slots[0].engine = cd::EngineKind::diagnostic;
    base.slots[0].frequency_hz = 73.0F;
    base.slots[0].level = 0.55F;

    auto fx_off = base;
    auto fx_on = base;
    fx_off.slots[0].effects[0] = {
        cd::EffectKind::drive, 0.92F, 0.50F, 0.0F, false};
    fx_on.slots[0].effects[0] = {
        cd::EffectKind::drive, 0.92F, 0.50F, 0.0F, true};
    expect(render_difference(fx_off, fx_on) > 1.0,
        "effect enabled state must materially change DSP output");

    auto mod_off = base;
    auto mod_on = base;
    mod_off.slots[0].modulators[0] = {
        false, cd::ModWave::sine, cd::ModDestination::timbre,
        4.0F, 0.85F, 0.0F, -1, 0.0F};
    mod_on.slots[0].modulators[0] = mod_off.slots[0].modulators[0];
    mod_on.slots[0].modulators[0].enabled = true;
    expect(render_difference(mod_off, mod_on) > 1.0,
        "enabled modulation must materially change DSP output");
}

void test_ui_wiring(SDL_Renderer* renderer) {
    cd::Session session = cd::make_default_session();
    session.locale = cd::Locale::ru;
    UiState state{};
    state.actor = 0;
    state.actor_fx = 0;
    state.master_fx = 0;
    state.modulator = 0;

    draw(renderer, session, state, Page::place);
    expect(g_ui_safe_side == 48,
        "Pixel landscape UI should reserve a symmetric safe side inset");
    for (const auto& hit : state.hits) {
        expect(hit.rect.x >= g_ui_safe_side,
            "no Place control may enter the left cutout safe area");
        expect(hit.rect.x + hit.rect.w <= kAndroidUiWidth - g_ui_safe_side,
            "no Place control may enter the right cutout safe area");
    }

    draw(renderer, session, state, Page::actor, ActorSection::modulation);
    const HitTarget* mod_toggle = find_action(state, Action::mod_toggle);
    expect(mod_toggle != nullptr, "modulation enable button should have a hit target");
    if (mod_toggle != nullptr) {
        const bool before = session.slots[0].modulators[0].enabled;
        tap(session, state, *mod_toggle);
        expect(session.slots[0].modulators[0].enabled != before,
            "modulation enable button should mutate the selected modulator");
    }
    const HitTarget* depth = find_slider(state, SliderKind::mod_depth);
    expect(depth != nullptr, "modulation depth should have a hit target");
    if (depth != nullptr) {
        set_slider(session, state, *depth, 0.90F);
        expect(session.slots[0].modulators[0].depth > 0.70F,
            "modulation depth gesture should reach Session");
    }
    const HitTarget* rate = find_slider(state, SliderKind::mod_rate);
    expect(rate != nullptr, "modulation rate should have a hit target");
    if (rate != nullptr) {
        set_slider(session, state, *rate, 0.75F);
        expect(session.slots[0].modulators[0].rate_hz > 1.0F,
            "modulation rate gesture should reach Session");
    }

    session.slots[0].effects[0] = {
        cd::EffectKind::delay, 0.25F, 0.45F, 0.30F, true};
    draw(renderer, session, state, Page::fx);
    const HitTarget* actor_toggle = find_action(state, Action::actor_fx_toggle);
    expect(actor_toggle != nullptr, "Actor FX should expose a real enable toggle");
    if (actor_toggle != nullptr) {
        tap(session, state, *actor_toggle);
        expect(!session.slots[0].effects[0].enabled,
            "Actor FX toggle should bypass without losing effect type");
        expect(session.slots[0].effects[0].kind == cd::EffectKind::delay,
            "Actor FX toggle should preserve selected effect type");
    }
    const HitTarget* actor_mix = find_slider(state, SliderKind::actor_fx_amount);
    expect(actor_mix != nullptr, "Actor FX amount should have a hit target");
    if (actor_mix != nullptr) {
        set_slider(session, state, *actor_mix, 0.82F);
        expect(session.slots[0].effects[0].amount > 0.75F,
            "Actor FX amount gesture should reach Session while bypassed");
    }

    session.master_effects[0] = {
        cd::EffectKind::chorus, 0.30F, 0.50F, 0.20F, true};
    draw(renderer, session, state, Page::master);
    const HitTarget* master_toggle = find_action(state, Action::master_fx_toggle);
    expect(master_toggle != nullptr, "Master FX should expose a real enable toggle");
    if (master_toggle != nullptr) {
        tap(session, state, *master_toggle);
        expect(!session.master_effects[0].enabled,
            "Master FX toggle should reach Session");
    }
    const HitTarget* master_mix = find_slider(state, SliderKind::master_fx_amount);
    expect(master_mix != nullptr, "Master FX amount should have a hit target");
    if (master_mix != nullptr) {
        set_slider(session, state, *master_mix, 0.76F);
        expect(session.master_effects[0].amount > 0.70F,
            "Master FX amount gesture should reach Session");
    }

    state.picker = PickerKind::effect;
    state.picker_master = false;
    state.picker_effect = 0;
    draw(renderer, session, state, Page::fx);
    for (const auto& hit : state.hits) {
        expect(hit.rect.x >= g_ui_safe_side,
            "picker controls must respect the left cutout safe area");
        expect(hit.rect.x + hit.rect.w <= kAndroidUiWidth - g_ui_safe_side,
            "picker controls must respect the right cutout safe area");
    }
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

    test_ui_wiring(renderer);
    test_audio_wiring();

    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
    SDL_Quit();
    if (failures != 0) {
        std::cerr << failures << " Android UI interaction test(s) failed\n";
        return 1;
    }
    std::cout << "Android UI interaction and DSP wiring tests passed\n";
    return 0;
}

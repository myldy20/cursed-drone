// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.

#include "cursed_drone/parameter_mapping.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

#define SDL_main cursed_drone_snapshot_unused_main
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

#include <filesystem>
#include <iostream>
#include <string>

namespace {

constexpr int kPixel8ProLandscapeWidth = 2992;
constexpr int kPixel8ProLandscapeHeight = 1344;

const char* page_filename(Page page) noexcept {
    switch (page) {
    case Page::place: return "android-place.bmp";
    case Page::actor: return "android-actor.bmp";
    case Page::fx: return "android-fx.bmp";
    case Page::master: return "android-master.bmp";
    case Page::memory: return "android-memory.bmp";
    }
    return "android-unknown.bmp";
}

bool render_page(const std::filesystem::path& output_directory, Page page,
    ActorSection actor_section = ActorSection::sound,
    const char* filename_override = nullptr) {
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(
        0, kPixel8ProLandscapeWidth, kPixel8ProLandscapeHeight, 32,
        SDL_PIXELFORMAT_ARGB8888);
    if (surface == nullptr) {
        std::cerr << "SDL_CreateRGBSurfaceWithFormat: " << SDL_GetError() << '\n';
        return false;
    }
    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
    if (renderer == nullptr) {
        std::cerr << "SDL_CreateSoftwareRenderer: " << SDL_GetError() << '\n';
        SDL_FreeSurface(surface);
        return false;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderSetLogicalSize(renderer, kAndroidUiWidth, kAndroidUiHeight);

    cd::Session session = cd::make_default_session();
    session.locale = cd::Locale::ru;
    session.performance.texture = 0.22F;
    session.performance.pulse = 0.12F;
    session.performance.chaos = 0.08F;
    session.performance.space = 0.20F;
    session.performance.events = 0.30F;
    session.slots[0].level = 0.34F;
    session.slots[1].level = 0.42F;
    session.slots[2].level = 0.28F;
    session.slots[3].level = 0.37F;
    session.slots[3].enabled = false;

    UiState state{};
    state.page = page;
    state.actor_section = actor_section;
    state.actor = 1;
    state.actor_fx = 0;
    state.master_fx = 0;
    state.modulator = 0;
    for (std::size_t i = 0; i < state.memory_present.size(); ++i) {
        state.memory_present[i] = i == 0 || i == 3 || i == 6;
    }

    cd::AudioTelemetry telemetry{};
    telemetry.master_rms = 0.14F;
    telemetry.slot_rms = {0.13F, 0.06F, 0.03F, 0.0F};
    telemetry.slot_event = {0.0F, 0.18F, 0.0F, 0.0F};
    approved_draw(renderer, session, state, telemetry, 0.31F,
        kAndroidUiWidth, kAndroidUiHeight);

    const std::filesystem::path output = output_directory /
        (filename_override != nullptr ? filename_override : page_filename(page));
    const bool saved = SDL_SaveBMP(surface, output.string().c_str()) == 0;
    if (!saved) {
        std::cerr << "SDL_SaveBMP " << output << ": " << SDL_GetError() << '\n';
    }
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
    return saved;
}

} // namespace

int main(int argc, char** argv) {
    const std::filesystem::path output_directory = argc > 1
        ? std::filesystem::path{argv[1]}
        : std::filesystem::path{"android-ui-snapshots"};
    std::error_code error;
    std::filesystem::create_directories(output_directory, error);
    if (error) {
        std::cerr << "Cannot create output directory: " << error.message() << '\n';
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init: " << SDL_GetError() << '\n';
        return 1;
    }

    constexpr std::array<Page, 5> pages{{
        Page::place, Page::actor, Page::fx, Page::master, Page::memory}};
    bool success = true;
    for (Page page : pages) {
        success = render_page(output_directory, page) && success;
    }
    success = render_page(output_directory, Page::actor,
        ActorSection::events, "android-actor-events.bmp") && success;
    success = render_page(output_directory, Page::actor,
        ActorSection::modulation, "android-actor-modulation.bmp") && success;
    SDL_Quit();
    return success ? 0 : 1;
}

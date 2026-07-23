// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.

// Reuse the 0.13 Android platform bridge, storage, audio, pickers and Session
// mutation code. Only SDL_main is renamed; this file supplies the approved
// widescreen renderer and the real entry point.
#include "cursed_drone/parameter_mapping.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

#define SDL_main cursed_drone_legacy_android_main
#include "android_touch_main.cpp"
#undef SDL_main

namespace {

constexpr int kAndroidUiWidth = 1496;
constexpr int kAndroidUiHeight = 672;
std::atomic<std::uint32_t> g_audio_deadline_misses{0U};

#include "approved_ui_compat.inc"
#include "approved_ui_primitives.inc"
#define a_actor a_actor_legacy
#include "approved_ui_actor.inc"
#undef a_actor
#include "approved_ui_actor_exact.inc"
#include "approved_ui_fx_memory.inc"

void monitored_audio_callback(void* userdata, Uint8* bytes, int byte_count) {
    const Uint64 started = SDL_GetPerformanceCounter();
    audio_callback(userdata, bytes, byte_count);
    const auto& bridge = *static_cast<AudioBridge*>(userdata);
    const auto frames = static_cast<double>(byte_count) /
        static_cast<double>(sizeof(cd::StereoFrame));
    const double available = frames /
        static_cast<double>(std::max(1.0F, bridge.sample_rate));
    const double elapsed = static_cast<double>(
        SDL_GetPerformanceCounter() - started) /
        static_cast<double>(SDL_GetPerformanceFrequency());
    if (elapsed > available * 0.90) {
        g_audio_deadline_misses.fetch_add(1U, std::memory_order_relaxed);
    }
}

void logical_touch_position(SDL_Renderer* renderer, SDL_Window* window,
    float normalized_x, float normalized_y, int& x, int& y) {
    int window_width = 1;
    int window_height = 1;
    SDL_GetWindowSize(window, &window_width, &window_height);
    const int window_x = static_cast<int>(std::lround(
        normalized_x * static_cast<float>(window_width)));
    const int window_y = static_cast<int>(std::lround(
        normalized_y * static_cast<float>(window_height)));
    float logical_x = 0.0F;
    float logical_y = 0.0F;
    SDL_RenderWindowToLogical(renderer, window_x, window_y,
        &logical_x, &logical_y);
    x = static_cast<int>(std::lround(logical_x));
    y = static_cast<int>(std::lround(logical_y));
}

} // namespace

extern "C" int SDL_main(int argc, char** argv) {
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    prepare_storage();
    SDL_Window* window = SDL_CreateWindow("Cursed Drone",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP |
        SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer* renderer = window != nullptr ? SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) : nullptr;
    if (renderer == nullptr && window != nullptr)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (window == nullptr || renderer == nullptr) {
        std::fprintf(stderr, "SDL video: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderSetLogicalSize(renderer, kAndroidUiWidth, kAndroidUiHeight);
    show_splash(renderer, kAndroidUiWidth, kAndroidUiHeight);

    cd::Session session = cd::make_default_session();
    if (!g_autosave_path.empty() && std::filesystem::exists(g_autosave_path)) {
        std::string error;
        if (!cd::load_session(g_autosave_path, session, error))
            std::fprintf(stderr, "autosave load: %s\n", error.c_str());
    }
    session.performance.morph = 0.0F;
    session.performance.morph_target = session.scene;

    const int native_rate = parse_argument(argc, argv,
        "--android-audio-rate=", 48'000);
    const int burst = parse_argument(argc, argv,
        "--android-audio-burst=", 256);
    AudioBridge audio{};
    SDL_AudioSpec desired{};
    SDL_AudioSpec obtained{};
    desired.freq = native_rate;
    desired.format = AUDIO_F32SYS;
    desired.channels = 2;
    desired.samples = static_cast<Uint16>(std::clamp(
        next_power_of_two(std::max(512, burst * 3)), 512, 2048));
    desired.callback = monitored_audio_callback;
    desired.userdata = &audio;
    const SDL_AudioDeviceID device = SDL_OpenAudioDevice(nullptr, 0,
        &desired, &obtained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (device == 0 || obtained.format != AUDIO_F32SYS ||
        obtained.channels != 2) {
        std::fprintf(stderr, "SDL audio: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    std::fprintf(stderr,
        "android audio requested rate=%d burst=%d callback=%u; "
        "obtained rate=%d callback=%u channels=%u format=0x%x\n",
        native_rate, burst, static_cast<unsigned>(desired.samples),
        obtained.freq, static_cast<unsigned>(obtained.samples),
        static_cast<unsigned>(obtained.channels),
        static_cast<unsigned>(obtained.format));
    audio.graph.prepare({static_cast<float>(obtained.freq), obtained.samples},
        session);
    audio.sample_rate = static_cast<float>(obtained.freq);
    SDL_PauseAudioDevice(device, 0);

    UiState state{};
    refresh_memory_cache(state);
    bool running = true;
    bool changed = false;
    bool save_pending = false;
    Uint32 changed_at = 0;
    Uint32 previous = SDL_GetTicks();
    Uint32 last_audio_report = previous;
    std::uint32_t previous_deadline_misses = 0U;
    while (running) {
        SDL_Event event{};
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) running = false;
            else if (event.type == SDL_KEYDOWN && event.key.repeat == 0 &&
                (event.key.keysym.sym == SDLK_ESCAPE ||
                    event.key.keysym.sym == SDLK_AC_BACK)) {
                if (state.picker != PickerKind::none)
                    state.picker = PickerKind::none;
                else if (state.page != Page::place)
                    state.page = Page::place;
                else running = false;
            } else if (event.type == SDL_FINGERDOWN && !state.finger_down) {
                state.finger_down = true;
                state.finger_id = event.tfinger.fingerId;
                int x = 0;
                int y = 0;
                logical_touch_position(renderer, window, event.tfinger.x,
                    event.tfinger.y, x, y);
                changed = approved_press(session, state,
                    hit_at(state, x, y), x) || changed;
            } else if (event.type == SDL_FINGERMOTION && state.finger_down &&
                event.tfinger.fingerId == state.finger_id &&
                state.slider_active) {
                int x = 0;
                int y = 0;
                logical_touch_position(renderer, window, event.tfinger.x,
                    event.tfinger.y, x, y);
                const float normalized = static_cast<float>(
                    x - state.pressed.rect.x) / static_cast<float>(
                        std::max(1, state.pressed.rect.w));
                set_slider_value(session, state, state.pressed, normalized);
                changed = true;
            } else if (event.type == SDL_FINGERUP && state.finger_down &&
                event.tfinger.fingerId == state.finger_id) {
                int x = 0;
                int y = 0;
                logical_touch_position(renderer, window, event.tfinger.x,
                    event.tfinger.y, x, y);
                changed = approved_release(session, state, x, y) || changed;
                state.finger_down = false;
            }
        }
        const Uint32 now = SDL_GetTicks();
        changed = update_fade(session, state,
            static_cast<float>(now - previous) * 0.001F) || changed;
        previous = now;
        if (state.pending_trigger >= 0) {
            audio.graph.trigger_slot(static_cast<std::size_t>(
                state.pending_trigger));
            set_toast(state, ru(session) ? "СОБЫТИЕ ЗАПУЩЕНО" :
                "EVENT TRIGGERED");
            state.pending_trigger = -1;
        }
        if (g_panic_requested) {
            audio.graph.panic();
            set_toast(state, ru(session) ? "ЗВУК СБРОШЕН" :
                "KILL SILENCE");
            g_panic_requested = false;
        }
        if (changed) {
            static_cast<void>(audio.graph.submit_session(session));
            save_pending = true;
            changed_at = now;
            changed = false;
        }
        if (save_pending && !g_autosave_path.empty() &&
            now - changed_at >= 750U) {
            std::string error;
            if (cd::save_session(session, g_autosave_path, error))
                save_pending = false;
            else changed_at = now;
        }
        if (now - last_audio_report >= 2'000U) {
            const auto misses = g_audio_deadline_misses.load(
                std::memory_order_relaxed);
            std::fprintf(stderr,
                "android audio dsp=%.1f%% deadline_misses=%u (+%u)\n",
                static_cast<double>(audio.cpu_load.load(
                    std::memory_order_relaxed) * 100.0F),
                static_cast<unsigned>(misses),
                static_cast<unsigned>(misses - previous_deadline_misses));
            previous_deadline_misses = misses;
            last_audio_report = now;
        }
        approved_draw(renderer, session, state, audio.graph.telemetry(),
            audio.cpu_load.load(std::memory_order_relaxed),
            kAndroidUiWidth, kAndroidUiHeight);
        SDL_Delay(1);
    }
    if (save_pending && !g_autosave_path.empty()) {
        std::string error;
        static_cast<void>(cd::save_session(session, g_autosave_path, error));
    }
    SDL_CloseAudioDevice(device);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

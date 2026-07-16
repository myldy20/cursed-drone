// SPDX-License-Identifier: GPL-3.0-or-later
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "cursed_drone/audio.hpp"
#include "cursed_drone/session.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace cd = cursed_drone;

namespace {

struct AudioBridge {
    cd::AudioGraph graph{};
};

void audio_callback(void* userdata, Uint8* bytes, int byte_count) {
    auto& bridge = *static_cast<AudioBridge*>(userdata);
    auto* frames = reinterpret_cast<cd::StereoFrame*>(bytes);
    const auto count = static_cast<std::size_t>(byte_count) / sizeof(cd::StereoFrame);
    bridge.graph.process(cd::BufferView<cd::StereoFrame>{frames, count});
}

void log_sdl_error(const char* operation) {
    std::cerr << "ERROR " << operation << ": " << SDL_GetError() << '\n';
}

const char* safe_name(const char* value) {
    return value == nullptr ? "unknown" : value;
}

void draw_probe(SDL_Renderer* renderer, Uint32 elapsed_ms) {
    SDL_SetRenderDrawColor(renderer, 7, 6, 11, 255);
    SDL_RenderClear(renderer);

    constexpr std::array<SDL_Color, 4> colors{{
        {197, 64, 84, 255},
        {215, 132, 49, 255},
        {66, 155, 145, 255},
        {103, 79, 166, 255},
    }};
    constexpr int margin = 12;
    constexpr int gap = 8;
    constexpr int panel_width = 116;
    const float phase = static_cast<float>(elapsed_ms % 2000U) / 2000.0F;

    for (int index = 0; index < 4; ++index) {
        const auto color = colors[static_cast<std::size_t>(index)];
        const float offset = static_cast<float>(index) * 0.17F;
        const float wrapped = phase + offset - static_cast<float>(static_cast<int>(phase + offset));
        const float pulse = 0.35F + 0.65F * (1.0F - std::abs(wrapped * 2.0F - 1.0F));
        SDL_SetRenderDrawColor(
            renderer,
            static_cast<Uint8>(static_cast<float>(color.r) * pulse),
            static_cast<Uint8>(static_cast<float>(color.g) * pulse),
            static_cast<Uint8>(static_cast<float>(color.b) * pulse),
            255);
        SDL_Rect panel{margin + index * (panel_width + gap), 54, panel_width, 276};
        SDL_RenderFillRect(renderer, &panel);

        SDL_SetRenderDrawColor(renderer, 238, 225, 198, 255);
        const int meter_height = 30 + static_cast<int>(190.0F * pulse);
        SDL_Rect meter{panel.x + 43, panel.y + panel.h - meter_height - 18, 30, meter_height};
        SDL_RenderFillRect(renderer, &meter);
    }
    SDL_RenderPresent(renderer);
}

} // namespace

int main(int argc, char** argv) {
    SDL_SetMainReady();
    bool windowed = false;
    Uint32 duration_ms = 90'000U;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--windowed") {
            windowed = true;
        } else if (argument == "--seconds" && index + 1 < argc) {
            duration_ms = static_cast<Uint32>(std::max(5, std::stoi(argv[++index])) * 1000);
        }
    }

    std::cout << "CURSED_DRONE_PROBE version=1\n";
    std::cout << "platform=" << safe_name(SDL_GetPlatform()) << '\n';
    std::cout.flush();

    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) != 0) {
        log_sdl_error("SDL_Init");
        return 1;
    }

    std::cout << "video_driver=" << safe_name(SDL_GetCurrentVideoDriver()) << '\n';
    std::cout << "audio_driver=" << safe_name(SDL_GetCurrentAudioDriver()) << '\n';
    std::cout << "cpu_count=" << SDL_GetCPUCount() << '\n';
    std::cout << "system_ram_mb=" << SDL_GetSystemRAM() << '\n';
    std::cout << "joystick_count=" << SDL_NumJoysticks() << '\n';

    std::vector<SDL_GameController*> controllers;
    std::vector<SDL_Joystick*> joysticks;
    for (int index = 0; index < SDL_NumJoysticks(); ++index) {
        const char* name = SDL_JoystickNameForIndex(index);
        std::cout << "input[" << index << "].name=" << safe_name(name)
                  << " gamecontroller=" << (SDL_IsGameController(index) == SDL_TRUE ? 1 : 0) << '\n';
        if (SDL_IsGameController(index) == SDL_TRUE) {
            if (auto* controller = SDL_GameControllerOpen(index); controller != nullptr) {
                controllers.push_back(controller);
                char* mapping = SDL_GameControllerMapping(controller);
                std::cout << "input[" << index << "].mapping=" << safe_name(mapping) << '\n';
                SDL_free(mapping);
            }
        } else if (auto* joystick = SDL_JoystickOpen(index); joystick != nullptr) {
            joysticks.push_back(joystick);
        }
    }

    const Uint32 flags = SDL_WINDOW_SHOWN | (windowed ? 0U : SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_Window* window = SDL_CreateWindow(
        "Cursed Drone Device Probe — press every button, Start exits",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1024,
        768,
        flags);
    if (window == nullptr) {
        log_sdl_error("SDL_CreateWindow");
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (renderer == nullptr) {
        log_sdl_error("SDL_CreateRenderer");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(renderer, 512, 384);
    SDL_RendererInfo renderer_info{};
    if (SDL_GetRendererInfo(renderer, &renderer_info) == 0) {
        std::cout << "renderer=" << safe_name(renderer_info.name)
                  << " accelerated=" << ((renderer_info.flags & SDL_RENDERER_ACCELERATED) != 0U ? 1 : 0)
                  << " vsync=" << ((renderer_info.flags & SDL_RENDERER_PRESENTVSYNC) != 0U ? 1 : 0) << '\n';
    }

    AudioBridge audio{};
    SDL_AudioSpec desired{};
    SDL_AudioSpec obtained{};
    desired.freq = 48'000;
    desired.format = AUDIO_F32SYS;
    desired.channels = 2;
    desired.samples = 512;
    desired.callback = audio_callback;
    desired.userdata = &audio;
    const SDL_AudioDeviceID device = SDL_OpenAudioDevice(
        nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (device == 0 || obtained.format != AUDIO_F32SYS || obtained.channels != 2) {
        log_sdl_error("SDL_OpenAudioDevice stereo-f32");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    std::cout << "audio_obtained.freq=" << obtained.freq << '\n';
    std::cout << "audio_obtained.channels=" << static_cast<int>(obtained.channels) << '\n';
    std::cout << "audio_obtained.samples=" << obtained.samples << '\n';
    std::cout << "PROBE_READY press every button; Start or Escape exits after two seconds\n";
    std::cout.flush();

    audio.graph.prepare({static_cast<float>(obtained.freq), obtained.samples}, cd::make_default_session());
    SDL_PauseAudioDevice(device, 0);

    const Uint32 started = SDL_GetTicks();
    unsigned key_events = 0U;
    unsigned controller_events = 0U;
    unsigned joystick_events = 0U;
    bool running = true;
    while (running && SDL_GetTicks() - started < duration_ms) {
        SDL_Event event{};
        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
            case SDL_QUIT:
                std::cout << "event=quit\n";
                running = false;
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                ++key_events;
                std::cout << "event=key " << (event.type == SDL_KEYDOWN ? "down" : "up")
                          << " scancode=" << SDL_GetScancodeName(event.key.keysym.scancode)
                          << " keycode=" << event.key.keysym.sym << '\n';
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE && SDL_GetTicks() - started > 2000U) {
                    running = false;
                }
                break;
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
                ++controller_events;
                std::cout << "event=controller_button "
                          << (event.type == SDL_CONTROLLERBUTTONDOWN ? "down" : "up")
                          << " id=" << static_cast<int>(event.cbutton.button)
                          << " name=" << safe_name(SDL_GameControllerGetStringForButton(
                                 static_cast<SDL_GameControllerButton>(event.cbutton.button))) << '\n';
                if (event.type == SDL_CONTROLLERBUTTONDOWN &&
                    event.cbutton.button == SDL_CONTROLLER_BUTTON_START && SDL_GetTicks() - started > 2000U) {
                    running = false;
                }
                break;
            case SDL_CONTROLLERAXISMOTION:
                if (std::abs(static_cast<int>(event.caxis.value)) > 12'000) {
                    ++controller_events;
                    std::cout << "event=controller_axis id=" << static_cast<int>(event.caxis.axis)
                              << " value=" << event.caxis.value << '\n';
                }
                break;
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                ++joystick_events;
                std::cout << "event=joystick_button "
                          << (event.type == SDL_JOYBUTTONDOWN ? "down" : "up")
                          << " id=" << static_cast<int>(event.jbutton.button) << '\n';
                break;
            case SDL_JOYAXISMOTION:
                if (std::abs(static_cast<int>(event.jaxis.value)) > 12'000) {
                    ++joystick_events;
                    std::cout << "event=joystick_axis id=" << static_cast<int>(event.jaxis.axis)
                              << " value=" << event.jaxis.value << '\n';
                }
                break;
            default:
                break;
            }
        }
        draw_probe(renderer, SDL_GetTicks() - started);
        std::cout.flush();
        SDL_Delay(4U);
    }

    SDL_PauseAudioDevice(device, 1);
    SDL_CloseAudioDevice(device);
    std::cout << "PROBE_COMPLETE key_events=" << key_events
              << " controller_events=" << controller_events
              << " joystick_events=" << joystick_events << '\n';
    for (auto* controller : controllers) {
        SDL_GameControllerClose(controller);
    }
    for (auto* joystick : joysticks) {
        SDL_JoystickClose(joystick);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

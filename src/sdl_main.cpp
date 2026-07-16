// SPDX-License-Identifier: GPL-3.0-or-later
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "cursed_drone/audio.hpp"
#include "cursed_drone/i18n.hpp"
#include "cursed_drone/session.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <span>
#include <string>

namespace cd = cursed_drone;

namespace {

struct AudioBridge {
    cd::AudioGraph graph{};
};

void audio_callback(void* userdata, Uint8* bytes, int byte_count) {
    auto& bridge = *static_cast<AudioBridge*>(userdata);
    auto* frames = reinterpret_cast<cd::StereoFrame*>(bytes);
    const auto count = static_cast<std::size_t>(byte_count) / sizeof(cd::StereoFrame);
    bridge.graph.process(std::span<cd::StereoFrame>{frames, count});
}

float* selected_parameter(cd::SlotSettings& slot, int parameter) {
    switch (parameter) {
    case 0: return &slot.frequency_hz;
    case 1: return &slot.timbre;
    case 2: return &slot.color;
    case 3: return &slot.motion;
    case 4: return &slot.texture;
    default: return &slot.level;
    }
}

float selected_value(const cd::SlotSettings& slot, int parameter) {
    switch (parameter) {
    case 0: return slot.frequency_hz;
    case 1: return slot.timbre;
    case 2: return slot.color;
    case 3: return slot.motion;
    case 4: return slot.texture;
    default: return slot.level;
    }
}

cd::TextId selected_text(int parameter) {
    constexpr std::array names{
        cd::TextId::frequency,
        cd::TextId::timbre,
        cd::TextId::color,
        cd::TextId::motion,
        cd::TextId::texture,
        cd::TextId::level,
    };
    return names.at(static_cast<std::size_t>(parameter));
}

void adjust(cd::Session& session, int slot_index, int parameter, float direction) {
    auto& slot = session.slots[static_cast<std::size_t>(slot_index)];
    float* value = selected_parameter(slot, parameter);
    if (parameter == 0) {
        *value = std::clamp(*value * std::pow(2.0F, direction / 12.0F), 8.0F, 2'000.0F);
    } else {
        *value = std::clamp(*value + direction * 0.025F, 0.0F, 1.0F);
    }
}

void update_title(SDL_Window* window, const cd::Session& session, int slot_index, int parameter) {
    const auto& slot = session.slots[static_cast<std::size_t>(slot_index)];
    const float value = selected_value(slot, parameter);
    char number[32]{};
    std::snprintf(number, sizeof(number), parameter == 0 ? "%.1f Hz" : "%.2f", static_cast<double>(value));
    const std::string title = std::string{cd::tr(session.locale, cd::TextId::app_name)} + " — " +
        std::string{cd::tr(session.locale, cd::TextId::slot)} + " " + std::to_string(slot_index + 1) +
        " · " + std::string{cd::tr(session.locale, selected_text(parameter))} + " " + number;
    SDL_SetWindowTitle(window, title.c_str());
}

void draw(SDL_Renderer* renderer, const cd::Session& session, int selected_slot, int selected_parameter_index) {
    SDL_SetRenderDrawColor(renderer, 8, 7, 12, 255);
    SDL_RenderClear(renderer);
    constexpr int logical_width = 512;
    constexpr int margin = 12;
    constexpr int gap = 8;
    constexpr int panel_width = (logical_width - margin * 2 - gap * 3) / 4;

    for (int index = 0; index < static_cast<int>(cd::kSlotCount); ++index) {
        const auto& slot = session.slots[static_cast<std::size_t>(index)];
        const int x = margin + index * (panel_width + gap);
        SDL_Rect panel{x, 50, panel_width, 316};
        if (index == selected_slot) {
            SDL_SetRenderDrawColor(renderer, 117, 67, 171, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 35, 30, 46, 255);
        }
        SDL_RenderFillRect(renderer, &panel);

        constexpr std::array<SDL_Color, 4> colors{{
            {216, 88, 88, 255},
            {224, 154, 63, 255},
            {80, 169, 154, 255},
            {91, 122, 187, 255},
        }};
        for (int effect = 0; effect < static_cast<int>(cd::kEffectsPerSlot); ++effect) {
            SDL_SetRenderDrawColor(
                renderer,
                colors[static_cast<std::size_t>(effect)].r,
                colors[static_cast<std::size_t>(effect)].g,
                colors[static_cast<std::size_t>(effect)].b,
                255);
            const int width = static_cast<int>(
                static_cast<float>(panel_width - 18) * slot.effects[static_cast<std::size_t>(effect)].amount);
            SDL_Rect effect_bar{x + 9, 232 + effect * 25, width, 13};
            SDL_RenderFillRect(renderer, &effect_bar);
        }

        const float normalized = selected_parameter_index == 0
            ? std::clamp(std::log2(slot.frequency_hz / 8.0F) / 8.0F, 0.0F, 1.0F)
            : selected_value(slot, selected_parameter_index);
        SDL_SetRenderDrawColor(renderer, 236, 224, 193, 255);
        SDL_Rect parameter_bar{x + 9, 72, panel_width - 18, 140};
        SDL_RenderDrawRect(renderer, &parameter_bar);
        SDL_Rect fill{x + 12, 209 - static_cast<int>(132.0F * normalized), panel_width - 24, static_cast<int>(132.0F * normalized)};
        SDL_RenderFillRect(renderer, &fill);

        if (!slot.enabled) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 170);
            SDL_RenderFillRect(renderer, &panel);
        }
    }
    SDL_RenderPresent(renderer);
}

} // namespace

int main(int, char**) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Cursed Drone",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1024,
        768,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer* renderer = window != nullptr
        ? SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)
        : nullptr;
    if (window == nullptr || renderer == nullptr) {
        std::fprintf(stderr, "SDL video: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(renderer, 512, 384);

    cd::Session session = cd::make_default_session();
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
        std::fprintf(stderr, "SDL audio: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    audio.graph.prepare({static_cast<float>(obtained.freq), obtained.samples}, session);
    SDL_PauseAudioDevice(device, 0);

    SDL_GameController* controller = nullptr;
    for (int joystick = 0; joystick < SDL_NumJoysticks(); ++joystick) {
        if (SDL_IsGameController(joystick) == SDL_TRUE) {
            controller = SDL_GameControllerOpen(joystick);
            break;
        }
    }

    int selected_slot = 0;
    int selected_parameter_index = 0;
    bool running = true;
    update_title(window, session, selected_slot, selected_parameter_index);
    while (running) {
        SDL_Event event{};
        while (SDL_PollEvent(&event) != 0) {
            bool controls_changed = false;
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE: running = false; break;
                case SDLK_LEFT: selected_slot = (selected_slot + 3) % 4; controls_changed = true; break;
                case SDLK_RIGHT: selected_slot = (selected_slot + 1) % 4; controls_changed = true; break;
                case SDLK_UP: selected_parameter_index = (selected_parameter_index + 5) % 6; controls_changed = true; break;
                case SDLK_DOWN: selected_parameter_index = (selected_parameter_index + 1) % 6; controls_changed = true; break;
                case SDLK_a: adjust(session, selected_slot, selected_parameter_index, -1.0F); controls_changed = true; break;
                case SDLK_d: adjust(session, selected_slot, selected_parameter_index, 1.0F); controls_changed = true; break;
                case SDLK_l:
                    session.locale = session.locale == cd::Locale::ru ? cd::Locale::en : cd::Locale::ru;
                    controls_changed = true;
                    break;
                case SDLK_SPACE:
                    session.slots[static_cast<std::size_t>(selected_slot)].enabled =
                        !session.slots[static_cast<std::size_t>(selected_slot)].enabled;
                    controls_changed = true;
                    break;
                default: break;
                }
            } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                switch (event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT: selected_slot = (selected_slot + 3) % 4; controls_changed = true; break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: selected_slot = (selected_slot + 1) % 4; controls_changed = true; break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP: selected_parameter_index = (selected_parameter_index + 5) % 6; controls_changed = true; break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN: selected_parameter_index = (selected_parameter_index + 1) % 6; controls_changed = true; break;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                    adjust(session, selected_slot, selected_parameter_index, -1.0F);
                    controls_changed = true;
                    break;
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                    adjust(session, selected_slot, selected_parameter_index, 1.0F);
                    controls_changed = true;
                    break;
                case SDL_CONTROLLER_BUTTON_A:
                    session.slots[static_cast<std::size_t>(selected_slot)].enabled =
                        !session.slots[static_cast<std::size_t>(selected_slot)].enabled;
                    controls_changed = true;
                    break;
                case SDL_CONTROLLER_BUTTON_Y:
                    session.locale = session.locale == cd::Locale::ru ? cd::Locale::en : cd::Locale::ru;
                    controls_changed = true;
                    break;
                default: break;
                }
            }
            if (controls_changed) {
                static_cast<void>(audio.graph.submit_session(session));
                update_title(window, session, selected_slot, selected_parameter_index);
            }
        }
        draw(renderer, session, selected_slot, selected_parameter_index);
        SDL_Delay(8);
    }

    SDL_CloseAudioDevice(device);
    if (controller != nullptr) {
        SDL_GameControllerClose(controller);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design

#include <SDL.h>
#include <emscripten/emscripten.h>

#include <algorithm>

extern "C" int SDL_main(int argc, char** argv);

extern "C" EMSCRIPTEN_KEEPALIVE void cursed_drone_web_resize(
    int pixel_width, int pixel_height) {
    if (SDL_WasInit(SDL_INIT_VIDEO) == 0U) return;
    SDL_Window* window = SDL_GetWindowFromID(1U);
    if (window == nullptr) return;
    SDL_SetWindowSize(window, std::max(320, pixel_width),
        std::max(240, pixel_height));
}

int main(int argc, char** argv) {
    // Let SDL translate browser mouse input into its native finger-event path.
    // SDL owns CSS, backing-buffer, HiDPI and logical-viewport conversion, so
    // clicks and drags stay aligned with the rendered controls on Retina Macs.
    SDL_SetHintWithPriority(SDL_HINT_MOUSE_TOUCH_EVENTS, "1",
        SDL_HINT_OVERRIDE);
    return SDL_main(argc, argv);
}

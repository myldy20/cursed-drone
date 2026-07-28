// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design

#include <SDL.h>
#include <emscripten/emscripten.h>

#include <algorithm>

extern "C" int SDL_main(int argc, char** argv);

namespace {

constexpr SDL_TouchID kWebMouseTouchId = 1;
constexpr SDL_FingerID kWebMouseFingerId = 1;
bool g_mouse_down = false;
float g_previous_x = 0.0F;
float g_previous_y = 0.0F;

void push_finger_event(Uint32 type, float x, float y) {
    if (SDL_WasInit(SDL_INIT_EVENTS) == 0U) return;
    SDL_Event event{};
    event.type = type;
    event.tfinger.type = type;
    event.tfinger.timestamp = SDL_GetTicks();
    event.tfinger.touchId = kWebMouseTouchId;
    event.tfinger.fingerId = kWebMouseFingerId;
    event.tfinger.x = x;
    event.tfinger.y = y;
    event.tfinger.dx = x - g_previous_x;
    event.tfinger.dy = y - g_previous_y;
    event.tfinger.pressure = type == SDL_FINGERUP ? 0.0F : 1.0F;
    static_cast<void>(SDL_PushEvent(&event));
    g_previous_x = x;
    g_previous_y = y;
}

} // namespace

extern "C" EMSCRIPTEN_KEEPALIVE void cursed_drone_web_mouse_event(
    int event_type, double target_x, double target_y,
    double canvas_width, double canvas_height) {
    const float x = std::clamp(static_cast<float>(
        target_x / std::max(1.0, canvas_width)), 0.0F, 1.0F);
    const float y = std::clamp(static_cast<float>(
        target_y / std::max(1.0, canvas_height)), 0.0F, 1.0F);

    switch (event_type) {
    case 0: // mouse down
        g_mouse_down = true;
        g_previous_x = x;
        g_previous_y = y;
        push_finger_event(SDL_FINGERDOWN, x, y);
        break;
    case 1: // mouse move
        if (g_mouse_down) push_finger_event(SDL_FINGERMOTION, x, y);
        break;
    case 2: // mouse up
        if (g_mouse_down) {
            push_finger_event(SDL_FINGERUP, x, y);
            g_mouse_down = false;
        }
        break;
    default:
        break;
    }
}

EM_JS(void, install_mouse_bridge, (), {
    const attach = () => {
        const canvas = document.querySelector('#canvas');
        if (!canvas) {
            requestAnimationFrame(attach);
            return;
        }
        if (canvas.dataset.cursedDroneMouseBridge === '1') return;
        canvas.dataset.cursedDroneMouseBridge = '1';

        const forward = (type, event) => {
            const rect = canvas.getBoundingClientRect();
            _cursed_drone_web_mouse_event(
                type,
                event.clientX - rect.left,
                event.clientY - rect.top,
                rect.width,
                rect.height);
        };

        canvas.addEventListener('mousedown', (event) => {
            if (event.button !== 0) return;
            forward(0, event);
            event.preventDefault();
        }, {capture: true, passive: false});

        window.addEventListener('mousemove', (event) => {
            if ((event.buttons & 1) === 0) return;
            forward(1, event);
            event.preventDefault();
        }, {capture: true, passive: false});

        window.addEventListener('mouseup', (event) => {
            if (event.button !== 0) return;
            forward(2, event);
            event.preventDefault();
        }, {capture: true, passive: false});
    };
    attach();
});

extern "C" EMSCRIPTEN_KEEPALIVE void cursed_drone_web_resize(
    int pixel_width, int pixel_height) {
    if (SDL_WasInit(SDL_INIT_VIDEO) == 0U) return;
    SDL_Window* window = SDL_GetWindowFromID(1U);
    if (window == nullptr) return;
    SDL_SetWindowSize(window, std::max(320, pixel_width),
        std::max(240, pixel_height));
}

int main(int argc, char** argv) {
    install_mouse_bridge();
    return SDL_main(argc, argv);
}

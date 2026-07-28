// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design

#include <SDL.h>
#include <emscripten/emscripten.h>

#include <algorithm>
#include <cmath>

extern "C" int SDL_main(int argc, char** argv);

namespace {

constexpr SDL_TouchID kWebMouseTouchId = 1;
constexpr SDL_FingerID kWebMouseFingerId = 1;
constexpr double kLogicalWidth = 1496.0;
constexpr double kLogicalHeight = 672.0;
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

bool normalized_logical_position(double target_x, double target_y,
    double canvas_width, double canvas_height, bool clamp_to_view,
    float& normalized_x, float& normalized_y) {
    if (canvas_width <= 0.0 || canvas_height <= 0.0) return false;

    // The canvas fills the browser viewport, while SDL preserves the fixed
    // 1496x672 logical aspect ratio inside it. Map CSS coordinates into that
    // actual rendered viewport before converting them back to SDL window
    // coordinates. This keeps mouse hit testing aligned on Retina/HiDPI Macs.
    const double scale = std::min(canvas_width / kLogicalWidth,
        canvas_height / kLogicalHeight);
    if (scale <= 0.0) return false;
    const double content_width = kLogicalWidth * scale;
    const double content_height = kLogicalHeight * scale;
    const double offset_x = (canvas_width - content_width) * 0.5;
    const double offset_y = (canvas_height - content_height) * 0.5;
    double logical_x = (target_x - offset_x) / scale;
    double logical_y = (target_y - offset_y) / scale;

    const bool inside = logical_x >= 0.0 && logical_x <= kLogicalWidth &&
        logical_y >= 0.0 && logical_y <= kLogicalHeight;
    if (!inside && !clamp_to_view) return false;
    logical_x = std::clamp(logical_x, 0.0, kLogicalWidth);
    logical_y = std::clamp(logical_y, 0.0, kLogicalHeight);

    SDL_Window* window = SDL_GetWindowFromID(1U);
    SDL_Renderer* renderer = window != nullptr ? SDL_GetRenderer(window) : nullptr;
    if (window == nullptr || renderer == nullptr) return false;

    int window_x = 0;
    int window_y = 0;
    SDL_RenderLogicalToWindow(renderer, static_cast<float>(logical_x),
        static_cast<float>(logical_y), &window_x, &window_y);
    int window_width = 1;
    int window_height = 1;
    SDL_GetWindowSize(window, &window_width, &window_height);
    normalized_x = std::clamp(static_cast<float>(window_x) /
        static_cast<float>(std::max(1, window_width)), 0.0F, 1.0F);
    normalized_y = std::clamp(static_cast<float>(window_y) /
        static_cast<float>(std::max(1, window_height)), 0.0F, 1.0F);
    return true;
}

} // namespace

extern "C" EMSCRIPTEN_KEEPALIVE void cursed_drone_web_mouse_event(
    int event_type, double target_x, double target_y,
    double canvas_width, double canvas_height) {
    float x = 0.0F;
    float y = 0.0F;
    const bool clamp_to_view = event_type != 0;
    if (!normalized_logical_position(target_x, target_y, canvas_width,
            canvas_height, clamp_to_view, x, y)) {
        return;
    }

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

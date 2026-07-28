// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design

#include <SDL.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <algorithm>

extern "C" int SDL_main(int argc, char** argv);

namespace {

constexpr SDL_TouchID kWebMouseTouchId = 1;
constexpr SDL_FingerID kWebMouseFingerId = 1;
bool g_mouse_down = false;
float g_previous_x = 0.0F;
float g_previous_y = 0.0F;

void normalized_canvas_position(const EmscriptenMouseEvent& mouse,
    float& x, float& y) {
    double width = 1.0;
    double height = 1.0;
    if (emscripten_get_element_css_size("#canvas", &width, &height) !=
        EMSCRIPTEN_RESULT_SUCCESS) {
        width = 1.0;
        height = 1.0;
    }
    x = std::clamp(static_cast<float>(mouse.targetX / std::max(1.0, width)),
        0.0F, 1.0F);
    y = std::clamp(static_cast<float>(mouse.targetY / std::max(1.0, height)),
        0.0F, 1.0F);
}

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

EM_BOOL bridge_mouse_event(int event_type,
    const EmscriptenMouseEvent* mouse, void*) {
    if (mouse == nullptr) return EM_FALSE;
    float x = 0.0F;
    float y = 0.0F;
    normalized_canvas_position(*mouse, x, y);

    switch (event_type) {
    case EMSCRIPTEN_EVENT_MOUSEDOWN:
        if (mouse->button != 0) return EM_FALSE;
        g_mouse_down = true;
        g_previous_x = x;
        g_previous_y = y;
        push_finger_event(SDL_FINGERDOWN, x, y);
        return EM_TRUE;
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
        if (!g_mouse_down || (mouse->buttons & 1U) == 0U) return EM_FALSE;
        push_finger_event(SDL_FINGERMOTION, x, y);
        return EM_TRUE;
    case EMSCRIPTEN_EVENT_MOUSEUP:
        if (mouse->button != 0 || !g_mouse_down) return EM_FALSE;
        push_finger_event(SDL_FINGERUP, x, y);
        g_mouse_down = false;
        return EM_TRUE;
    default:
        return EM_FALSE;
    }
}

void install_mouse_bridge() {
    emscripten_set_mousedown_callback("#canvas", nullptr, true,
        bridge_mouse_event);
    emscripten_set_mousemove_callback("#canvas", nullptr, true,
        bridge_mouse_event);
    emscripten_set_mouseup_callback("#canvas", nullptr, true,
        bridge_mouse_event);
}

} // namespace

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

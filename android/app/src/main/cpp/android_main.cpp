// SPDX-License-Identifier: GPL-3.0-or-later
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <vector>

int cursed_android_init(Uint32 flags);
int cursed_android_poll_event(SDL_Event* event);
int cursed_android_render_set_logical_size(SDL_Renderer* renderer, int width, int height);
void cursed_android_render_present(SDL_Renderer* renderer);

#define main cursed_drone_main_impl
#define SDL_Init cursed_android_init
#define SDL_PollEvent cursed_android_poll_event
#define SDL_RenderSetLogicalSize cursed_android_render_set_logical_size
#define SDL_RenderPresent cursed_android_render_present
#include "../../../../../src/sdl_main.cpp"
#undef SDL_RenderPresent
#undef SDL_RenderSetLogicalSize
#undef SDL_PollEvent
#undef SDL_Init
#undef main

namespace {

constexpr int kAndroidLogicalWidth = 704;
constexpr int kAndroidLogicalHeight = 384;
constexpr int kDesktopUiWidth = 512;
constexpr int kTouchPanelX = 512;
constexpr int kCellWidth = 56;
constexpr int kCellHeight = 48;
constexpr int kGestureStep = 18;

struct TouchCell {
    SDL_Rect rect;
    const char* label;
    SDL_Keycode key;
    int repeat_count;
    bool holdable;
};

constexpr std::array<TouchCell, 13> kTouchCells{{
    {{520,   8, kCellWidth, kCellHeight}, "PAGE-", SDLK_TAB, 4, false},
    {{584,   8, kCellWidth, kCellHeight}, "HELP",  SDLK_h,   1, false},
    {{648,   8, kCellWidth, kCellHeight}, "PAGE+", SDLK_TAB, 1, false},
    {{584,  72, kCellWidth, kCellHeight}, "UP",    SDLK_UP,  1, true},
    {{520, 128, kCellWidth, kCellHeight}, "LEFT",  SDLK_LEFT, 1, true},
    {{584, 128, kCellWidth, kCellHeight}, "A",     SDLK_RETURN, 1, false},
    {{648, 128, kCellWidth, kCellHeight}, "RIGHT", SDLK_RIGHT, 1, true},
    {{584, 184, kCellWidth, kCellHeight}, "DOWN",  SDLK_DOWN, 1, true},
    {{520, 248, kCellWidth, kCellHeight}, "BACK",  SDLK_ESCAPE, 1, false},
    {{584, 248, kCellWidth, kCellHeight}, "SECTION", SDLK_x, 1, false},
    {{648, 248, kCellWidth, kCellHeight}, "MENU",  SDLK_m, 1, false},
    {{520, 312, kCellWidth, kCellHeight}, "FADE",  SDLK_f, 1, false},
    {{648, 312, kCellWidth, kCellHeight}, "KILL",  SDLK_k, 1, false},
}};

SDL_Renderer* g_renderer = nullptr;
std::deque<SDL_Event> g_synthetic_events{};
bool g_finger_active = false;
SDL_FingerID g_active_finger = 0;
SDL_Keycode g_held_key = SDLK_UNKNOWN;
int g_pressed_cell = -1;
float g_touch_last_x = 0.0F;
float g_touch_last_y = 0.0F;
bool g_touch_in_content = false;
bool g_touch_moved = false;
Uint32 g_last_tap_at = 0;
float g_last_tap_x = 0.0F;
float g_last_tap_y = 0.0F;

bool copy_asset(const char* source_name, const std::filesystem::path& destination) {
    SDL_RWops* source = SDL_RWFromFile(source_name, "rb");
    if (source == nullptr) return false;

    const Sint64 byte_count = SDL_RWsize(source);
    if (byte_count <= 0) {
        SDL_RWclose(source);
        return false;
    }

    std::vector<unsigned char> bytes(static_cast<std::size_t>(byte_count));
    const std::size_t read = SDL_RWread(source, bytes.data(), 1, bytes.size());
    SDL_RWclose(source);
    if (read != bytes.size()) return false;

    std::error_code error;
    std::filesystem::create_directories(destination.parent_path(), error);
    std::ofstream output(destination, std::ios::binary | std::ios::trunc);
    if (!output) return false;
    output.write(reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    return output.good();
}

void prepare_android_data() {
    char* preference_path = SDL_GetPrefPath("myldy20", "cursed-drone");
    if (preference_path == nullptr) return;

    const std::filesystem::path root{preference_path};
    SDL_free(preference_path);

    copy_asset("branding/cursed-drone-splash.bmp",
        root / "branding" / "cursed-drone-splash.bmp");
    copy_asset("scales/19-edo.scl", root / "scales" / "19-edo.scl");
    copy_asset("scales/just-minor.scl", root / "scales" / "just-minor.scl");

    const std::string root_string = root.string();
    const std::string branding_string = (root / "branding").string();
    setenv("CURSED_DRONE_DATA_DIR", root_string.c_str(), 1);
    setenv("CURSED_DRONE_ASSET_DIR", branding_string.c_str(), 1);
}

void queue_key(SDL_Keycode key, bool down, Uint32 timestamp) {
    SDL_Event event{};
    event.type = down ? SDL_KEYDOWN : SDL_KEYUP;
    event.key.type = event.type;
    event.key.timestamp = timestamp;
    event.key.state = down ? SDL_PRESSED : SDL_RELEASED;
    event.key.repeat = 0;
    event.key.keysym.scancode = SDL_GetScancodeFromKey(key);
    event.key.keysym.sym = key;
    event.key.keysym.mod = KMOD_NONE;
    g_synthetic_events.push_back(event);
}

void queue_press(SDL_Keycode key, int repeat_count, Uint32 timestamp) {
    for (int index = 0; index < repeat_count; ++index) {
        queue_key(key, true, timestamp);
        queue_key(key, false, timestamp);
    }
}

bool pop_synthetic(SDL_Event* event) {
    if (g_synthetic_events.empty()) return false;
    *event = g_synthetic_events.front();
    g_synthetic_events.pop_front();
    return true;
}

void logical_touch_point(const SDL_TouchFingerEvent& touch, float& x, float& y) {
    x = touch.x * static_cast<float>(kAndroidLogicalWidth);
    y = touch.y * static_cast<float>(kAndroidLogicalHeight);
    if (g_renderer == nullptr) return;

    SDL_Window* window = SDL_GetWindowFromID(touch.windowID);
    if (window == nullptr) return;

    int window_width = 0;
    int window_height = 0;
    SDL_GetWindowSize(window, &window_width, &window_height);
    if (window_width <= 0 || window_height <= 0) return;

    const int window_x = static_cast<int>(std::lround(touch.x * static_cast<float>(window_width)));
    const int window_y = static_cast<int>(std::lround(touch.y * static_cast<float>(window_height)));
    float logical_x = x;
    float logical_y = y;
    SDL_RenderWindowToLogical(g_renderer, window_x, window_y, &logical_x, &logical_y);
    x = logical_x;
    y = logical_y;
}

int cell_at(float x, float y) {
    for (int index = 0; index < static_cast<int>(kTouchCells.size()); ++index) {
        const SDL_Rect& rect = kTouchCells[static_cast<std::size_t>(index)].rect;
        if (x >= static_cast<float>(rect.x) && x < static_cast<float>(rect.x + rect.w) &&
            y >= static_cast<float>(rect.y) && y < static_cast<float>(rect.y + rect.h)) {
            return index;
        }
    }
    return -1;
}

void handle_finger_down(const SDL_TouchFingerEvent& touch) {
    if (g_finger_active) return;
    g_finger_active = true;
    g_active_finger = touch.fingerId;

    float x = 0.0F;
    float y = 0.0F;
    logical_touch_point(touch, x, y);
    g_touch_last_x = x;
    g_touch_last_y = y;
    g_touch_moved = false;
    g_touch_in_content = x < static_cast<float>(kDesktopUiWidth);

    if (g_touch_in_content) return;

    g_pressed_cell = cell_at(x, y);
    if (g_pressed_cell < 0) return;
    const TouchCell& cell = kTouchCells[static_cast<std::size_t>(g_pressed_cell)];
    if (cell.holdable) {
        g_held_key = cell.key;
        queue_key(cell.key, true, touch.timestamp);
    } else {
        queue_press(cell.key, cell.repeat_count, touch.timestamp);
    }
}

void handle_finger_motion(const SDL_TouchFingerEvent& touch) {
    if (!g_finger_active || touch.fingerId != g_active_finger || !g_touch_in_content) return;

    float x = 0.0F;
    float y = 0.0F;
    logical_touch_point(touch, x, y);
    const float dx = x - g_touch_last_x;
    const float dy = y - g_touch_last_y;

    if (std::abs(dx) >= static_cast<float>(kGestureStep) ||
        std::abs(dy) >= static_cast<float>(kGestureStep)) {
        g_touch_moved = true;
        if (std::abs(dx) >= std::abs(dy)) {
            const SDL_Keycode key = dx > 0.0F ? SDLK_RIGHT : SDLK_LEFT;
            const int count = std::max(1, static_cast<int>(std::abs(dx)) / kGestureStep);
            queue_press(key, count, touch.timestamp);
        } else {
            const SDL_Keycode key = dy > 0.0F ? SDLK_DOWN : SDLK_UP;
            const int count = std::max(1, static_cast<int>(std::abs(dy)) / kGestureStep);
            queue_press(key, count, touch.timestamp);
        }
        g_touch_last_x = x;
        g_touch_last_y = y;
    }
}

void handle_finger_up(const SDL_TouchFingerEvent& touch) {
    if (!g_finger_active || touch.fingerId != g_active_finger) return;

    float x = 0.0F;
    float y = 0.0F;
    logical_touch_point(touch, x, y);

    if (g_held_key != SDLK_UNKNOWN) {
        queue_key(g_held_key, false, touch.timestamp);
        g_held_key = SDLK_UNKNOWN;
    } else if (g_touch_in_content && !g_touch_moved) {
        const float dx = x - g_last_tap_x;
        const float dy = y - g_last_tap_y;
        const bool double_tap = touch.timestamp - g_last_tap_at <= 320U &&
            dx * dx + dy * dy <= 32.0F * 32.0F;
        if (double_tap) {
            queue_press(SDLK_x, 1, touch.timestamp);
            g_last_tap_at = 0;
        } else {
            queue_press(SDLK_RETURN, 1, touch.timestamp);
            g_last_tap_at = touch.timestamp;
            g_last_tap_x = x;
            g_last_tap_y = y;
        }
    }

    g_finger_active = false;
    g_pressed_cell = -1;
    g_touch_in_content = false;
    g_touch_moved = false;
}

void draw_rect(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color color, bool filled) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    if (filled) SDL_RenderFillRect(renderer, &rect);
    else SDL_RenderDrawRect(renderer, &rect);
}

void draw_touch_panel(SDL_Renderer* renderer) {
    const SDL_Color background{12, 10, 17, 255};
    const SDL_Color idle{28, 23, 38, 255};
    const SDL_Color active{73, 46, 104, 255};
    const SDL_Color border{105, 92, 118, 255};
    const SDL_Color text{238, 226, 197, 255};
    const SDL_Color dim{151, 143, 158, 255};
    const SDL_Color danger{216, 88, 88, 255};

    draw_rect(renderer, {kTouchPanelX, 0, kAndroidLogicalWidth - kTouchPanelX,
        kAndroidLogicalHeight}, background, true);
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
    SDL_RenderDrawLine(renderer, kTouchPanelX, 0, kTouchPanelX, kAndroidLogicalHeight);

    for (int index = 0; index < static_cast<int>(kTouchCells.size()); ++index) {
        const TouchCell& cell = kTouchCells[static_cast<std::size_t>(index)];
        const bool pressed = index == g_pressed_cell;
        const SDL_Color fill_color = pressed ? active : idle;
        draw_rect(renderer, cell.rect, fill_color, true);
        draw_rect(renderer, cell.rect, cell.key == SDLK_k ? danger : border, false);
        const int text_x = cell.rect.x + (cell.rect.w - cd::ui::text_width(cell.label)) / 2;
        const int text_y = cell.rect.y + (cell.rect.h - 8) / 2;
        cd::ui::draw_text(renderer, text_x, text_y, cell.label,
            cell.key == SDLK_k ? danger : (pressed ? text : dim));
    }

    cd::ui::draw_text(renderer, 530, 232, "TAP = A   DRAG = D-PAD", dim);
    cd::ui::draw_text(renderer, 565, 368, "TOUCH", dim);
}

} // namespace

int cursed_android_init(Uint32 flags) {
    const int result = SDL_Init(flags);
    if (result == 0) prepare_android_data();
    return result;
}

int cursed_android_poll_event(SDL_Event* event) {
    if (pop_synthetic(event)) return 1;

    SDL_Event raw{};
    while (SDL_PollEvent(&raw) != 0) {
        if (raw.type == SDL_FINGERDOWN) {
            handle_finger_down(raw.tfinger);
        } else if (raw.type == SDL_FINGERMOTION) {
            handle_finger_motion(raw.tfinger);
        } else if (raw.type == SDL_FINGERUP) {
            handle_finger_up(raw.tfinger);
        } else {
            *event = raw;
            return 1;
        }
        if (pop_synthetic(event)) return 1;
    }
    return pop_synthetic(event) ? 1 : 0;
}

int cursed_android_render_set_logical_size(SDL_Renderer* renderer, int, int) {
    g_renderer = renderer;
    return SDL_RenderSetLogicalSize(renderer, kAndroidLogicalWidth, kAndroidLogicalHeight);
}

void cursed_android_render_present(SDL_Renderer* renderer) {
    draw_touch_panel(renderer);
    SDL_RenderPresent(renderer);
}

extern "C" int SDL_main(int argc, char** argv) {
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    setenv("CURSED_DRONE_HANDHELD", "1", 0);
    return cursed_drone_main_impl(argc, argv);
}

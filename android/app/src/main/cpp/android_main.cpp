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
#include <string>
#include <string_view>
#include <vector>

int cursed_android_init(Uint32 flags);
SDL_AudioDeviceID cursed_android_open_audio_device(
    const char* device,
    int iscapture,
    const SDL_AudioSpec* desired,
    SDL_AudioSpec* obtained,
    int allowed_changes);
int cursed_android_poll_event(SDL_Event* event);
int cursed_android_render_set_logical_size(SDL_Renderer* renderer, int width, int height);
void cursed_android_render_present(SDL_Renderer* renderer);

#define main cursed_drone_main_impl
#define SDL_Init cursed_android_init
#define SDL_OpenAudioDevice cursed_android_open_audio_device
#define SDL_PollEvent cursed_android_poll_event
#define SDL_RenderSetLogicalSize cursed_android_render_set_logical_size
#define SDL_RenderPresent cursed_android_render_present
#include "../../../../../src/sdl_main.cpp"
#undef SDL_RenderPresent
#undef SDL_RenderSetLogicalSize
#undef SDL_PollEvent
#undef SDL_OpenAudioDevice
#undef SDL_Init
#undef main

namespace {

constexpr int kAndroidUiWidth = 512;
constexpr int kAndroidUiHeight = 384;
constexpr int kDragStep = 10;
constexpr SDL_Rect kFadeRect{428, 358, 74, 20};

struct ShadowState {
    Page page{Page::perform};
    int slot{0};
    std::array<int, 5> selected{};
    std::array<int, 4> slot_selected{};
    int focus_zone{1};
    int effect_field{0};
    int actor_advanced_field{0};
    int actor_modulator{0};
    int master_effect{0};
    int master_effect_field{0};
    int memory_slot{0};
    int memory_action{0};
    int memory_setting{0};
    Picker picker{Picker::none};
    bool picker_master{false};
    int picker_group{0};
    int picker_item{0};
    bool help_open{false};
    bool menu_open{false};
    int menu_item{0};
    std::array<bool, 4> musical_actor{};
};

enum class TargetKind {
    none,
    page,
    fade,
    place_landscape,
    place_macro,
    place_actor,
    place_actor_mute,
    actor_selector,
    actor_basic_tab,
    actor_advanced_tab,
    actor_basic_row,
    actor_advanced_row,
    fx_actor,
    fx_slot,
    fx_row,
    master_signal_row,
    master_fx_slot,
    master_fx_row,
    memory_slot,
    memory_action,
    memory_setting,
    picker_group,
    picker_item,
};

struct TouchTarget {
    TargetKind kind{TargetKind::none};
    int index{0};
    int aux{0};
    bool adjustable{false};
};

SDL_Renderer* g_renderer = nullptr;
std::deque<SDL_Event> g_synthetic_events{};
ShadowState g_shadow{};
bool g_finger_active = false;
SDL_FingerID g_active_finger = 0;
float g_touch_last_x = 0.0F;
float g_touch_last_y = 0.0F;
bool g_touch_moved = false;
TouchTarget g_touch_target{};
int g_android_audio_rate = 48'000;
int g_android_audio_burst = 256;

bool point_in(const SDL_Rect& rect, float x, float y) {
    return x >= static_cast<float>(rect.x) &&
        x < static_cast<float>(rect.x + rect.w) &&
        y >= static_cast<float>(rect.y) &&
        y < static_cast<float>(rect.y + rect.h);
}

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

bool pop_synthetic(SDL_Event* event) {
    if (g_synthetic_events.empty()) return false;
    *event = g_synthetic_events.front();
    g_synthetic_events.pop_front();
    return true;
}

int shadow_page_index(Page page) {
    return static_cast<int>(page);
}

int& shadow_parameter() {
    if (g_shadow.page == Page::slot) {
        return g_shadow.slot_selected[static_cast<std::size_t>(g_shadow.slot)];
    }
    return g_shadow.selected[static_cast<std::size_t>(shadow_page_index(g_shadow.page))];
}

void shadow_change_page(int direction) {
    constexpr int count = 5;
    g_shadow.page = static_cast<Page>(
        (shadow_page_index(g_shadow.page) + direction + count) % count);
    switch (g_shadow.page) {
    case Page::perform:
    case Page::slot:
        g_shadow.focus_zone = 1;
        break;
    case Page::effects:
        g_shadow.focus_zone = 1;
        shadow_parameter() = 0;
        g_shadow.effect_field = 0;
        break;
    case Page::master:
    case Page::memory:
        g_shadow.focus_zone = 0;
        break;
    }
}

void shadow_cycle_focus() {
    g_shadow.focus_zone =
        (g_shadow.focus_zone + 1) % focus_zone_count(g_shadow.page);
    if (g_shadow.page == Page::effects && g_shadow.focus_zone > 0) {
        shadow_parameter() = g_shadow.focus_zone - 1;
        g_shadow.effect_field = 0;
    }
    if (g_shadow.page == Page::master && g_shadow.focus_zone > 0) {
        g_shadow.master_effect = g_shadow.focus_zone - 1;
        g_shadow.master_effect_field = 0;
    }
}

void shadow_move_picker(int horizontal, int vertical) {
    if (g_shadow.picker == Picker::scene) {
        constexpr int rows = 5;
        int row = g_shadow.picker_item % rows;
        int column = g_shadow.picker_item / rows;
        row = (row + vertical + rows) % rows;
        column = (column + horizontal + 2) % 2;
        g_shadow.picker_item = column * rows + row;
        return;
    }
    if (g_shadow.picker == Picker::engine) {
        const int groups = static_cast<int>(kEngineGroups.size());
        g_shadow.picker_group =
            (g_shadow.picker_group + horizontal + groups) % groups;
        g_shadow.picker_item = (g_shadow.picker_item + vertical + 4) % 4;
        return;
    }
    if (g_shadow.picker == Picker::effect) {
        int count = effect_group_size(g_shadow.picker_group);
        const int rows = g_shadow.picker_group == 0 ? 8 : 5;
        int row = g_shadow.picker_item % rows;
        int column = g_shadow.picker_item / rows;
        if (vertical != 0) {
            for (int attempt = 0; attempt < rows; ++attempt) {
                row = (row + vertical + rows) % rows;
                const int candidate = column * rows + row;
                if (candidate < count) {
                    g_shadow.picker_item = candidate;
                    break;
                }
            }
        }
        if (horizontal != 0) {
            const int candidate = (column + horizontal) * rows + row;
            if (candidate >= 0 && candidate < count) {
                g_shadow.picker_item = candidate;
            } else {
                g_shadow.picker_group =
                    (g_shadow.picker_group + horizontal + 2) % 2;
                count = effect_group_size(g_shadow.picker_group);
                g_shadow.picker_item = std::min(row, count - 1);
            }
        }
    }
}

void shadow_go_back() {
    if (g_shadow.help_open) {
        g_shadow.help_open = false;
    } else if (g_shadow.menu_open) {
        g_shadow.menu_open = false;
    } else if (g_shadow.picker != Picker::none) {
        g_shadow.picker = Picker::none;
        g_shadow.picker_master = false;
    } else if (g_shadow.page != Page::perform) {
        g_shadow.page = Page::perform;
        g_shadow.focus_zone = 1;
    }
}

void shadow_activate() {
    if (g_shadow.picker != Picker::none) {
        if (g_shadow.picker == Picker::engine) {
            g_shadow.musical_actor[static_cast<std::size_t>(g_shadow.slot)] = false;
        }
        g_shadow.picker = Picker::none;
        g_shadow.picker_master = false;
        return;
    }
    if (g_shadow.page == Page::perform) {
        if (g_shadow.focus_zone == 0) {
            g_shadow.picker = Picker::scene;
            g_shadow.picker_item = 0;
        }
        return;
    }
    if (g_shadow.page == Page::slot) {
        if (g_shadow.focus_zone == 1) {
            const int field = shadow_parameter();
            if (field == 1) {
                auto& musical =
                    g_shadow.musical_actor[static_cast<std::size_t>(g_shadow.slot)];
                musical = !musical;
            } else if (field == 2) {
                if (g_shadow.musical_actor[static_cast<std::size_t>(g_shadow.slot)]) {
                    g_shadow.focus_zone = 2;
                    g_shadow.actor_advanced_field = 0;
                } else {
                    g_shadow.picker = Picker::engine;
                    g_shadow.picker_group = 0;
                    g_shadow.picker_item = 0;
                }
            }
        } else if (g_shadow.focus_zone == 2 &&
                   g_shadow.actor_advanced_field == 9) {
            g_shadow.actor_modulator = (g_shadow.actor_modulator + 1) % 4;
        }
        return;
    }
    if (g_shadow.page == Page::effects &&
        g_shadow.focus_zone > 0 &&
        g_shadow.effect_field == 0) {
        shadow_parameter() = g_shadow.focus_zone - 1;
        g_shadow.picker = Picker::effect;
        g_shadow.picker_master = false;
        g_shadow.picker_group = 0;
        g_shadow.picker_item = 0;
        return;
    }
    if (g_shadow.page == Page::master &&
        g_shadow.focus_zone > 0 &&
        g_shadow.master_effect_field == 0) {
        g_shadow.master_effect = g_shadow.focus_zone - 1;
        g_shadow.picker = Picker::effect;
        g_shadow.picker_master = true;
        g_shadow.picker_group = 0;
        g_shadow.picker_item = 0;
    }
}

void shadow_arrow(int horizontal, int vertical) {
    if (g_shadow.picker != Picker::none) {
        shadow_move_picker(horizontal, vertical);
        return;
    }
    if (g_shadow.menu_open || g_shadow.help_open) return;

    if (g_shadow.page == Page::perform) {
        if (g_shadow.focus_zone == 1 && vertical != 0) {
            shadow_parameter() = (shadow_parameter() + vertical + 5) % 5;
        } else if (g_shadow.focus_zone == 2 && horizontal != 0) {
            g_shadow.slot = (g_shadow.slot + horizontal + 4) % 4;
        }
        return;
    }
    if (g_shadow.page == Page::slot) {
        if (g_shadow.focus_zone == 0 && horizontal != 0) {
            g_shadow.slot = (g_shadow.slot + horizontal + 4) % 4;
        } else if (g_shadow.focus_zone == 1 && vertical != 0) {
            shadow_parameter() = (shadow_parameter() + vertical + 10) % 10;
        } else if (g_shadow.focus_zone == 1 && horizontal != 0 &&
                   shadow_parameter() == 1) {
            g_shadow.musical_actor[static_cast<std::size_t>(g_shadow.slot)] =
                horizontal > 0;
        } else if (g_shadow.focus_zone == 2 && vertical != 0) {
            g_shadow.actor_advanced_field =
                (g_shadow.actor_advanced_field + vertical + 17) % 17;
        }
        return;
    }
    if (g_shadow.page == Page::effects) {
        if (g_shadow.focus_zone == 0 && horizontal != 0) {
            g_shadow.slot = (g_shadow.slot + horizontal + 4) % 4;
        } else if (g_shadow.focus_zone > 0 && vertical != 0) {
            g_shadow.effect_field =
                std::clamp(g_shadow.effect_field + vertical, 0, 3);
        }
        return;
    }
    if (g_shadow.page == Page::master) {
        if (g_shadow.focus_zone == 0 && vertical != 0) {
            shadow_parameter() = (shadow_parameter() + vertical + 2) % 2;
        } else if (g_shadow.focus_zone > 0 && vertical != 0) {
            g_shadow.master_effect_field =
                std::clamp(g_shadow.master_effect_field + vertical, 0, 3);
        }
        return;
    }
    if (g_shadow.focus_zone == 0) {
        if (horizontal != 0) {
            g_shadow.memory_slot = (g_shadow.memory_slot + horizontal + 8) % 8;
        }
        if (vertical != 0) {
            g_shadow.memory_slot =
                (g_shadow.memory_slot + vertical * 4 + 8) % 8;
        }
    } else if (g_shadow.focus_zone == 1 && vertical != 0) {
        g_shadow.memory_action =
            (g_shadow.memory_action + vertical + 3) % 3;
    } else if (g_shadow.focus_zone == 2 && vertical != 0) {
        g_shadow.memory_setting =
            (g_shadow.memory_setting + vertical + 3) % 3;
    }
}

void shadow_key(SDL_Keycode key) {
    if (g_shadow.picker != Picker::none) {
        if (key == SDLK_ESCAPE) shadow_go_back();
        else if (key == SDLK_LEFT) shadow_arrow(-1, 0);
        else if (key == SDLK_RIGHT) shadow_arrow(1, 0);
        else if (key == SDLK_UP) shadow_arrow(0, -1);
        else if (key == SDLK_DOWN) shadow_arrow(0, 1);
        else if (key == SDLK_RETURN || key == SDLK_SPACE) shadow_activate();
        return;
    }
    if (key == SDLK_ESCAPE) shadow_go_back();
    else if (key == SDLK_LEFT) shadow_arrow(-1, 0);
    else if (key == SDLK_RIGHT) shadow_arrow(1, 0);
    else if (key == SDLK_UP) shadow_arrow(0, -1);
    else if (key == SDLK_DOWN) shadow_arrow(0, 1);
    else if (key == SDLK_RETURN || key == SDLK_SPACE) shadow_activate();
    else if (key == SDLK_TAB) shadow_change_page(1);
    else if (key == SDLK_x) shadow_cycle_focus();
    else if (key == SDLK_h) g_shadow.help_open = !g_shadow.help_open;
    else if (key == SDLK_m) {
        g_shadow.menu_open = true;
        g_shadow.menu_item = 0;
    }
}

void queue_press(SDL_Keycode key, int repeat_count, Uint32 timestamp) {
    for (int index = 0; index < repeat_count; ++index) {
        shadow_key(key);
        queue_key(key, true, timestamp);
        queue_key(key, false, timestamp);
    }
}

void set_page(int target, Uint32 timestamp) {
    target = std::clamp(target, 0, 4);
    const int current = shadow_page_index(g_shadow.page);
    const int steps = (target - current + 5) % 5;
    queue_press(SDLK_TAB, steps, timestamp);
}

void set_focus(int target, Uint32 timestamp) {
    target = std::clamp(target, 0, focus_zone_count(g_shadow.page) - 1);
    const int count = focus_zone_count(g_shadow.page);
    const int steps = (target - g_shadow.focus_zone + count) % count;
    queue_press(SDLK_x, steps, timestamp);
}

void move_linear(int current, int target, Uint32 timestamp) {
    const int delta = target - current;
    if (delta < 0) queue_press(SDLK_UP, -delta, timestamp);
    else if (delta > 0) queue_press(SDLK_DOWN, delta, timestamp);
}

void set_slot_for_page(int target, Uint32 timestamp) {
    target = std::clamp(target, 0, 3);
    int required_focus = 0;
    if (g_shadow.page == Page::perform) required_focus = 2;
    set_focus(required_focus, timestamp);
    int delta = target - g_shadow.slot;
    if (delta > 2) delta -= 4;
    if (delta < -2) delta += 4;
    if (delta < 0) queue_press(SDLK_LEFT, -delta, timestamp);
    else if (delta > 0) queue_press(SDLK_RIGHT, delta, timestamp);
}

void set_basic_row(int row, Uint32 timestamp) {
    set_focus(1, timestamp);
    move_linear(shadow_parameter(), row, timestamp);
}

void set_advanced_row(int row, Uint32 timestamp) {
    set_focus(2, timestamp);
    move_linear(g_shadow.actor_advanced_field, row, timestamp);
}

void set_fx_slot(int slot, Uint32 timestamp) {
    set_focus(slot + 1, timestamp);
}

void set_fx_row(int row, Uint32 timestamp) {
    if (g_shadow.focus_zone == 0) set_focus(1, timestamp);
    move_linear(g_shadow.effect_field, row, timestamp);
}

void set_master_row(int row, Uint32 timestamp) {
    if (g_shadow.focus_zone == 0) set_focus(1, timestamp);
    move_linear(g_shadow.master_effect_field, row, timestamp);
}

void logical_touch_point(const SDL_TouchFingerEvent& touch, float& x, float& y) {
    x = touch.x * static_cast<float>(kAndroidUiWidth);
    y = touch.y * static_cast<float>(kAndroidUiHeight);
    if (g_renderer == nullptr) return;

    SDL_Window* window = SDL_GetWindowFromID(touch.windowID);
    if (window == nullptr) return;

    int window_width = 0;
    int window_height = 0;
    SDL_GetWindowSize(window, &window_width, &window_height);
    if (window_width <= 0 || window_height <= 0) return;

    const int window_x =
        static_cast<int>(std::lround(touch.x * static_cast<float>(window_width)));
    const int window_y =
        static_cast<int>(std::lround(touch.y * static_cast<float>(window_height)));
    float logical_x = x;
    float logical_y = y;
    SDL_RenderWindowToLogical(
        g_renderer, window_x, window_y, &logical_x, &logical_y);
    x = logical_x;
    y = logical_y;
}

TouchTarget hit_picker(float x, float y) {
    if (g_shadow.picker == Picker::scene) {
        for (int item = 0; item < static_cast<int>(kScenes.size()); ++item) {
            const int column = item / 5;
            const int row = item % 5;
            const SDL_Rect rect{92 + column * 164, 102 + row * 42, 154, 32};
            if (point_in(rect, x, y)) {
                return {TargetKind::picker_item, item, 0, false};
            }
        }
    } else if (g_shadow.picker == Picker::engine) {
        for (int group = 0;
             group < static_cast<int>(kEngineGroups.size());
             ++group) {
            const SDL_Rect rect{
                88 + (group % 3) * 108, 101 + (group / 3) * 21, 104, 20};
            if (point_in(rect, x, y)) {
                return {TargetKind::picker_group, group, 0, false};
            }
        }
        for (int item = 0; item < 4; ++item) {
            const SDL_Rect rect{92, 172 + item * 32, 328, 28};
            if (point_in(rect, x, y)) {
                return {TargetKind::picker_item, item, 0, false};
            }
        }
    } else if (g_shadow.picker == Picker::effect) {
        for (int group = 0; group < 2; ++group) {
            const SDL_Rect rect{88 + group * 164, 90, 154, 24};
            if (point_in(rect, x, y)) {
                return {TargetKind::picker_group, group, 0, false};
            }
        }
        const int count = effect_group_size(g_shadow.picker_group);
        const int rows = g_shadow.picker_group == 0 ? 8 : 5;
        for (int item = 0; item < count; ++item) {
            const int column = item / rows;
            const int row = item % rows;
            const SDL_Rect rect{92 + column * 164, 118 + row * 25, 154, 23};
            if (point_in(rect, x, y)) {
                return {TargetKind::picker_item, item, 0, false};
            }
        }
    }
    return {};
}

TouchTarget hit_content(float x, float y) {
    if (g_shadow.picker != Picker::none) return hit_picker(x, y);
    if (point_in(kFadeRect, x, y)) return {TargetKind::fade, 0, 0, false};

    if (y >= 24.0F && y < 46.0F) {
        for (int page = 0; page < 5; ++page) {
            const SDL_Rect rect{10 + page * 99, 24, 95, 22};
            if (point_in(rect, x, y)) {
                return {TargetKind::page, page, 0, false};
            }
        }
    }

    if (g_shadow.page == Page::perform) {
        if (point_in({16, 50, 480, 29}, x, y)) {
            return {TargetKind::place_landscape, 0, 0, false};
        }
        for (int row = 0; row < 5; ++row) {
            if (point_in({21, 82 + row * 28, 470, 25}, x, y)) {
                return {TargetKind::place_macro, row, 0, true};
            }
        }
        for (int actor = 0; actor < 4; ++actor) {
            const int actor_x = 22 + actor * 117;
            if (point_in({actor_x, 315, 109, 28}, x, y)) {
                return {TargetKind::place_actor_mute, actor, 0, false};
            }
            if (point_in({actor_x, 261, 109, 54}, x, y)) {
                return {TargetKind::place_actor, actor, 0, false};
            }
        }
    } else if (g_shadow.page == Page::slot) {
        for (int actor = 0; actor < 4; ++actor) {
            if (point_in({18 + actor * 119, 52, 111, 29}, x, y)) {
                return {TargetKind::actor_selector, actor, 0, false};
            }
        }
        if (point_in({18, 85, 236, 27}, x, y)) {
            return {TargetKind::actor_basic_tab, 0, 0, false};
        }
        if (point_in({258, 85, 236, 27}, x, y)) {
            return {TargetKind::actor_advanced_tab, 0, 0, false};
        }
        if (g_shadow.focus_zone != 2) {
            for (int row = 0; row < 10; ++row) {
                if (point_in({23, 116 + row * 21, 466, 21}, x, y)) {
                    return {
                        TargetKind::actor_basic_row, row, 0, row >= 3};
                }
            }
        } else {
            for (int field = 0; field < 17; ++field) {
                const bool right = field >= 9;
                const int row = right ? field - 9 : field;
                const int field_x = right ? 258 : 23;
                if (point_in({field_x, 116 + row * 24, 231, 23}, x, y)) {
                    const bool action = field == 4 || field == 9 || field == 10;
                    return {
                        TargetKind::actor_advanced_row, field, 0, !action};
                }
            }
        }
    } else if (g_shadow.page == Page::effects) {
        for (int actor = 0; actor < 4; ++actor) {
            if (point_in({18 + actor * 119, 52, 111, 29}, x, y)) {
                return {TargetKind::fx_actor, actor, 0, false};
            }
        }
        for (int slot = 0; slot < 4; ++slot) {
            if (point_in({18 + slot * 119, 85, 111, 42}, x, y)) {
                return {TargetKind::fx_slot, slot, 0, false};
            }
        }
        for (int row = 0; row < 4; ++row) {
            if (point_in({232, 166 + row * 38, 252, 34}, x, y)) {
                return {TargetKind::fx_row, row, 0, row > 0};
            }
        }
    } else if (g_shadow.page == Page::master) {
        for (int row = 0; row < 2; ++row) {
            if (point_in({22, 119 + row * 22, 466, 21}, x, y)) {
                return {
                    TargetKind::master_signal_row, row, 0, true};
            }
        }
        for (int slot = 0; slot < 4; ++slot) {
            if (point_in({18 + slot * 119, 160, 111, 41}, x, y)) {
                return {TargetKind::master_fx_slot, slot, 0, false};
            }
        }
        for (int row = 0; row < 4; ++row) {
            if (point_in({214, 208 + row * 28, 272, 26}, x, y)) {
                return {TargetKind::master_fx_row, row, 0, row > 0};
            }
        }
    } else {
        for (int slot = 0; slot < 8; ++slot) {
            const int column = slot % 4;
            const int row = slot / 4;
            if (point_in({22 + column * 118, 80 + row * 49, 106, 43}, x, y)) {
                return {TargetKind::memory_slot, slot, 0, false};
            }
        }
        for (int action = 0; action < 3; ++action) {
            if (point_in({22, 198 + action * 28, 260, 26}, x, y)) {
                return {TargetKind::memory_action, action, 0, false};
            }
        }
        for (int setting = 0; setting < 3; ++setting) {
            if (point_in({302, 198 + setting * 28, 188, 26}, x, y)) {
                return {
                    TargetKind::memory_setting, setting, 0, setting > 0};
            }
        }
    }
    return {};
}

void focus_target(const TouchTarget& target, Uint32 timestamp) {
    switch (target.kind) {
    case TargetKind::page:
        set_page(target.index, timestamp);
        break;
    case TargetKind::place_landscape:
        set_focus(0, timestamp);
        break;
    case TargetKind::place_macro:
        set_focus(1, timestamp);
        move_linear(shadow_parameter(), target.index, timestamp);
        break;
    case TargetKind::place_actor:
    case TargetKind::place_actor_mute:
        set_slot_for_page(target.index, timestamp);
        break;
    case TargetKind::actor_selector:
        set_slot_for_page(target.index, timestamp);
        break;
    case TargetKind::actor_basic_tab:
        set_focus(1, timestamp);
        break;
    case TargetKind::actor_advanced_tab:
        set_focus(2, timestamp);
        break;
    case TargetKind::actor_basic_row:
        set_basic_row(target.index, timestamp);
        break;
    case TargetKind::actor_advanced_row:
        set_advanced_row(target.index, timestamp);
        break;
    case TargetKind::fx_actor:
        set_focus(0, timestamp);
        set_slot_for_page(target.index, timestamp);
        break;
    case TargetKind::fx_slot:
        set_fx_slot(target.index, timestamp);
        break;
    case TargetKind::fx_row:
        set_fx_row(target.index, timestamp);
        break;
    case TargetKind::master_signal_row:
        set_focus(0, timestamp);
        move_linear(shadow_parameter(), target.index, timestamp);
        break;
    case TargetKind::master_fx_slot:
        set_focus(target.index + 1, timestamp);
        break;
    case TargetKind::master_fx_row:
        set_master_row(target.index, timestamp);
        break;
    case TargetKind::memory_slot:
        set_focus(0, timestamp);
        {
            const int current_column = g_shadow.memory_slot % 4;
            const int current_row = g_shadow.memory_slot / 4;
            const int target_column = target.index % 4;
            const int target_row = target.index / 4;
            const int horizontal = target_column - current_column;
            const int vertical = target_row - current_row;
            if (horizontal < 0) queue_press(SDLK_LEFT, -horizontal, timestamp);
            else if (horizontal > 0) queue_press(SDLK_RIGHT, horizontal, timestamp);
            if (vertical < 0) queue_press(SDLK_UP, -vertical, timestamp);
            else if (vertical > 0) queue_press(SDLK_DOWN, vertical, timestamp);
        }
        break;
    case TargetKind::memory_action:
        set_focus(1, timestamp);
        move_linear(g_shadow.memory_action, target.index, timestamp);
        break;
    case TargetKind::memory_setting:
        set_focus(2, timestamp);
        move_linear(g_shadow.memory_setting, target.index, timestamp);
        break;
    case TargetKind::picker_group:
        if (g_shadow.picker == Picker::engine) {
            int delta = target.index - g_shadow.picker_group;
            const int count = static_cast<int>(kEngineGroups.size());
            if (delta > count / 2) delta -= count;
            if (delta < -count / 2) delta += count;
            if (delta < 0) queue_press(SDLK_LEFT, -delta, timestamp);
            else if (delta > 0) queue_press(SDLK_RIGHT, delta, timestamp);
        } else if (g_shadow.picker == Picker::effect) {
            const int delta = target.index - g_shadow.picker_group;
            if (delta < 0) queue_press(SDLK_LEFT, -delta, timestamp);
            else if (delta > 0) queue_press(SDLK_RIGHT, delta, timestamp);
        }
        break;
    case TargetKind::picker_item:
        if (g_shadow.picker == Picker::scene) {
            const int current_row = g_shadow.picker_item % 5;
            const int current_column = g_shadow.picker_item / 5;
            const int target_row = target.index % 5;
            const int target_column = target.index / 5;
            const int horizontal = target_column - current_column;
            const int vertical = target_row - current_row;
            if (horizontal < 0) queue_press(SDLK_LEFT, -horizontal, timestamp);
            else if (horizontal > 0) queue_press(SDLK_RIGHT, horizontal, timestamp);
            if (vertical < 0) queue_press(SDLK_UP, -vertical, timestamp);
            else if (vertical > 0) queue_press(SDLK_DOWN, vertical, timestamp);
        } else if (g_shadow.picker == Picker::engine) {
            move_linear(g_shadow.picker_item, target.index, timestamp);
        } else if (g_shadow.picker == Picker::effect) {
            const int rows = g_shadow.picker_group == 0 ? 8 : 5;
            const int current_row = g_shadow.picker_item % rows;
            const int current_column = g_shadow.picker_item / rows;
            const int target_row = target.index % rows;
            const int target_column = target.index / rows;
            const int horizontal = target_column - current_column;
            const int vertical = target_row - current_row;
            if (horizontal < 0) queue_press(SDLK_LEFT, -horizontal, timestamp);
            else if (horizontal > 0) queue_press(SDLK_RIGHT, horizontal, timestamp);
            if (vertical < 0) queue_press(SDLK_UP, -vertical, timestamp);
            else if (vertical > 0) queue_press(SDLK_DOWN, vertical, timestamp);
        }
        break;
    default:
        break;
    }
}

void activate_target(const TouchTarget& target, Uint32 timestamp) {
    switch (target.kind) {
    case TargetKind::fade:
        queue_press(SDLK_f, 1, timestamp);
        break;
    case TargetKind::place_landscape:
    case TargetKind::place_actor_mute:
        queue_press(SDLK_RETURN, 1, timestamp);
        break;
    case TargetKind::actor_basic_row:
        if (target.index <= 2) queue_press(SDLK_RETURN, 1, timestamp);
        break;
    case TargetKind::actor_advanced_row:
        if (target.index == 4 || target.index == 9 || target.index == 10) {
            queue_press(SDLK_RETURN, 1, timestamp);
        }
        break;
    case TargetKind::fx_row:
    case TargetKind::master_fx_row:
        if (target.index == 0) queue_press(SDLK_RETURN, 1, timestamp);
        break;
    case TargetKind::memory_action:
        queue_press(SDLK_RETURN, 1, timestamp);
        break;
    case TargetKind::memory_setting:
        if (target.index == 0) queue_press(SDLK_RIGHT, 1, timestamp);
        break;
    case TargetKind::picker_item:
        queue_press(SDLK_RETURN, 1, timestamp);
        break;
    default:
        break;
    }
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
    g_touch_target = hit_content(x, y);
    focus_target(g_touch_target, touch.timestamp);
}

void handle_finger_motion(const SDL_TouchFingerEvent& touch) {
    if (!g_finger_active || touch.fingerId != g_active_finger) return;

    float x = 0.0F;
    float y = 0.0F;
    logical_touch_point(touch, x, y);
    const float dx = x - g_touch_last_x;
    const float dy = y - g_touch_last_y;

    if (std::abs(dx) >= static_cast<float>(kDragStep) ||
        std::abs(dy) >= static_cast<float>(kDragStep)) {
        g_touch_moved = true;
    }
    if (g_touch_target.adjustable &&
        std::abs(dx) >= static_cast<float>(kDragStep)) {
        const SDL_Keycode key = dx > 0.0F ? SDLK_RIGHT : SDLK_LEFT;
        const int count = std::clamp(
            static_cast<int>(std::abs(dx)) / kDragStep, 1, 8);
        queue_press(key, count, touch.timestamp);
        g_touch_last_x = x;
        g_touch_last_y = y;
    }
}

void handle_finger_up(const SDL_TouchFingerEvent& touch) {
    if (!g_finger_active || touch.fingerId != g_active_finger) return;
    if (!g_touch_moved) activate_target(g_touch_target, touch.timestamp);
    g_finger_active = false;
    g_touch_moved = false;
    g_touch_target = {};
}

int next_power_of_two(int value) {
    int result = 1;
    while (result < value && result < 4096) result <<= 1;
    return std::clamp(result, 256, 4096);
}

void parse_android_arguments(int argc, char** argv) {
    constexpr std::string_view rate_prefix{"--android-audio-rate="};
    constexpr std::string_view burst_prefix{"--android-audio-burst="};
    for (int index = 1; index < argc; ++index) {
        const std::string argument{argv[index] != nullptr ? argv[index] : ""};
        if (argument.rfind(rate_prefix, 0) == 0) {
            g_android_audio_rate =
                std::clamp(std::atoi(argument.c_str() + rate_prefix.size()),
                    8'000, 192'000);
        } else if (argument.rfind(burst_prefix, 0) == 0) {
            g_android_audio_burst =
                std::clamp(std::atoi(argument.c_str() + burst_prefix.size()),
                    64, 2'048);
        }
    }
}

void draw_touch_fade(SDL_Renderer* renderer) {
    const SDL_Color idle{28, 23, 38, 238};
    const SDL_Color border{105, 92, 118, 255};
    const SDL_Color text{238, 226, 197, 255};
    fill(renderer, kFadeRect, idle);
    outline(renderer, kFadeRect, border);
    const std::string_view label{"FADE"};
    cd::ui::draw_text(
        renderer,
        kFadeRect.x + (kFadeRect.w - cd::ui::text_width(label)) / 2,
        kFadeRect.y + 6,
        label,
        text);
}

} // namespace

int cursed_android_init(Uint32 flags) {
    const int result = SDL_Init(flags);
    if (result == 0) prepare_android_data();
    return result;
}

SDL_AudioDeviceID cursed_android_open_audio_device(
    const char* device,
    int iscapture,
    const SDL_AudioSpec* desired,
    SDL_AudioSpec* obtained,
    int allowed_changes) {
    if (iscapture != 0 || desired == nullptr) {
        return SDL_OpenAudioDevice(
            device, iscapture, desired, obtained, allowed_changes);
    }

    SDL_AudioSpec stable = *desired;
    stable.freq = g_android_audio_rate;
    const int target_frames =
        std::max(2'048, std::max(g_android_audio_burst * 6,
            static_cast<int>(stable.samples)));
    stable.samples =
        static_cast<Uint16>(next_power_of_two(target_frames));

    SDL_Log(
        "Cursed Drone Android audio request: %d Hz, %u frames "
        "(device burst %d)",
        stable.freq,
        static_cast<unsigned>(stable.samples),
        g_android_audio_burst);

    return SDL_OpenAudioDevice(
        device,
        iscapture,
        &stable,
        obtained,
        allowed_changes | SDL_AUDIO_ALLOW_FREQUENCY_CHANGE |
            SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
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
            if (raw.type == SDL_KEYDOWN && raw.key.repeat == 0) {
                shadow_key(raw.key.keysym.sym);
            }
            *event = raw;
            return 1;
        }
        if (pop_synthetic(event)) return 1;
    }
    return pop_synthetic(event) ? 1 : 0;
}

int cursed_android_render_set_logical_size(
    SDL_Renderer* renderer, int, int) {
    g_renderer = renderer;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    return SDL_RenderSetLogicalSize(
        renderer, kAndroidUiWidth, kAndroidUiHeight);
}

void cursed_android_render_present(SDL_Renderer* renderer) {
    draw_touch_fade(renderer);
    SDL_RenderPresent(renderer);
}

extern "C" int SDL_main(int argc, char** argv) {
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    SDL_SetHint(SDL_HINT_AUDIO_RESAMPLING_MODE, "medium");
    setenv("CURSED_DRONE_HANDHELD", "1", 0);
    setenv("CURSED_DRONE_TOUCH", "1", 1);
    parse_android_arguments(argc, argv);
    return cursed_drone_main_impl(argc, argv);
}

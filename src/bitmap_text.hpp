// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <SDL.h>

#include <string_view>

namespace cursed_drone::ui {

void draw_text(
    SDL_Renderer* renderer,
    int x,
    int y,
    std::string_view text,
    SDL_Color color,
    int scale = 1);

[[nodiscard]] int text_width(std::string_view text, int scale = 1) noexcept;

} // namespace cursed_drone::ui

// SPDX-License-Identifier: GPL-3.0-or-later
#include "bitmap_text.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace cursed_drone::ui {
namespace {

constexpr std::uint8_t kFontData[] = {
#include "../third_party/font512/font_data.inc"
};
static_assert(std::size(kFontData) == 512U * 8U);

constexpr std::array<std::uint16_t, 32> kCyrillicUpper{
    0x041, 0x180, 0x042, 0x156, 0x184, 0x045, 0x186, 0x033,
    0x18A, 0x18C, 0x04B, 0x18E, 0x04D, 0x048, 0x04F, 0x16B,
    0x050, 0x043, 0x054, 0x193, 0x177, 0x058, 0x194, 0x196,
    0x198, 0x19A, 0x19C, 0x19E, 0x1A0, 0x1A2, 0x1A4, 0x1A6,
};

constexpr std::array<std::uint16_t, 32> kCyrillicLower{
    0x061, 0x181, 0x182, 0x183, 0x185, 0x065, 0x187, 0x189,
    0x18B, 0x18D, 0x164, 0x18F, 0x190, 0x191, 0x06F, 0x16C,
    0x070, 0x063, 0x192, 0x079, 0x178, 0x078, 0x195, 0x197,
    0x199, 0x19B, 0x19D, 0x19F, 0x1A1, 0x1A3, 0x1A5, 0x1A7,
};

std::uint32_t next_codepoint(std::string_view text, std::size_t& offset) noexcept {
    const auto first = static_cast<std::uint8_t>(text[offset++]);
    if (first < 0x80U) {
        return first;
    }
    if ((first & 0xE0U) == 0xC0U && offset < text.size()) {
        const auto second = static_cast<std::uint8_t>(text[offset++]);
        return static_cast<std::uint32_t>(first & 0x1FU) << 6U |
            static_cast<std::uint32_t>(second & 0x3FU);
    }
    if ((first & 0xF0U) == 0xE0U && offset + 1U < text.size()) {
        const auto second = static_cast<std::uint8_t>(text[offset++]);
        const auto third = static_cast<std::uint8_t>(text[offset++]);
        return static_cast<std::uint32_t>(first & 0x0FU) << 12U |
            static_cast<std::uint32_t>(second & 0x3FU) << 6U |
            static_cast<std::uint32_t>(third & 0x3FU);
    }
    while (offset < text.size() && (static_cast<std::uint8_t>(text[offset]) & 0xC0U) == 0x80U) {
        ++offset;
    }
    return static_cast<std::uint32_t>('?');
}

std::uint16_t glyph_index(std::uint32_t codepoint) noexcept {
    if (codepoint < 128U) {
        return static_cast<std::uint16_t>(codepoint);
    }
    if (codepoint == 0x0401U) {
        return 0x0D3U;
    }
    if (codepoint == 0x0451U) {
        return 0x089U;
    }
    if (codepoint >= 0x0410U && codepoint <= 0x042FU) {
        return kCyrillicUpper[codepoint - 0x0410U];
    }
    if (codepoint >= 0x0430U && codepoint <= 0x044FU) {
        return kCyrillicLower[codepoint - 0x0430U];
    }
    return static_cast<std::uint16_t>('?');
}

} // namespace

void draw_text(SDL_Renderer* renderer, int x, int y, std::string_view text, SDL_Color color, int scale) {
    scale = scale < 1 ? 1 : scale;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    std::size_t offset = 0U;
    int cursor = x;
    while (offset < text.size()) {
        const auto glyph = glyph_index(next_codepoint(text, offset));
        const std::size_t base = static_cast<std::size_t>(glyph) * 8U;
        for (int row = 0; row < 8; ++row) {
            const auto pixels = kFontData[base + static_cast<std::size_t>(row)];
            for (int column = 0; column < 8; ++column) {
                if ((pixels & static_cast<std::uint8_t>(0x80U >> column)) != 0U) {
                    SDL_Rect pixel{cursor + column * scale, y + row * scale, scale, scale};
                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
        }
        cursor += 8 * scale;
    }
}

int text_width(std::string_view text, int scale) noexcept {
    std::size_t offset = 0U;
    int glyphs = 0;
    while (offset < text.size()) {
        static_cast<void>(next_codepoint(text, offset));
        ++glyphs;
    }
    return glyphs * 8 * (scale < 1 ? 1 : scale);
}

} // namespace cursed_drone::ui

#include "approved_ui_font_data.hpp"

struct AFontTexture {
    SDL_Texture* texture{nullptr};
    int width{0};
    int height{0};
    float advance{0.0F};
};

SDL_Renderer* g_a_font_renderer = nullptr;
std::unordered_map<std::uint32_t, AFontTexture> g_a_font_textures{};

int a_font_pixel_size(int scale) noexcept {
    switch (std::clamp(scale, 1, 5)) {
    case 1: return 10;
    case 2: return 16;
    case 3: return 22;
    case 4: return 28;
    default: return 34;
    }
}

std::uint32_t a_utf8_next(std::string_view text, std::size_t& index) noexcept {
    if (index >= text.size()) return 0U;
    const auto first = static_cast<unsigned char>(text[index++]);
    if (first < 0x80U) return first;
    if ((first & 0xE0U) == 0xC0U && index < text.size()) {
        const auto second = static_cast<unsigned char>(text[index++]);
        return static_cast<std::uint32_t>((first & 0x1FU) << 6U) |
            static_cast<std::uint32_t>(second & 0x3FU);
    }
    if ((first & 0xF0U) == 0xE0U && index + 1U < text.size()) {
        const auto second = static_cast<unsigned char>(text[index++]);
        const auto third = static_cast<unsigned char>(text[index++]);
        return static_cast<std::uint32_t>((first & 0x0FU) << 12U) |
            static_cast<std::uint32_t>((second & 0x3FU) << 6U) |
            static_cast<std::uint32_t>(third & 0x3FU);
    }
    if ((first & 0xF8U) == 0xF0U && index + 2U < text.size()) {
        const auto second = static_cast<unsigned char>(text[index++]);
        const auto third = static_cast<unsigned char>(text[index++]);
        const auto fourth = static_cast<unsigned char>(text[index++]);
        return static_cast<std::uint32_t>((first & 0x07U) << 18U) |
            static_cast<std::uint32_t>((second & 0x3FU) << 12U) |
            static_cast<std::uint32_t>((third & 0x3FU) << 6U) |
            static_cast<std::uint32_t>(fourth & 0x3FU);
    }
    return static_cast<std::uint32_t>('?');
}

const AFontGlyphData* a_font_glyph_data(std::uint32_t codepoint) noexcept {
    const auto found = std::lower_bound(kAFontGlyphs.begin(), kAFontGlyphs.end(),
        codepoint, [](const AFontGlyphData& glyph, std::uint32_t value) {
            return glyph.codepoint < value;
        });
    if (found != kAFontGlyphs.end() && found->codepoint == codepoint) {
        return &*found;
    }
    return a_font_glyph_data(static_cast<std::uint32_t>('?'));
}

void a_font_clear_cache() {
    for (auto& [codepoint, glyph] : g_a_font_textures) {
        (void)codepoint;
        if (glyph.texture != nullptr) SDL_DestroyTexture(glyph.texture);
    }
    g_a_font_textures.clear();
}

AFontTexture& a_font_texture(SDL_Renderer* renderer, std::uint32_t codepoint) {
    if (renderer != g_a_font_renderer) {
        a_font_clear_cache();
        g_a_font_renderer = renderer;
    }
    auto existing = g_a_font_textures.find(codepoint);
    if (existing != g_a_font_textures.end()) return existing->second;

    const AFontGlyphData* data = a_font_glyph_data(codepoint);
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(data->width) *
        static_cast<std::size_t>(data->height) * 4U, 0U);
    std::size_t pixel = 0U;
    const std::size_t end = static_cast<std::size_t>(data->offset + data->length);
    for (std::size_t cursor = data->offset; cursor + 1U < end; cursor += 2U) {
        const std::uint8_t count = kAFontRle[cursor];
        const std::uint8_t alpha = kAFontRle[cursor + 1U];
        for (std::uint8_t repeat = 0U; repeat < count; ++repeat) {
            if (pixel >= static_cast<std::size_t>(data->width) * data->height) break;
            const std::size_t offset = pixel * 4U;
            rgba[offset] = 255U;
            rgba[offset + 1U] = 255U;
            rgba[offset + 2U] = 255U;
            rgba[offset + 3U] = alpha;
            ++pixel;
        }
    }

    AFontTexture glyph{};
    glyph.width = static_cast<int>(data->width);
    glyph.height = static_cast<int>(data->height);
    glyph.advance = static_cast<float>(data->advance64) / 64.0F;
    glyph.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STATIC, glyph.width, glyph.height);
    if (glyph.texture != nullptr) {
        SDL_UpdateTexture(glyph.texture, nullptr, rgba.data(), glyph.width * 4);
        SDL_SetTextureBlendMode(glyph.texture, SDL_BLENDMODE_BLEND);
    }
    return g_a_font_textures.emplace(codepoint, glyph).first->second;
}

int a_text_width(std::string_view value, int scale = 1) noexcept {
    const float factor = static_cast<float>(a_font_pixel_size(scale)) /
        static_cast<float>(kAFontBaseSize);
    float width = 0.0F;
    std::size_t cursor = 0U;
    while (cursor < value.size()) {
        const std::uint32_t codepoint = a_utf8_next(value, cursor);
        const AFontGlyphData* glyph = a_font_glyph_data(codepoint);
        width += static_cast<float>(glyph->advance64) / 64.0F * factor;
    }
    return static_cast<int>(std::lround(width));
}

void a_text(SDL_Renderer* renderer, int x, int y, std::string_view value,
    SDL_Color color, int scale = 1) {
    const int pixels = a_font_pixel_size(scale);
    const float factor = static_cast<float>(pixels) /
        static_cast<float>(kAFontBaseSize);
    float pen_x = static_cast<float>(x);
    std::size_t cursor = 0U;
    while (cursor < value.size()) {
        const std::uint32_t codepoint = a_utf8_next(value, cursor);
        AFontTexture& glyph = a_font_texture(renderer, codepoint);
        if (glyph.texture != nullptr) {
            SDL_SetTextureColorMod(glyph.texture, color.r, color.g, color.b);
            SDL_SetTextureAlphaMod(glyph.texture, color.a);
            SDL_FRect destination{pen_x, static_cast<float>(y),
                static_cast<float>(glyph.width) * factor,
                static_cast<float>(glyph.height) * factor};
            SDL_RenderCopyF(renderer, glyph.texture, nullptr, &destination);
        }
        pen_x += glyph.advance * factor;
    }
}

void a_centered_text(SDL_Renderer* renderer, const SDL_Rect& rect,
    std::string value, SDL_Color color, int scale) {
    while (value.size() > 3U && a_text_width(value, scale) > rect.w - 16) {
        value.pop_back();
    }
    const int width = a_text_width(value, scale);
    const int height = a_font_pixel_size(scale) + 3;
    a_text(renderer, rect.x + (rect.w - width) / 2,
        rect.y + (rect.h - height) / 2 - 1, value, color, scale);
}

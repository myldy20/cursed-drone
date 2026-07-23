#!/usr/bin/env python3
from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "android/app/src/main/cpp"

# Match the exact typeface and scale used by the approved 2992x1344 mockup.
font_path = Path("/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf")
if not font_path.is_file():
    raise SystemExit(f"missing deterministic build font: {font_path}")
font = ImageFont.truetype(str(font_path), 40)
codepoints = sorted(
    set(range(32, 127))
    | set(range(0x0400, 0x0460))
    | {0x2013, 0x2014, 0x2022, 0x2212, 0x25B2, 0x25BC, 0x2190, 0x2192}
)


def encode_rle(data: bytes) -> list[int]:
    if not data:
        return []
    result: list[int] = []
    value = data[0]
    count = 1
    for item in data[1:]:
        if item == value and count < 255:
            count += 1
        else:
            result.extend((count, value))
            value = item
            count = 1
    result.extend((count, value))
    return result


glyphs: list[tuple[int, int, int, int, int, int]] = []
rle: list[int] = []
for codepoint in codepoints:
    character = chr(codepoint)
    advance = max(1.0, float(font.getlength(character)))
    bbox = font.getbbox(character)
    ink_width = max(1, bbox[2] - bbox[0])
    width = max(3, int(math.ceil(max(advance, ink_width))) + 6)
    height = 52
    image = Image.new("L", (width, height), 0)
    ImageDraw.Draw(image).text((3 - bbox[0], 4 - bbox[1]), character, font=font, fill=255)
    encoded = encode_rle(image.tobytes())
    offset = len(rle)
    rle.extend(encoded)
    glyphs.append((codepoint, width, height, int(round(advance * 64.0)), offset, len(encoded)))

out = [
    "// Generated raster glyph atlas from DejaVu Sans Mono Bold.",
    "// The source font is not embedded; only pre-rasterized alpha masks are stored.",
    "#pragma once",
    "",
    "struct AFontGlyphData {",
    "    std::uint32_t codepoint;",
    "    std::uint16_t width;",
    "    std::uint16_t height;",
    "    std::uint16_t advance64;",
    "    std::uint32_t offset;",
    "    std::uint32_t length;",
    "};",
    "constexpr int kAFontBaseSize = 40;",
    "constexpr int kAFontLineHeight = 52;",
    f"constexpr std::array<AFontGlyphData, {len(glyphs)}> kAFontGlyphs{{{{",
]
for glyph in glyphs:
    out.append("    {" + ", ".join(f"{value}U" for value in glyph) + "},")
out.append("}};")
out.append(f"constexpr std::array<std::uint8_t, {len(rle)}> kAFontRle{{{{")
for index in range(0, len(rle), 24):
    out.append("    " + ", ".join(str(value) for value in rle[index:index + 24]) + ",")
out.extend(("}};", ""))
(CPP / "approved_ui_font_data.hpp").write_text("\n".join(out), encoding="utf-8")

font_renderer = CPP / "approved_ui_font.inc"
source = font_renderer.read_text(encoding="utf-8")
source = source.replace("case 3: return 22;", "case 3: return 24;")
source = source.replace("SDL_PIXELFORMAT_RGBA8888", "SDL_PIXELFORMAT_RGBA32")
font_renderer.write_text(source, encoding="utf-8")

place = CPP / "approved_ui_place_exact.inc"
source = place.read_text(encoding="utf-8")
replacements = {
    "const int label_w = std::clamp(rect.w / 4, 135, 220);":
        "const int label_w = 192;",
    "SDL_Rect bar{rect.x + label_w + 26, rect.y + 15,\n        rect.w - label_w - 34, 9};":
        "SDL_Rect bar{rect.x + label_w + 26, rect.y + 12,\n        rect.w - label_w - 34, 9};",
    "a_text(renderer, bar.x, rect.y + rect.h - 16,":
        "a_text(renderer, bar.x, rect.y + 28,",
    "rect.y + rect.h - 16, high, aMutedCream, label_scale);":
        "rect.y + 28, high, aMutedCream, label_scale);",
    "a_hline(renderer, rect.x, rect.y + rect.h - 1, rect.w, aBorderSoft);":
        "a_hline(renderer, rect.x, rect.y + 48, rect.w, aBorderSoft);",
    "a_text(renderer, 28, 11, \"CURSED DRONE\", aCream, scale + 2);":
        "a_text(renderer, 27, 20, \"CURSED DRONE\", aCream, scale + 2);",
    "a_text(renderer, 230, 17,": "a_text(renderer, 232, 27,",
    "a_text(renderer, rect.x + 12, rect.y + 8,":
        "a_text(renderer, rect.x + 11, rect.y + 10,",
    "a_text(renderer, rect.x + 38, rect.y + 8,":
        "a_text(renderer, rect.x + 34, rect.y + 10,",
    "const int signal_y = rect.y + 42;": "const int signal_y = rect.y + 36;",
    "signal_y + 13,": "signal_y + 17,",
    "const int level_y = signal_y + 35;": "const int level_y = rect.y + 74;",
    "SDL_Rect mute{rect.x + 11, rect.y + rect.h - 34,\n        rect.w - 22, 24};":
        "SDL_Rect mute{rect.x + 11, rect.y + rect.h - 33,\n        rect.w - 22, 22};",
    "void a_place(SDL_Renderer* renderer, cd::Session& session, UiState& state,\n    const cd::AudioTelemetry& telemetry, SDL_Rect area, int scale) {\n    a_card(renderer, area, aPanel2, aBorderSoft, 10, false);":
        "void a_place(SDL_Renderer* renderer, cd::Session& session, UiState& state,\n"
        "    const cd::AudioTelemetry& telemetry, SDL_Rect area, int scale) {\n"
        "    // Exact Pixel 8 Pro reference: 54 px sides and 42 px bottom.\n"
        "    area.x -= 1;\n"
        "    area.w += 2;\n"
        "    area.h += 7;\n"
        "    a_card(renderer, area, aPanel2, aBorderSoft, 10, false);",
    "const int gap = 10;": "const int gap = 12;",
    "const int start = macros.y + 42;\n    const int row_h = (macros.h - 52) / 5;":
        "const int start = macros.y + 39;\n    constexpr int row_h = 58;",
    "const int card_gap = 8;\n    const int cards_top = actors.y + 42;\n"
    "    const int card_w = (actors.w - 24 - card_gap) / 2;\n"
    "    const int card_h = (actors.y + actors.h - cards_top - 11 - card_gap) / 2;":
        "const int card_gap = 9;\n    const int cards_top = actors.y + 35;\n"
        "    const int card_w = (actors.w - 27 - card_gap) / 2;\n"
        "    const int card_h = (actors.y + actors.h - cards_top - 9 - card_gap) / 2;",
    "{actors.x + 11 + (i % 2) * (card_w + card_gap),":
        "{actors.x + 13 + (i % 2) * (card_w + card_gap),",
}
for before, after in replacements.items():
    if before not in source:
        raise SystemExit(f"expected Place fragment not found: {before[:80]!r}")
    source = source.replace(before, after, 1)
place.write_text(source, encoding="utf-8")

print(f"refined exact Place: glyphs={len(glyphs)} rle={len(rle)}")

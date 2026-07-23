#!/usr/bin/env python3
from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "android/app/src/main/cpp"

# Match the exact typeface used by the approved 2992x1344 mockup.
font_path = Path("/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf")
if not font_path.is_file():
    raise SystemExit(f"missing deterministic build font: {font_path}")
font = ImageFont.truetype(str(font_path), 40)
ascent, descent = font.getmetrics()
glyph_height = ascent + descent + 4
baseline = ascent + 2
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
    bbox = font.getbbox(character, anchor="ls")
    left_padding = max(3, 3 - bbox[0])
    ink_right = left_padding + bbox[2]
    width = max(3, int(math.ceil(max(advance + 6, ink_right + 3))))
    image = Image.new("L", (width, glyph_height), 0)
    # Every glyph is drawn against one shared baseline. This is critical for
    # Cyrillic labels such as ФЕЙД and for punctuation in version strings.
    ImageDraw.Draw(image).text(
        (left_padding, baseline), character, font=font, fill=255, anchor="ls"
    )
    encoded = encode_rle(image.tobytes())
    offset = len(rle)
    rle.extend(encoded)
    glyphs.append(
        (codepoint, width, glyph_height, int(round(advance * 64.0)), offset, len(encoded))
    )

out = [
    "// Generated raster glyph atlas from DejaVu Sans Mono Bold.",
    "// All glyphs share one baseline; the source font file is not embedded.",
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
    f"constexpr int kAFontLineHeight = {glyph_height};",
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

place_path = CPP / "approved_ui_place_exact.inc"
place = place_path.read_text(encoding="utf-8")
footer_start = "    const std::string footer = ru(session) ?\n"
start = place.find(footer_start)
if start < 0:
    raise SystemExit("Place footer block not found")
end_marker = "        footer, aMutedCream, scale);\n"
end = place.find(end_marker, start)
if end < 0:
    raise SystemExit("Place footer end not found")
end += len(end_marker)
place = place[:start] + place[end:]
place_path.write_text(place, encoding="utf-8")

print(
    f"refined exact Place v2: glyphs={len(glyphs)} rle={len(rle)} "
    f"baseline={baseline} height={glyph_height}; footer removed"
)

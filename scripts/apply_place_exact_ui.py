#!/usr/bin/env python3
from __future__ import annotations

import base64
import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
STAGING = ROOT / ".staging/place-ui"
CPP = ROOT / "android/app/src/main/cpp"


def decode_payload(names: list[str], relative_target: str) -> Path:
    encoded = "".join((STAGING / name).read_text(encoding="ascii").strip() for name in names)
    target = ROOT / relative_target
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(base64.b64decode(encoded, validate=True))
    return target


decode_payload(["font.b64"], "android/app/src/main/cpp/approved_ui_font.inc")
exact_path = decode_payload(
    ["place-exact.00.b64", "place-exact.01.b64"],
    "android/app/src/main/cpp/approved_ui_place_exact.inc",
)
exact_path.write_text(
    exact_path.read_text(encoding="utf-8").replace("aBorderDim", "aBorderSoft"),
    encoding="utf-8",
)
decode_payload(["snapshots.b64"], "tools/android_ui_snapshots.cpp")

# Generate deterministic alpha-mask glyph data. No font file is shipped in the app.
font_path = Path("/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf")
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
    "// Generated raster glyph atlas from DejaVu Sans Condensed Bold.",
    "// The source font is not embedded; only pre-rasterized alpha masks are stored.",
    "#pragma once",
    "#include <SDL.h>",
    "#include <algorithm>",
    "#include <array>",
    "#include <cstdint>",
    "#include <string_view>",
    "#include <unordered_map>",
    "#include <vector>",
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
    out.append("    " + ", ".join(str(value) for value in rle[index : index + 24]) + ",")
out.extend(("}};", ""))
(CPP / "approved_ui_font_data.hpp").write_text("\n".join(out), encoding="utf-8")

# Keep the old renderer available for the unfinished pages, but route every screen
# through the new anti-aliased typography and replace the shared header/Place components.
primitives_path = CPP / "approved_ui_primitives.inc"
source = primitives_path.read_text(encoding="utf-8")
prefix = """#include \"approved_ui_font.inc\"
#define text a_text
#define centered_text a_centered_text
#define a_header a_header_legacy
#define a_actor_card a_actor_card_legacy
#define a_place a_place_legacy

"""
if '#include "approved_ui_font.inc"' not in source:
    source = prefix + source
if "constexpr SDL_Color aPurple" not in source:
    source = source.replace(
        "constexpr SDL_Color aBlue{94, 132, 205, 255};",
        "constexpr SDL_Color aBlue{94, 132, 205, 255};\n"
        "constexpr SDL_Color aPurple{124, 67, 184, 255};",
    )
suffix = """

#undef a_header
#undef a_actor_card
#undef a_place
#include \"approved_ui_place_exact.inc\"
"""
if '#include "approved_ui_place_exact.inc"' not in source:
    source = source.rstrip() + suffix
primitives_path.write_text(source, encoding="utf-8")

print(f"exact Place UI generated: glyphs={len(glyphs)} rle={len(rle)}")

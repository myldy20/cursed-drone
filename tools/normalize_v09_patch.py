#!/usr/bin/env python3
from pathlib import Path

path = Path('tools/apply_v09_patch.py')
text = path.read_text(encoding='utf-8')

# replace_between keeps its end marker, so generated replacements must not repeat it.
text = text.replace(
    "\nconstexpr std::array<std::array<cd::EngineKind, 4>, 8> kEngineGroups''')",
    "\n''')")
text = text.replace(
    "\nSDL_Color react(SDL_Color color, float activity) noexcept {''')",
    "\n''')")

# The short v0.8 README uses compact feature bullets rather than the old catalogue wording.
text = text.replace(
    "'''- four serial FX slots per actor: drive, low/high-pass, tremolo, delay, crusher, wavefolder, ring modulation, comb or empty;'''",
    "'''- four serial effects and four modulation lanes per actor;'''")
text = text.replace(
    "'''- четыре последовательных FX-слота на актёра: drive, low/high-pass, tremolo, delay, crusher, wavefolder, ring modulation, comb или пусто;'''",
    "'''- четыре последовательных эффекта и четыре линии модуляции на каждый слот;'''")

# README does not contain a separate backticked version token.
text = text.replace(
    "replace_once('README.md', '`0.8.0`', '`0.9.0`')\n",
    "")

# PortMaster gameinfo.xml has no application-version element.
start = text.find("replace_once('packaging/portmaster/curseddrone/gameinfo.xml'")
if start >= 0:
    end_marker = "             '<version>0.9.0</version>')\n"
    end = text.find(end_marker, start)
    if end < 0:
        raise SystemExit('cannot locate gameinfo replacement end')
    text = text[:start] + text[end + len(end_marker):]

# The live effect_visual block differs from the old prototype. Leave its neutral
# fallback in place for this integration instead of failing on a stale selector.
visual_start = text.find(
    "replace_once('src/sdl_main.cpp',\n'''        case cd::EffectKind::comb:")
readme_start = text.find("replace_once('README.md',", visual_start)
if visual_start >= 0 and readme_start >= 0:
    text = text[:visual_start] + text[readme_start:]

# The patch normalizes the live source hints before replacing the picker block.
# Match that normalized source only inside the old picker literal.
marker = text.find('old_effect_picker =')
if marker < 0:
    raise SystemExit('old effect picker literal not found')
prefix = text[:marker]
suffix = text[marker:]
suffix = suffix.replace('СТРЕЛКИ ВЫБОР', 'D-PAD ВЫБОР')
suffix = suffix.replace('ARROWS SELECT', 'D-PAD SELECT')
text = prefix + suffix

path.write_text(text, encoding='utf-8')

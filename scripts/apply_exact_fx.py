#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "android/app/src/main/cpp"

exact = CPP / "approved_ui_fx_exact.inc"
source = exact.read_text(encoding="utf-8")
source = source.replace(
    'ru_dummy ? "НЕ ИСПОЛЬЗУЕТСЯ" : "NOT USED"',
    '"NOT USED"',
)
exact.write_text(source, encoding="utf-8")

main = CPP / "android_approved_main.cpp"
source = main.read_text(encoding="utf-8")
needle = '#include "approved_ui_actor_exact.inc"\n#include "approved_ui_fx_memory.inc"'
replacement = '#include "approved_ui_actor_exact.inc"\n#include "approved_ui_fx_exact.inc"\n#include "approved_ui_fx_memory.inc"'
if needle in source:
    source = source.replace(needle, replacement, 1)
elif '#include "approved_ui_fx_exact.inc"' not in source:
    raise SystemExit("Android approved include order fragment not found")
main.write_text(source, encoding="utf-8")

fx_memory = CPP / "approved_ui_fx_memory.inc"
source = fx_memory.read_text(encoding="utf-8")
if "a_fx_exact(renderer, session, state, telemetry, area, scale);" not in source:
    start = source.find("void a_fx(SDL_Renderer* renderer")
    end = source.find("\nvoid a_master(SDL_Renderer* renderer", start)
    if start < 0 or end < 0:
        raise SystemExit("legacy FX function boundaries not found")
    wrapper = '''void a_fx(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, SDL_Rect area, int scale) {
    a_fx_exact(renderer, session, state, telemetry, area, scale);
}
'''
    source = source[:start] + wrapper + source[end:]
fx_memory.write_text(source, encoding="utf-8")

print("Exact FX screen integrated")

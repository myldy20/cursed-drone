#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "android/app/src/main/cpp"


def include_after(path: Path, needle: str, include: str) -> None:
    source = path.read_text(encoding="utf-8")
    if include in source:
        return
    if needle not in source:
        raise SystemExit(f"include anchor not found in {path}: {needle}")
    path.write_text(source.replace(needle, needle + "\n" + include, 1),
                    encoding="utf-8")


include_after(CPP / "android_approved_main.cpp",
              '#include "approved_ui_fx_exact.inc"',
              '#include "approved_ui_master_exact.inc"')
include_after(ROOT / "tools/android_ui_snapshots.cpp",
              '#include "../android/app/src/main/cpp/approved_ui_fx_exact.inc"',
              '#include "../android/app/src/main/cpp/approved_ui_master_exact.inc"')

path = CPP / "approved_ui_fx_memory.inc"
source = path.read_text(encoding="utf-8")
start = source.find("void a_master(SDL_Renderer* renderer")
end = source.find("\nvoid a_memory(SDL_Renderer* renderer", start)
if start < 0 or end < 0:
    raise SystemExit("legacy Master function boundaries not found")
wrapper = '''void a_master(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    SDL_Rect area, int scale) {
    a_master_exact(renderer, session, state, area, scale);
}
'''
path.write_text(source[:start] + wrapper + source[end:], encoding="utf-8")

print("Exact Master screen integrated")

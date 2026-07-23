#!/usr/bin/env python3
from pathlib import Path

path = Path(__file__).resolve().parents[1] / "android/app/src/main/cpp/approved_ui_actor_exact.inc"
source = path.read_text(encoding="utf-8")
before = '"20 ГЦ", "МЯГКО", "ТЁМНЫЙ", "СТАТИЧНО", "ГЛАДКАЯ", "−∞ ДБ", "ЛЕВО"'
after = '"20 ГЦ", "МЯГКО", "ТЁМНЫЙ", "СТАТИЧНО", "ГЛАДКАЯ", "MUTE", "ЛЕВО"'
if before not in source:
    raise SystemExit("actor level endpoint fragment not found")
path.write_text(source.replace(before, after, 1), encoding="utf-8")

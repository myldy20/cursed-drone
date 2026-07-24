#!/usr/bin/env python3
from pathlib import Path

path = Path(__file__).resolve().parents[1] / "android/app/src/main/cpp/approved_ui_actor_exact.inc"
source = path.read_text(encoding="utf-8")
before = 'ru(session) ? "ЧАСТОТА СОБЫТИЙ" : "EVENT RATE",'
after = 'ru(session) ? "ЧАСТОТА" : "RATE",'
if before not in source:
    raise SystemExit("Actor Events rate label fragment not found")
path.write_text(source.replace(before, after, 1), encoding="utf-8")

#!/usr/bin/env python3
from pathlib import Path

path = Path(__file__).resolve().parents[1] / "android/app/src/main/cpp/approved_ui_actor_exact.inc"
source = path.read_text(encoding="utf-8")
source = source.replace(
    'ru(session) ? "ЗАПУСК ▶" : "TRIGGER ▶",',
    'ru(session) ? "ЗАПУСК >" : "TRIGGER >",',
)
source = source.replace(
    'Action::actor_trigger, state.actor, 0, scale + 1, false, color);',
    'Action::actor_trigger, state.actor, 0, scale + 1, true, color);',
    1,
)
path.write_text(source, encoding="utf-8")

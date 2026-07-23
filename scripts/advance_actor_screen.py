#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "android/app/src/main/cpp"


def replace_once(path: Path, before: str, after: str) -> None:
    source = path.read_text(encoding="utf-8")
    if before not in source:
        raise SystemExit(f"missing expected fragment in {path}: {before[:100]!r}")
    path.write_text(source.replace(before, after, 1), encoding="utf-8")


place = CPP / "approved_ui_place_exact.inc"
replace_once(
    place,
    '''    if (telemetry.slot_event[static_cast<std::size_t>(actor)] > 0.04F) {
        SDL_Rect event{rect.x + rect.w - 30, rect.y + 9, 20, 20};
        a_round_fill(renderer, event, 10, color);
        a_round_fill(renderer, {event.x + 3, event.y + 3,
            event.w - 6, event.h - 6}, 7, aTop);
    }

''',
    '''    // Event activity belongs on the detailed Actor screen. On Place it
    // looked like an unexplained decorative dot and competed with the title.

''',
)
replace_once(
    place,
    '''    SDL_Rect bar{rect.x + label_w + 26, rect.y + 12,
        rect.w - label_w - 34, 9};''',
    '''    SDL_Rect bar{rect.x + label_w + 26, rect.y + 18,
        rect.w - label_w - 34, 9};''',
)
replace_once(place, "            low, aMutedCream, label_scale);",
    "            low, aMutedCream, label_scale);")
replace_once(
    place,
    '''        a_text(renderer, bar.x, rect.y + 28,
            low, aMutedCream, label_scale);''',
    '''        a_text(renderer, bar.x, rect.y + rect.h - 18,
            low, aMutedCream, label_scale);''',
)
replace_once(
    place,
    '''            rect.y + 28, high, aMutedCream, label_scale);''',
    '''            rect.y + rect.h - 18, high, aMutedCream, label_scale);''',
)
replace_once(
    place,
    '''    a_hline(renderer, rect.x, rect.y + 48, rect.w, aBorderSoft);''',
    '''    a_hline(renderer, rect.x, rect.y + rect.h - 1,
        rect.w, aBorderSoft);''',
)
replace_once(
    place,
    '''    const int start = macros.y + 39;
    constexpr int row_h = 58;''',
    '''    const int start = macros.y + 39;
    const int row_h = std::max(58,
        (macros.y + macros.h - 8 - start) / 5);''',
)

actor_exact = CPP / "approved_ui_actor_exact.inc"
actor_source = actor_exact.read_text(encoding="utf-8")
actor_source = actor_source.replace('previous, "◀",', 'previous, "<",')
actor_source = actor_source.replace('next, "▶",', 'next, ">",')
actor_exact.write_text(actor_source, encoding="utf-8")

main_cpp = CPP / "android_approved_main.cpp"
replace_once(
    main_cpp,
    '''#include "approved_ui_primitives.inc"
#include "approved_ui_actor.inc"
#include "approved_ui_fx_memory.inc"''',
    '''#include "approved_ui_primitives.inc"
#define a_actor a_actor_legacy
#include "approved_ui_actor.inc"
#undef a_actor
#include "approved_ui_actor_exact.inc"
#include "approved_ui_fx_memory.inc"''',
)

snapshots = ROOT / "tools/android_ui_snapshots.cpp"
replace_once(
    snapshots,
    '''#include "../android/app/src/main/cpp/approved_ui_primitives.inc"
#include "../android/app/src/main/cpp/approved_ui_actor.inc"
#include "../android/app/src/main/cpp/approved_ui_fx_memory.inc"''',
    '''#include "../android/app/src/main/cpp/approved_ui_primitives.inc"
#define a_actor a_actor_legacy
#include "../android/app/src/main/cpp/approved_ui_actor.inc"
#undef a_actor
#include "../android/app/src/main/cpp/approved_ui_actor_exact.inc"
#include "../android/app/src/main/cpp/approved_ui_fx_memory.inc"''',
)

touch = CPP / "android_touch_main.cpp"
replace_once(
    touch,
    '''    none, page, fade, actor_select, actor_toggle, actor_section,
    scene_picker, engine_picker, actor_trigger, actor_fx_select, actor_fx_picker,''',
    '''    none, page, fade, actor_select, actor_toggle, actor_section,
    scene_picker, engine_picker, actor_trigger, actor_root_step,
    actor_fx_select, actor_fx_picker,''',
)
replace_once(
    touch,
    '''    case Action::actor_section: state.actor_section = static_cast<ActorSection>(hit.a); return false;
    case Action::scene_picker: state.picker = PickerKind::scene; state.picker_page = 0; return false;''',
    '''    case Action::actor_section: {
        auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
        if (hit.a == 90) {
            if (slot.engine == cd::EngineKind::plaits) {
                cd::Session recipe = session;
                cd::apply_scene_recipe(recipe, session.scene);
                const float level = slot.level;
                const float pan = slot.pan;
                slot = recipe.slots[static_cast<std::size_t>(state.actor)];
                slot.level = level;
                slot.pan = pan;
                session.scene_modified = true;
                return true;
            }
            return false;
        }
        if (hit.a == 91) {
            slot.engine = cd::EngineKind::plaits;
            session.scene_modified = true;
            return true;
        }
        if (hit.a == 99) {
            state.picker = PickerKind::scale;
            state.picker_page = 0;
            return false;
        }
        if (hit.a == 97) {
            state.picker = PickerKind::plaits_model;
            state.picker_page = 0;
            return false;
        }
        if (hit.a == 98) {
            state.picker = PickerKind::output;
            state.picker_page = 0;
            return false;
        }
        state.actor_section = static_cast<ActorSection>(hit.a);
        return false;
    }
    case Action::scene_picker: state.picker = PickerKind::scene; state.picker_page = 0; return false;''',
)
replace_once(
    touch,
    '''    case Action::actor_trigger: state.pending_trigger = hit.a; return false;
    case Action::actor_fx_select: state.actor_fx = hit.a; return false;''',
    '''    case Action::actor_trigger: state.pending_trigger = hit.a; return false;
    case Action::actor_root_step: {
        auto& root = session.slots[static_cast<std::size_t>(state.actor)].tuning.root_midi;
        root = std::clamp(root + hit.a, 0, 127);
        session.scene_modified = true;
        return true;
    }
    case Action::actor_fx_select: state.actor_fx = hit.a; return false;''',
)

print("Place polish and exact Actor screen wired")

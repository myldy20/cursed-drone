#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: Path, before: str, after: str) -> None:
    source = path.read_text(encoding="utf-8")
    if before not in source:
        raise SystemExit(f"missing fragment in {path}: {before[:120]!r}")
    path.write_text(source.replace(before, after, 1), encoding="utf-8")


header = ROOT / "android/app/src/main/cpp/approved_ui_place_exact.inc"
replace_once(header,
'''    (void)height;
    fill(renderer, {0, 0, width, 118}, aTop);
    a_text(renderer, 27, 20, "CURSED DRONE", aCream, scale + 2);
    a_text(renderer, 232, 27,''',
'''    (void)height;
    const int safe = std::clamp(g_ui_safe_side, 0, width / 4);
    const int usable_width = width - 2 * safe;
    fill(renderer, {0, 0, width, 118}, aTop);
    a_text(renderer, safe + 27, 20, "CURSED DRONE", aCream, scale + 2);
    a_text(renderer, safe + 232, 27,''')
replace_once(header,
'''    SDL_Rect fade{width - 406, 13, 165, 35};''',
'''    SDL_Rect fade{safe + usable_width - 406, 13, 165, 35};''')
replace_once(header,
'''    const int dsp_x = width - 215;''',
'''    const int dsp_x = safe + usable_width - 215;''')
replace_once(header,
'''    const int tab_w = (width - 2 * margin - 4 * gap) / 5;
    for (int i = 0; i < 5; ++i) {
        SDL_Rect tab{margin + i * (tab_w + gap), nav_y, tab_w, nav_h};''',
'''    const int tab_w = (usable_width - 2 * margin - 4 * gap) / 5;
    for (int i = 0; i < 5; ++i) {
        SDL_Rect tab{safe + margin + i * (tab_w + gap), nav_y, tab_w, nav_h};''')

test = ROOT / "tools/android_ui_interaction_tests.cpp"
replace_once(test,
'''const HitTarget* find_action(const UiState& state, Action action) {
    for (auto it = state.hits.rbegin(); it != state.hits.rend(); ++it) {
        if (it->action == action) return &*it;
    }
    return nullptr;
}''',
'''const HitTarget* find_action(const UiState& state, Action action,
    int argument = -9'999) {
    for (auto it = state.hits.rbegin(); it != state.hits.rend(); ++it) {
        if (it->action == action &&
            (argument == -9'999 || it->a == argument)) return &*it;
    }
    return nullptr;
}''')
replace_once(test,
'''    for (const auto& hit : state.hits) {
        expect(hit.rect.x >= g_ui_safe_side,
            "no Place control may enter the left cutout safe area");
        expect(hit.rect.x + hit.rect.w <= kAndroidUiWidth - g_ui_safe_side,
            "no Place control may enter the right cutout safe area");
    }''',
'''    for (const auto& hit : state.hits) {
        const bool left_safe = hit.rect.x >= g_ui_safe_side;
        const bool right_safe = hit.rect.x + hit.rect.w <=
            kAndroidUiWidth - g_ui_safe_side;
        if (!left_safe || !right_safe) {
            std::cerr << "unsafe Place hit action=" << static_cast<int>(hit.action)
                      << " rect=" << hit.rect.x << ',' << hit.rect.y << ','
                      << hit.rect.w << ',' << hit.rect.h << '\\n';
        }
        expect(left_safe,
            "no Place control may enter the left cutout safe area");
        expect(right_safe,
            "no Place control may enter the right cutout safe area");
    }''')
replace_once(test,
'''    const HitTarget* actor_toggle = find_action(state, Action::actor_fx_toggle);''',
'''    const HitTarget* actor_toggle = find_action(
        state, Action::actor_fx_toggle, 0);''')
replace_once(test,
'''    const HitTarget* master_toggle = find_action(state, Action::master_fx_toggle);''',
'''    const HitTarget* master_toggle = find_action(
        state, Action::master_fx_toggle, 0);''')
replace_once(test,
'''    for (const auto& hit : state.hits) {
        expect(hit.rect.x >= g_ui_safe_side,
            "picker controls must respect the left cutout safe area");
        expect(hit.rect.x + hit.rect.w <= kAndroidUiWidth - g_ui_safe_side,
            "picker controls must respect the right cutout safe area");
    }''',
'''    for (const auto& hit : state.hits) {
        const bool left_safe = hit.rect.x >= g_ui_safe_side;
        const bool right_safe = hit.rect.x + hit.rect.w <=
            kAndroidUiWidth - g_ui_safe_side;
        if (!left_safe || !right_safe) {
            std::cerr << "unsafe picker hit action=" << static_cast<int>(hit.action)
                      << " rect=" << hit.rect.x << ',' << hit.rect.y << ','
                      << hit.rect.w << ',' << hit.rect.h << '\\n';
        }
        expect(left_safe,
            "picker controls must respect the left cutout safe area");
        expect(right_safe,
            "picker controls must respect the right cutout safe area");
    }''')

print("Exact header safe area and addressed FX interaction tests applied")

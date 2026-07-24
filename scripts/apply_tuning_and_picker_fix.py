#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: Path, before: str, after: str) -> None:
    source = path.read_text(encoding="utf-8")
    if before not in source:
        raise SystemExit(f"missing fragment in {path}: {before[:160]!r}")
    path.write_text(source.replace(before, after, 1), encoding="utf-8")


audio = ROOT / "src/audio.cpp"
replace_once(audio,
'''#include "cursed_drone/audio.hpp"
#include "plaits_actor.hpp"''',
'''#include "cursed_drone/audio.hpp"
#include "cursed_drone/scala.hpp"
#include "plaits_actor.hpp"''')
replace_once(audio,
'''                parameters.level = std::clamp(parameters.level, 0.0F, 1.5F);
                parameters.pan = std::clamp(parameters.pan, -1.0F, 1.0F);

                const StereoFrame actor_frame = runtime.engine.next(''',
'''                parameters.level = std::clamp(parameters.level, 0.0F, 1.5F);
                parameters.pan = std::clamp(parameters.pan, -1.0F, 1.0F);
                // Quantise after pitch modulation and global drift so an enabled
                // tuning acts as an audible pitch grid rather than passive metadata.
                parameters.frequency = quantize_frequency(
                    parameters.frequency, settings.tuning);

                const StereoFrame actor_frame = runtime.engine.next(''')

scala = ROOT / "src/scala.cpp"
replace_once(scala,
'''std::vector<ParsedScale> load_scala_scales(const std::vector<std::filesystem::path>& directories) {
    std::vector<ParsedScale> scales;
    scales.push_back(equal_temperament_scale());''',
'''std::vector<ParsedScale> load_scala_scales(const std::vector<std::filesystem::path>& directories) {
    std::vector<ParsedScale> scales;
    scales.push_back(equal_temperament_scale());
    scales.push_back({"MINOR PENTATONIC",
        {300.0F, 500.0F, 700.0F, 1000.0F, 1200.0F}, 1200.0F});
    scales.push_back({"PHRYGIAN",
        {100.0F, 300.0F, 500.0F, 700.0F, 800.0F, 1000.0F, 1200.0F},
        1200.0F});
    scales.push_back({"HARMONIC MINOR",
        {200.0F, 300.0F, 500.0F, 700.0F, 800.0F, 1100.0F, 1200.0F},
        1200.0F});
    scales.push_back({"WHOLE TONE",
        {200.0F, 400.0F, 600.0F, 800.0F, 1000.0F, 1200.0F}, 1200.0F});''')
replace_once(scala,
'''    for (int index = 0; index < tuning.degree_count; ++index) {
        const float candidate = tuning.cents[static_cast<std::size_t>(index)];
        const float distance = std::abs(candidate - local);''',
'''    for (int index = 0; index < tuning.degree_count; ++index) {
        const float candidate = tuning.cents[static_cast<std::size_t>(index)];
        // Degree count and period are editable. Ignore zero-filled or out-of-period
        // degrees rather than allowing them to create duplicate roots.
        if (candidate <= 0.0F || candidate >= period) continue;
        const float distance = std::abs(candidate - local);''')

touch = ROOT / "android/app/src/main/cpp/android_touch_main.cpp"
replace_once(touch,
'''enum class Action {
    none, page, fade, actor_select, actor_toggle, actor_section,
    scene_picker, engine_picker, actor_trigger, actor_root_step,''',
'''enum class Action {
    none, page, fade, actor_select, actor_toggle, actor_section,
    scene_picker, engine_picker, actor_trigger, tuning_toggle, actor_root_step,''')
replace_once(touch,
'''    actor_level, actor_pan, actor_event_density, tuning_root, euclidean_steps, euclidean_pulses,''',
'''    actor_level, actor_pan, actor_event_density, tuning_root, tuning_degrees,
    tuning_period, euclidean_steps, euclidean_pulses,''')
replace_once(touch,
'''constexpr SDL_Color kRed{225, 77, 96, 255};
constexpr std::array<SDL_Color, 4> kActorColors{{''',
'''constexpr SDL_Color kRed{225, 77, 96, 255};
constexpr int kPickerColumns = 7;
constexpr int kPickerRows = 5;
constexpr int kPickerPageSize = kPickerColumns * kPickerRows;
constexpr std::array<SDL_Color, 4> kActorColors{{''')
replace_once(touch,
'''    g_scales = cd::load_scala_scales({g_data_root / "scales"});
    g_scales.insert(g_scales.begin(), cd::equal_temperament_scale());''',
'''    g_scales = cd::load_scala_scales({g_data_root / "scales"});''')
replace_once(touch,
'''    case SliderKind::tuning_root: slot.tuning.root_midi = cd::mapping::tuning_root_from_normalized(normalized); break;
    case SliderKind::euclidean_steps:''',
'''    case SliderKind::tuning_root:
        slot.tuning.root_midi = cd::mapping::tuning_root_from_normalized(normalized);
        break;
    case SliderKind::tuning_degrees:
        slot.tuning.degree_count = 1 + static_cast<int>(std::lround(
            normalized * static_cast<float>(cd::kScaleDegreeCount - 1U)));
        break;
    case SliderKind::tuning_period:
        slot.tuning.period_cents = 50.0F + normalized * 4750.0F;
        break;
    case SliderKind::euclidean_steps:''')
replace_once(touch,
'''    case Action::actor_trigger: state.pending_trigger = hit.a; return false;
    case Action::actor_root_step: {''',
'''    case Action::actor_trigger: state.pending_trigger = hit.a; return false;
    case Action::tuning_toggle: {
        auto& tuning = session.slots[static_cast<std::size_t>(state.actor)].tuning;
        tuning.enabled = !tuning.enabled;
        session.scene_modified = true;
        return true;
    }
    case Action::actor_root_step: {''')
replace_once(touch,
'''        state.picker_page = cd::mapping::picker_next_page(
            state.picker_page, picker_count(state.picker), 8);''',
'''        state.picker_page = cd::mapping::picker_next_page(
            state.picker_page, picker_count(state.picker), kPickerPageSize);''')

picker_before = '''    const int pad = std::max(16, height / 45);
    const int safe = std::clamp(g_ui_safe_side, 0, width / 4);
    const int usable_width = width - 2 * safe;
    const int title_h = std::max(70, height / 10);
    SDL_Rect title{safe + pad, pad, usable_width - 2 * pad, title_h};'''
picker_after = '''    const int pad = std::max(12, height / 56);
    const int safe = std::clamp(g_ui_safe_side, 0, width / 4);
    const int usable_width = width - 2 * safe;
    const int title_h = std::max(52, height / 13);
    SDL_Rect title{safe + pad, pad, usable_width - 2 * pad, title_h};'''
replace_once(touch, picker_before, picker_after)
replace_once(touch,
'''    constexpr int columns = 3;
    constexpr int rows = 4;
    constexpr int page_size = columns * rows;
    const int count = picker_count(state.picker);
    const int max_page = std::max(0, (count - 1) / page_size);
    state.picker_page = std::clamp(state.picker_page, 0, max_page);
    const int grid_y = title.y + title.h + pad;
    const int footer_h = std::max(62, height / 11);
    const int grid_h = height - grid_y - footer_h - 2 * pad;
    const int gap = pad;
    const int item_w = (usable_width - 2 * pad - gap * (columns - 1)) / columns;
    const int item_h = (grid_h - gap * (rows - 1)) / rows;
    const int selected = current_picker_index(state, session);
    const int first = state.picker_page * page_size;
    for (int local = 0; local < page_size; ++local) {
        const int index = first + local;
        if (index >= count) break;
        const int col = local % columns;
        const int row = local / columns;
        SDL_Rect rect{safe + pad + col * (item_w + gap), grid_y + row * (item_h + gap), item_w, item_h};
        button(renderer, state, rect, picker_label(state.picker, index, session),
            index == selected, Action::picker_item, index, 0, scale,
            index == selected ? kGreen : kPurple);
    }
    const int footer_y = height - footer_h - pad;
    const int nav_w = (usable_width - 3 * pad) / 2;
    button(renderer, state, {safe + pad, footer_y, nav_w, footer_h},
        ru(session) ? "◀ ПРЕДЫДУЩИЕ" : "◀ PREVIOUS", state.picker_page > 0,
        Action::picker_previous, 0, 0, scale, kPurple);
    button(renderer, state, {safe + 2 * pad + nav_w, footer_y, nav_w, footer_h},
        ru(session) ? "СЛЕДУЮЩИЕ ▶" : "NEXT ▶", state.picker_page < max_page,
        Action::picker_next, 0, 0, scale, kPurple);''',
'''    const int count = picker_count(state.picker);
    const int max_page = std::max(0, (count - 1) / kPickerPageSize);
    state.picker_page = std::clamp(state.picker_page, 0, max_page);
    const int grid_y = title.y + title.h + 10;
    const int footer_h = max_page > 0 ? std::max(48, height / 14) : 0;
    const int grid_bottom = max_page > 0 ? height - footer_h - 2 * pad : height - pad;
    const int grid_h = std::max(1, grid_bottom - grid_y);
    constexpr int gap = 8;
    const int item_w = (usable_width - 2 * pad - gap * (kPickerColumns - 1)) /
        kPickerColumns;
    const int available_item_h = (grid_h - gap * (kPickerRows - 1)) /
        kPickerRows;
    const int item_h = std::clamp(available_item_h, 48, 68);
    const int selected = current_picker_index(state, session);
    const int first = state.picker_page * kPickerPageSize;
    for (int local = 0; local < kPickerPageSize; ++local) {
        const int index = first + local;
        if (index >= count) break;
        const int col = local % kPickerColumns;
        const int row = local / kPickerColumns;
        SDL_Rect rect{safe + pad + col * (item_w + gap),
            grid_y + row * (item_h + gap), item_w, item_h};
        button(renderer, state, rect, picker_label(state.picker, index, session),
            index == selected, Action::picker_item, index, 0,
            std::max(1, scale - 1), index == selected ? kGreen : kPurple);
    }
    if (max_page > 0) {
        const int footer_y = height - footer_h - pad;
        const int nav_w = (usable_width - 3 * pad) / 2;
        button(renderer, state, {safe + pad, footer_y, nav_w, footer_h},
            ru(session) ? "◀ ПРЕДЫДУЩИЕ" : "◀ PREVIOUS", state.picker_page > 0,
            Action::picker_previous, 0, 0, scale, kPurple);
        button(renderer, state,
            {safe + 2 * pad + nav_w, footer_y, nav_w, footer_h},
            ru(session) ? "СЛЕДУЮЩИЕ ▶" : "NEXT ▶", state.picker_page < max_page,
            Action::picker_next, 0, 0, scale, kPurple);
    }''')

actor = ROOT / "android/app/src/main/cpp/approved_ui_actor_exact.inc"
replace_once(actor,
'''std::string x_actor_midi_note(int midi) {
    static constexpr std::array<std::string_view, 12> names{{
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}};
    midi = std::clamp(midi, 0, 127);
    return std::string{names[static_cast<std::size_t>(midi % 12)]} +
        std::to_string(midi / 12 - 1);
}
''',
'''std::string x_actor_midi_note(int midi) {
    static constexpr std::array<std::string_view, 12> names{{
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}};
    midi = std::clamp(midi, 0, 127);
    return std::string{names[static_cast<std::size_t>(midi % 12)]} +
        std::to_string(midi / 12 - 1);
}

std::string x_actor_frequency_note(float frequency) {
    frequency = std::clamp(frequency, 4.0F, 20'000.0F);
    const int midi = static_cast<int>(std::lround(
        69.0F + 12.0F * std::log2(frequency / 440.0F)));
    return x_actor_midi_note(midi);
}
''')
replace_once(actor,
'''    const int pad = 12;
    const int tuning_h = static_cast<int>(right.h * 0.58F);
    SDL_Rect tuning{right.x + pad, right.y + pad, right.w - 2 * pad, tuning_h - pad};
    SDL_Rect root{right.x + pad, right.y + tuning_h + 2,
        right.w - 2 * pad, right.h - tuning_h - pad - 2};
    a_text(renderer, tuning.x, tuning.y,
        ru(session) ? "СТРОЙ" : "TUNING", aCream, scale + 1);
    SDL_Rect scale_pick{tuning.x, tuning.y + 28, tuning.w * 3 / 5, 43};
    x_button(renderer, state, scale_pick, std::string{slot.tuning.name.data()},
        Action::actor_section, 99, 0, scale + 1, false, aPurple);
    SDL_Rect editor{scale_pick.x + scale_pick.w + 8, scale_pick.y,
        tuning.x + tuning.w - (scale_pick.x + scale_pick.w + 8), 43};
    x_button(renderer, state, editor,
        ru(session) ? "РЕДАКТОР СТРОЯ" : "TUNING EDITOR",
        Action::actor_section, 99, 0, scale, false, aPurple);
    x_actor_scale_preview(renderer,
        {tuning.x + 8, tuning.y + 105, tuning.w - 16, 31},
        slot.tuning.root_midi, aActor[static_cast<std::size_t>(state.actor)]);

    a_text(renderer, root.x, root.y,
        ru(session) ? "КОРЕНЬ" : "ROOT", aCream, scale + 1);
    const int controls_y = root.y + 28;
    SDL_Rect previous{root.x, controls_y, 48, 45};
    SDL_Rect next{root.x + root.w - 48, controls_y, 48, 45};
    SDL_Rect note{previous.x + previous.w + 7, controls_y,
        root.w - previous.w - next.w - 14, 45};
    x_button(renderer, state, previous, "<", Action::actor_root_step,
        -1, 0, scale + 1, false, aPurple);
    a_card(renderer, note, aPanel2, aBorderSoft, 7, false);
    a_centered_text(renderer, note,
        x_actor_midi_note(slot.tuning.root_midi) + "   " +
            std::to_string(slot.tuning.root_midi),
        aGreen, scale + 2);
    add_hit(state, note, Action::slider, 0, 0, SliderKind::tuning_root);
    x_button(renderer, state, next, ">", Action::actor_root_step,
        1, 0, scale + 1, false, aPurple);
    SDL_Rect snap{root.x, controls_y + 54, root.w, 40};
    x_button(renderer, state, snap,
        ru(session) ? "ПРИВЯЗКА К ЛАДУ" : "SNAP TO SCALE",
        Action::actor_section, 99, 0, scale, false, aPurple);''',
'''    const int pad = 12;
    SDL_Rect tuning{right.x + pad, right.y + pad,
        right.w - 2 * pad, right.h - 2 * pad};
    const SDL_Color accent = aActor[static_cast<std::size_t>(state.actor)];
    a_text(renderer, tuning.x, tuning.y + 5,
        ru(session) ? "КВАНТОВАНИЕ ВЫСОТЫ" : "PITCH QUANTISER",
        aCream, scale + 1);
    SDL_Rect enabled{tuning.x + tuning.w - 116, tuning.y, 116, 32};
    x_button(renderer, state, enabled,
        slot.tuning.enabled ? (ru(session) ? "ВКЛ" : "ON") :
            (ru(session) ? "ВЫКЛ" : "OFF"),
        Action::tuning_toggle, 0, 0, scale, slot.tuning.enabled,
        slot.tuning.enabled ? aGreen : aRed);

    SDL_Rect scale_pick{tuning.x, tuning.y + 42, tuning.w, 42};
    x_button(renderer, state, scale_pick,
        std::string{slot.tuning.name.data()} + "  ▼",
        Action::actor_section, 99, 0, scale + 1, slot.tuning.enabled,
        aPurple);

    const float snapped_frequency = cd::quantize_frequency(
        slot.frequency_hz, slot.tuning);
    SDL_Rect result{tuning.x, tuning.y + 94, tuning.w, 48};
    a_card(renderer, result, aPanel2,
        slot.tuning.enabled ? accent : aBorderSoft, 7, false);
    a_text(renderer, result.x + 12, result.y + 7,
        ru(session) ? "ИСХОДНАЯ" : "RAW", aMutedCream, scale);
    a_text(renderer, result.x + 12, result.y + 24,
        decimal(slot.frequency_hz, "HZ"), aCream, scale + 1);
    a_text(renderer, result.x + result.w / 2 - 22, result.y + 18,
        ">", aMutedCream, scale + 1);
    a_text(renderer, result.x + result.w / 2 + 8, result.y + 7,
        ru(session) ? "ПО СТРОЮ" : "SNAPPED", aMutedCream, scale);
    const std::string snapped = decimal(snapped_frequency, "HZ") + "  " +
        x_actor_frequency_note(snapped_frequency);
    a_text(renderer, result.x + result.w / 2 + 8, result.y + 24,
        snapped, slot.tuning.enabled ? aGreen : aMutedCream, scale + 1);

    a_text(renderer, tuning.x, tuning.y + 153,
        ru(session) ? "КОРЕНЬ" : "ROOT", aCream, scale);
    const int controls_y = tuning.y + 171;
    SDL_Rect previous{tuning.x, controls_y, 46, 40};
    SDL_Rect next{tuning.x + tuning.w - 46, controls_y, 46, 40};
    SDL_Rect note{previous.x + previous.w + 7, controls_y,
        tuning.w - previous.w - next.w - 14, 40};
    x_button(renderer, state, previous, "<", Action::actor_root_step,
        -1, 0, scale + 1, false, aPurple);
    a_card(renderer, note, aPanel2, aBorderSoft, 7, false);
    a_centered_text(renderer, note,
        x_actor_midi_note(slot.tuning.root_midi) + "   MIDI " +
            std::to_string(slot.tuning.root_midi),
        aGreen, scale + 1);
    add_hit(state, note, Action::slider, 0, 0, SliderKind::tuning_root);
    x_button(renderer, state, next, ">", Action::actor_root_step,
        1, 0, scale + 1, false, aPurple);

    const float degree_norm = static_cast<float>(slot.tuning.degree_count - 1) /
        static_cast<float>(std::max<std::size_t>(1U, cd::kScaleDegreeCount - 1U));
    x_slider(renderer, state,
        {tuning.x, tuning.y + 221, tuning.w, 54},
        ru(session) ? "СТУПЕНИ" : "DEGREES",
        std::to_string(slot.tuning.degree_count), degree_norm,
        SliderKind::tuning_degrees, 0, 0, scale, aBlue, "1",
        std::to_string(cd::kScaleDegreeCount));
    const float period_norm = std::clamp(
        (slot.tuning.period_cents - 50.0F) / 4750.0F, 0.0F, 1.0F);
    x_slider(renderer, state,
        {tuning.x, tuning.y + 281, tuning.w, 54},
        ru(session) ? "ПЕРИОД" : "PERIOD",
        std::to_string(static_cast<int>(std::lround(
            slot.tuning.period_cents))) + " CENT",
        period_norm, SliderKind::tuning_period, 0, 0, scale,
        aPurple, "50", "4800");''')

interaction = ROOT / "tools/android_ui_interaction_tests.cpp"
replace_once(interaction,
'''    base.slots[0].level = 0.55F;

    auto fx_off = base;''',
'''    base.slots[0].level = 0.55F;

    auto tuning_off = base;
    tuning_off.slots[0].frequency_hz = 61.0F;
    tuning_off.slots[0].tuning.enabled = false;
    auto tuning_on = tuning_off;
    cd::apply_scale(tuning_on.slots[0].tuning,
        {"TEST PENTATONIC", {300.0F, 500.0F, 700.0F, 1000.0F, 1200.0F},
            1200.0F});
    expect(render_difference(tuning_off, tuning_on) > 1.0,
        "enabled tuning must materially change DSP output");

    auto fx_off = base;''')
replace_once(interaction,
'''    UiState state{};
    state.actor = 0;''',
'''    g_scales = cd::load_scala_scales({});
    expect(g_scales.size() >= 5U,
        "Android should provide useful built-in tunings without external files");
    UiState state{};
    state.actor = 0;''')
replace_once(interaction,
'''    draw(renderer, session, state, Page::actor, ActorSection::modulation);''',
'''    draw(renderer, session, state, Page::actor, ActorSection::sound);
    const HitTarget* tuning_toggle = find_action(state, Action::tuning_toggle);
    expect(tuning_toggle != nullptr,
        "Tuning should expose a real enable toggle");
    if (tuning_toggle != nullptr) {
        const bool before = session.slots[0].tuning.enabled;
        tap(session, state, *tuning_toggle);
        expect(session.slots[0].tuning.enabled != before,
            "Tuning toggle should mutate Session");
    }
    int tuning_picker_count = 0;
    for (const auto& hit : state.hits) {
        if (hit.action == Action::actor_section && hit.a == 99)
            ++tuning_picker_count;
    }
    expect(tuning_picker_count == 1,
        "Tuning should have one unambiguous scale picker");
    const HitTarget* degrees = find_slider(state, SliderKind::tuning_degrees);
    expect(degrees != nullptr, "Tuning degree count should be editable");
    if (degrees != nullptr) {
        set_slider(session, state, *degrees, 0.15F);
        expect(session.slots[0].tuning.degree_count >= 2 &&
            session.slots[0].tuning.degree_count <= 8,
            "Tuning degree gesture should reach Session");
    }
    const HitTarget* period = find_slider(state, SliderKind::tuning_period);
    expect(period != nullptr, "Tuning period should be editable");
    if (period != nullptr) {
        set_slider(session, state, *period, 0.24F);
        expect(session.slots[0].tuning.period_cents > 1'000.0F &&
            session.slots[0].tuning.period_cents < 1'300.0F,
            "Tuning period gesture should reach Session");
    }

    draw(renderer, session, state, Page::actor, ActorSection::modulation);''')
replace_once(interaction,
'''    state.picker = PickerKind::effect;
    state.picker_master = false;
    state.picker_effect = 0;
    draw(renderer, session, state, Page::fx);
    for (const auto& hit : state.hits) {''',
'''    const auto verify_compact_picker = [&](PickerKind picker, int expected) {
        state.picker = picker;
        state.picker_page = 0;
        draw(renderer, session, state, Page::fx);
        int items = 0;
        bool has_navigation = false;
        int tallest = 0;
        for (const auto& hit : state.hits) {
            if (hit.action == Action::picker_item) {
                ++items;
                tallest = std::max(tallest, hit.rect.h);
            }
            if (hit.action == Action::picker_previous ||
                hit.action == Action::picker_next) has_navigation = true;
        }
        expect(items == expected,
            "all built-in picker items should fit on one screen");
        expect(!has_navigation,
            "single-page built-in pickers should not show pagination");
        expect(tallest <= 68,
            "picker buttons should stay compact for short labels");
    };
    verify_compact_picker(PickerKind::scene,
        static_cast<int>(cd::catalog::scenes.size()));
    verify_compact_picker(PickerKind::effect,
        static_cast<int>(cd::catalog::effects.size()));
    verify_compact_picker(PickerKind::engine,
        static_cast<int>(cd::catalog::engines.size()));
    verify_compact_picker(PickerKind::scale,
        static_cast<int>(g_scales.size()));

    state.picker = PickerKind::effect;
    state.picker_master = false;
    state.picker_effect = 0;
    draw(renderer, session, state, Page::fx);
    for (const auto& hit : state.hits) {''')

core_tests = ROOT / "tests/test_main.cpp"
replace_once(core_tests,
'''    const float quantized = cd::quantize_frequency(61.0F, tuning);
    expect(std::isfinite(quantized) && quantized > 0.0F, "Scala quantisation should produce a valid frequency");
    expect(std::abs(quantized - 61.0F) > 0.001F, "Scala quantisation should move an off-scale frequency");''',
'''    const float quantized = cd::quantize_frequency(61.0F, tuning);
    expect(std::isfinite(quantized) && quantized > 0.0F, "Scala quantisation should produce a valid frequency");
    expect(std::abs(quantized - 61.0F) > 0.001F, "Scala quantisation should move an off-scale frequency");
    const auto builtins = cd::load_scala_scales({});
    expect(builtins.size() >= 5U, "useful built-in scales should always be available");
    for (std::size_t left = 0; left < builtins.size(); ++left) {
        for (std::size_t right = left + 1U; right < builtins.size(); ++right) {
            expect(builtins[left].name != builtins[right].name,
                "built-in scale names should not be duplicated");
        }
    }''')

print("Functional tuning and compact single-screen pickers applied")

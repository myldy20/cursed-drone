#!/usr/bin/env python3
from pathlib import Path

path = Path('src/sdl_main.cpp')
text = path.read_text(encoding='utf-8')

def replace_once(old: str, new: str) -> None:
    global text
    count = text.count(old)
    if count != 1:
        raise SystemExit(f'expected one match, found {count}: {old[:120]!r}')
    text = text.replace(old, new)

replace_once(
'''    bool start_held{false};
    bool select_held{false};
};''',
'''    bool start_held{false};
    bool select_held{false};
    bool back_held{false};
    bool back_long_action{false};
    Uint32 back_held_since{0};
};''')

replace_once(
'''    const Uint32 deadline = SDL_GetTicks() + (handheld() ? 1'350U : 1'000U);
    bool keep_running = true;
    bool skip = false;
    while (keep_running && !skip && SDL_TICKS_PASSED(deadline, SDL_GetTicks()) == SDL_FALSE) {
        SDL_Event event{};
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                keep_running = false;
            } else if (event.type == SDL_KEYDOWN ||
                       event.type == SDL_CONTROLLERBUTTONDOWN ||
                       event.type == SDL_JOYBUTTONDOWN) {
                skip = true;
            }
        }
        SDL_Delay(16);
    }''',
'''    const Uint32 shown_at = SDL_GetTicks();
    const Uint32 minimum = handheld() ? 2'200U : 1'500U;
    const Uint32 maximum = handheld() ? 4'200U : 3'000U;
    bool keep_running = true;
    bool skip = false;
    while (keep_running && !skip && SDL_GetTicks() - shown_at < maximum) {
        SDL_Event event{};
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                keep_running = false;
            } else if (SDL_GetTicks() - shown_at >= minimum &&
                       (event.type == SDL_KEYDOWN ||
                        event.type == SDL_CONTROLLERBUTTONDOWN ||
                        event.type == SDL_JOYBUTTONDOWN)) {
                skip = true;
            }
        }
        SDL_Delay(16);
    }''')

replace_once(
'''    SDL_SetRenderDrawColor(renderer, 75, 67, 86, 255);
    SDL_RenderDrawLine(renderer, 32, 267, 468, 267);
    draw_myldy_mark(renderer, 32, 275, 1, kInk);
    cd::ui::draw_text(renderer, 68, 276, "MYLDY DESIGN  @MYLDY20", kInk);
    cd::ui::draw_text(renderer, 68, 292, "DSP: DAISYSP  UI: FONT512  PORTMASTER", kDim);
    constexpr std::string_view version{"V0.9.0"};
    cd::ui::draw_text(renderer, 468 - cd::ui::text_width(version), 276, version, kDim);
    char cpu[48]{};
    std::snprintf(cpu, sizeof(cpu), "DSP %d%%", state.displayed_cpu_percent);
    cd::ui::draw_text(renderer, 468 - cd::ui::text_width(cpu), 292, cpu,
        state.displayed_cpu_percent < 75 ? kDim : kFxColors[0]);''',
'''    SDL_SetRenderDrawColor(renderer, 75, 67, 86, 255);
    SDL_RenderDrawLine(renderer, 32, 267, 468, 267);
    constexpr std::string_view version{"EXPERIMENT 0.10"};
    cd::ui::draw_text(renderer, 32, 278, version, kDim);
    char cpu[48]{};
    std::snprintf(cpu, sizeof(cpu), "DSP %d%%", state.displayed_cpu_percent);
    cd::ui::draw_text(renderer, 468 - cd::ui::text_width(cpu), 278, cpu,
        state.displayed_cpu_percent < 75 ? kDim : kFxColors[0]);''')

# Picker labels use physical TrimUI labels: A confirms, B cancels.
text = text.replace('B ПРИНЯТЬ   A НАЗАД', 'A ПРИНЯТЬ   B НАЗАД')
text = text.replace('B APPLY   A BACK', 'A APPLY   B BACK')
text = text.replace('B OK  A НАЗАД', 'A OK  B НАЗАД')
text = text.replace('B OK  A BACK', 'A OK  B BACK')
text = text.replace('B ВЫБРАТЬ  A НАЗАД', 'A ВЫБРАТЬ  B НАЗАД')
text = text.replace('B SELECT  A BACK', 'A SELECT  B BACK')

old_help = '''        } else if (state.page == Page::effects) {
            help = ru(session) ? "LT/RT FX  UP/DN ПАРАМ.  Y ДОРОЖКА  L/R ЗНАЧ.  START ТИП"
                               : "LT/RT FX  UP/DN PARAM  Y TRACK  L/R VALUE  START TYPE";
        } else if (state.page == Page::perform) {
            help = ru(session) ? "UP/DN РУЧКА/УРОВ.  LT/RT ДОРОЖКА  B MUTE  START ЛАНДШАФТ"
                               : "UP/DN MACRO/LEVEL  LT/RT TRACK  B MUTE  START LANDSCAPE";
        } else if (state.page == Page::master || state.page == Page::setup) {
            help = ru(session) ? "UP/DN ПАРАМ.  L/R ЗНАЧ.  X ЭКРАН  SELECT ФЕЙД  A KILL"
                               : "UP/DN PARAM  L/R VALUE  X PAGE  SELECT FADE  A KILL";
        } else {
            help = ru(session) ? "LT/RT СЛОТ  UP/DN ПАРАМ.  L/R ЗНАЧ.  START ВЫБОР  B MUTE"
                               : "LT/RT SLOT  UP/DN PARAM  L/R VALUE  START CHOOSE  B MUTE";
        }'''
new_help = '''        } else if (state.page == Page::effects) {
            help = ru(session) ? "D-PAD НАВИГАЦИЯ  L/R ЗНАЧ.  A ЭФФЕКТ  Y АКТЕР  B НАЗАД"
                               : "D-PAD NAVIGATE  L/R VALUE  A EFFECT  Y ACTOR  B BACK";
        } else if (state.page == Page::perform) {
            help = ru(session) ? "D-PAD НАВИГАЦИЯ  L/R ЗНАЧ.  A ДЕЙСТВИЕ  Y АКТЕР  X ЭКРАН"
                               : "D-PAD NAVIGATE  L/R VALUE  A ACTION  Y ACTOR  X PAGE";
        } else if (state.page == Page::master || state.page == Page::setup) {
            help = ru(session) ? "D-PAD НАВИГАЦИЯ  L/R ЗНАЧ.  B НАЗАД  SELECT ФЕЙД"
                               : "D-PAD NAVIGATE  L/R VALUE  B BACK  SELECT FADE";
        } else {
            help = ru(session) ? "D-PAD НАВИГАЦИЯ  L/R ЗНАЧ.  A ОТКРЫТЬ  Y АКТЕР  B НАЗАД"
                               : "D-PAD NAVIGATE  L/R VALUE  A OPEN  Y ACTOR  B BACK";
        }'''
replace_once(old_help, new_help)

# Add helpers before the fade function.
replace_once(
'''void toggle_fade(cd::Session& session, UiState& state) noexcept {''',
'''void activate_current(cd::Session& session, UiState& state) {
    if (state.page == Page::perform) {
        if (state.scene_track_focus) {
            auto& slot = session.slots[static_cast<std::size_t>(state.slot)];
            slot.enabled = !slot.enabled;
        } else {
            open_scene_picker(state, session);
        }
    } else if (state.page == Page::slot) {
        if (parameter(state) == 0) open_engine_picker(state, session);
    } else if (state.page == Page::effects) {
        open_effect_picker(state, session);
    }
}

void go_back(UiState& state) noexcept {
    if (state.picker != Picker::none) {
        state.picker = Picker::none;
    } else if (state.page == Page::perform && state.scene_track_focus) {
        state.scene_track_focus = false;
    } else if (state.page != Page::perform) {
        state.page = Page::perform;
    }
}

void toggle_fade(cd::Session& session, UiState& state) noexcept {''')

old_controller = '''            } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START) state.start_held = true;
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) state.select_held = true;
                if (state.start_held && state.select_held) {
                    running = false;
                    continue;
                }
                if (state.picker != Picker::none) {
                    switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: move_picker(state, -1, 0); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: move_picker(state, 1, 0); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP: move_picker(state, 0, -1); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: move_picker(state, 0, 1); break;
                    case SDL_CONTROLLER_BUTTON_A:
                    case SDL_CONTROLLER_BUTTON_START: confirm_picker(state, session); changed = true; break;
                    case SDL_CONTROLLER_BUTTON_B: state.picker = Picker::none; break;
                    default: break;
                    }
                } else {
                    switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: navigate_horizontal(state, session, -1); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: navigate_horizontal(state, session, 1); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP: navigate_vertical(state, session, -1); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: navigate_vertical(state, session, 1); break;
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                        if (state.page == Page::slot && parameter(state) == 0) open_engine_picker(state, session);
                        else { start_adjust(session, state, -1, now); changed = true; }
                        break;
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                        if (state.page == Page::slot && parameter(state) == 0) open_engine_picker(state, session);
                        else { start_adjust(session, state, 1, now); changed = true; }
                        break;
                    case SDL_CONTROLLER_BUTTON_A:
                        session.slots[static_cast<std::size_t>(state.slot)].enabled =
                            !session.slots[static_cast<std::size_t>(state.slot)].enabled;
                        changed = true;
                        break;
                    case SDL_CONTROLLER_BUTTON_B:
                        audio.graph.panic();
                        state.kill_flash_until = now + 700U;
                        break;
                    case SDL_CONTROLLER_BUTTON_X:
                        state.page = static_cast<Page>((page_index(state.page) + 1) % 5);
                        break;
                    case SDL_CONTROLLER_BUTTON_Y:
                        if (state.page == Page::effects) state.slot = (state.slot + 1) % 4;
                        break;
                    case SDL_CONTROLLER_BUTTON_BACK: toggle_fade(session, state); changed = true; break;
                    case SDL_CONTROLLER_BUTTON_START:
                        open_context_picker(state, session);
                        break;
                    default: break;
                    }
                }
            } else if (event.type == SDL_CONTROLLERBUTTONUP) {
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START) state.start_held = false;
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) state.select_held = false;
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER) stop_adjust(state, -1);
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) stop_adjust(state, 1);
            }'''
new_controller = '''            } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START) state.start_held = true;
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) state.select_held = true;
                if (state.start_held && state.select_held) {
                    running = false;
                    continue;
                }
                if (state.picker != Picker::none) {
                    switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: move_picker(state, -1, 0); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: move_picker(state, 1, 0); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP: move_picker(state, 0, -1); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: move_picker(state, 0, 1); break;
                    // TrimUI physical A is reported as SDL B; physical B is SDL A.
                    case SDL_CONTROLLER_BUTTON_B: confirm_picker(state, session); changed = true; break;
                    case SDL_CONTROLLER_BUTTON_A: state.picker = Picker::none; break;
                    default: break;
                    }
                } else {
                    switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: navigate_horizontal(state, session, -1); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: navigate_horizontal(state, session, 1); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP: navigate_vertical(state, session, -1); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: navigate_vertical(state, session, 1); break;
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                        start_adjust(session, state, -1, now); changed = true;
                        break;
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                        start_adjust(session, state, 1, now); changed = true;
                        break;
                    case SDL_CONTROLLER_BUTTON_B:
                        activate_current(session, state); changed = true;
                        break;
                    case SDL_CONTROLLER_BUTTON_A:
                        state.back_held = true;
                        state.back_long_action = false;
                        state.back_held_since = now;
                        break;
                    case SDL_CONTROLLER_BUTTON_X:
                        state.page = static_cast<Page>((page_index(state.page) + 1) % 5);
                        break;
                    case SDL_CONTROLLER_BUTTON_Y:
                        state.slot = (state.slot + 1) % 4;
                        break;
                    case SDL_CONTROLLER_BUTTON_BACK: toggle_fade(session, state); changed = true; break;
                    case SDL_CONTROLLER_BUTTON_START:
                        break;
                    default: break;
                    }
                }
            } else if (event.type == SDL_CONTROLLERBUTTONUP) {
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START) state.start_held = false;
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) state.select_held = false;
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER) stop_adjust(state, -1);
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) stop_adjust(state, 1);
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_A && state.back_held) {
                    if (!state.back_long_action) go_back(state);
                    state.back_held = false;
                }
            }'''
replace_once(old_controller, new_controller)

replace_once(
'''        const Uint32 now = SDL_GetTicks();
        changed = repeat_adjust(session, state, now) || changed;''',
'''        const Uint32 now = SDL_GetTicks();
        if (state.back_held && !state.back_long_action && now - state.back_held_since >= 1'100U) {
            audio.graph.panic();
            state.kill_flash_until = now + 700U;
            state.back_long_action = true;
        }
        changed = repeat_adjust(session, state, now) || changed;''')

path.write_text(text, encoding='utf-8')

// SPDX-License-Identifier: GPL-3.0-or-later
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "bitmap_text.hpp"
#include "cursed_drone/audio.hpp"
#include "cursed_drone/i18n.hpp"
#include "cursed_drone/session.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <span>
#include <string>
#include <string_view>

namespace cd = cursed_drone;

namespace {

constexpr int kWidth = 512;
constexpr int kHeight = 384;
constexpr SDL_Color kInk{238, 226, 197, 255};
constexpr SDL_Color kDim{151, 143, 158, 255};
constexpr SDL_Color kPurple{117, 67, 171, 255};
constexpr SDL_Color kPanel{35, 30, 46, 255};
constexpr std::array<SDL_Color, 4> kFxColors{{
    {216, 88, 88, 255}, {224, 154, 63, 255}, {80, 169, 154, 255}, {91, 122, 187, 255},
}};

enum class Page { perform, slot, effects, master };

struct AudioBridge {
    cd::AudioGraph graph{};
};

struct UiState {
    Page page{Page::perform};
    int slot{0};
    std::array<int, 4> selected{};
    int held_direction{0};
    Uint32 held_since{0};
    Uint32 last_repeat{0};
    bool auto_fade{false};
    float fade_target{1.0F};
    Uint32 kill_flash_until{0};
};

void audio_callback(void* userdata, Uint8* bytes, int byte_count) {
    auto& bridge = *static_cast<AudioBridge*>(userdata);
    auto* frames = reinterpret_cast<cd::StereoFrame*>(bytes);
    bridge.graph.process({
        frames, static_cast<std::size_t>(byte_count) / sizeof(cd::StereoFrame)});
}

bool ru(const cd::Session& session) noexcept { return session.locale == cd::Locale::ru; }

std::string_view page_name(Page page, bool russian) noexcept {
    switch (page) {
    case Page::perform: return russian ? "ИГРА" : "PERFORM";
    case Page::slot: return russian ? "СЛОТ" : "SLOT";
    case Page::effects: return russian ? "ЭФФЕКТЫ" : "EFFECTS";
    case Page::master: return russian ? "МАСТЕР" : "MASTER";
    }
    return {};
}

std::string_view macro_name(int index, bool russian) noexcept {
    constexpr std::array<std::string_view, 5> r{
        "ТЕКСТУРА", "ПУЛЬС", "ХАОС", "ПРОСТРАНСТВО", "ФЕЙД"};
    constexpr std::array<std::string_view, 5> e{
        "TEXTURE", "PULSE", "CHAOS", "SPACE", "FADE"};
    return (russian ? r : e)[static_cast<std::size_t>(index)];
}

std::string_view slot_name(int index, cd::Locale locale) {
    constexpr std::array ids{
        cd::TextId::frequency, cd::TextId::timbre, cd::TextId::color, cd::TextId::motion,
        cd::TextId::texture, cd::TextId::level, cd::TextId::pan,
    };
    return cd::tr(locale, ids[static_cast<std::size_t>(index)]);
}

std::string_view effect_name(cd::EffectKind kind, bool russian) noexcept {
    switch (kind) {
    case cd::EffectKind::bypass: return russian ? "ОБХОД" : "BYPASS";
    case cd::EffectKind::drive: return "DRIVE";
    case cd::EffectKind::lowpass: return russian ? "ФИЛЬТР" : "LOWPASS";
    case cd::EffectKind::tremolo: return russian ? "ТРЕМОЛО" : "TREMOLO";
    case cd::EffectKind::delay: return russian ? "ДЕЛЕЙ" : "DELAY";
    case cd::EffectKind::crusher: return russian ? "КРАШЕР" : "CRUSHER";
    }
    return {};
}

std::string_view effect_field(int index, bool russian) noexcept {
    switch (index % 3) {
    case 0: return russian ? "СИЛА" : "AMOUNT";
    case 1: return russian ? "ТОН" : "TONE";
    default: return russian ? "ОБР.СВЯЗЬ" : "FEEDBACK";
    }
}

int page_index(Page page) noexcept { return static_cast<int>(page); }

int parameter_count(Page page) noexcept {
    switch (page) {
    case Page::perform: return 5;
    case Page::slot: return 7;
    case Page::effects: return 12;
    case Page::master: return 2;
    }
    return 1;
}

int& parameter(UiState& state) noexcept {
    return state.selected[static_cast<std::size_t>(page_index(state.page))];
}

int parameter(const UiState& state) noexcept {
    return state.selected[static_cast<std::size_t>(page_index(state.page))];
}

float macro_value(const cd::PerformanceSettings& settings, int index) noexcept {
    switch (index) {
    case 0: return settings.texture;
    case 1: return settings.pulse;
    case 2: return settings.chaos;
    case 3: return settings.space;
    default: return settings.fade;
    }
}

float* macro_value(cd::PerformanceSettings& settings, int index) noexcept {
    switch (index) {
    case 0: return &settings.texture;
    case 1: return &settings.pulse;
    case 2: return &settings.chaos;
    case 3: return &settings.space;
    default: return &settings.fade;
    }
}

float slot_value(const cd::SlotSettings& slot, int index) noexcept {
    switch (index) {
    case 0: return slot.frequency_hz;
    case 1: return slot.timbre;
    case 2: return slot.color;
    case 3: return slot.motion;
    case 4: return slot.texture;
    case 5: return slot.level;
    default: return slot.pan;
    }
}

float* slot_value(cd::SlotSettings& slot, int index) noexcept {
    switch (index) {
    case 0: return &slot.frequency_hz;
    case 1: return &slot.timbre;
    case 2: return &slot.color;
    case 3: return &slot.motion;
    case 4: return &slot.texture;
    case 5: return &slot.level;
    default: return &slot.pan;
    }
}

float effect_value(const cd::EffectSettings& effect, int field) noexcept {
    return field == 0 ? effect.amount : (field == 1 ? effect.tone : effect.feedback);
}

float* effect_value(cd::EffectSettings& effect, int field) noexcept {
    return field == 0 ? &effect.amount : (field == 1 ? &effect.tone : &effect.feedback);
}

std::string value_text(const cd::Session& session, const UiState& state, int slot_override = -1) {
    char result[40]{};
    const int selected = parameter(state);
    switch (state.page) {
    case Page::perform:
        std::snprintf(result, sizeof(result), "%d%%", static_cast<int>(std::lround(
            macro_value(session.performance, selected) * 100.0F)));
        break;
    case Page::slot: {
        const int slot = slot_override >= 0 ? slot_override : state.slot;
        const float value = slot_value(session.slots[static_cast<std::size_t>(slot)], selected);
        if (selected == 0) {
            std::snprintf(result, sizeof(result), "%.1f Hz", static_cast<double>(value));
        } else if (selected == 6) {
            std::snprintf(result, sizeof(result), "%+.2f", static_cast<double>(value));
        } else {
            std::snprintf(result, sizeof(result), "%d%%", static_cast<int>(std::lround(value * 100.0F)));
        }
        break;
    }
    case Page::effects: {
        const auto& effect = session.slots[static_cast<std::size_t>(state.slot)]
            .effects[static_cast<std::size_t>(selected / 3)];
        std::snprintf(result, sizeof(result), "%d%%", static_cast<int>(std::lround(
            effect_value(effect, selected % 3) * 100.0F)));
        break;
    }
    case Page::master:
        if (selected == 0) {
            std::snprintf(result, sizeof(result), "%d%%", static_cast<int>(std::lround(
                session.master_level * 100.0F)));
        } else {
            std::snprintf(result, sizeof(result), "%.0f BPM", static_cast<double>(session.tempo_bpm));
        }
        break;
    }
    return result;
}

std::string current_label(const cd::Session& session, const UiState& state) {
    switch (state.page) {
    case Page::perform: return std::string{macro_name(parameter(state), ru(session))};
    case Page::slot: return std::string{slot_name(parameter(state), session.locale)};
    case Page::effects:
        return "FX " + std::to_string(parameter(state) / 3 + 1) + " " +
            std::string{effect_field(parameter(state), ru(session))};
    case Page::master:
        return parameter(state) == 0 ? std::string{cd::tr(session.locale, cd::TextId::master)} :
            (ru(session) ? "ТЕМП" : "TEMPO");
    }
    return {};
}

void adjust(cd::Session& session, UiState& state, float steps) {
    const int selected = parameter(state);
    switch (state.page) {
    case Page::perform: {
        float* value = macro_value(session.performance, selected);
        *value = std::clamp(*value + steps * 0.01F, 0.0F, 1.0F);
        if (selected == 4) {
            state.auto_fade = false;
        }
        break;
    }
    case Page::slot: {
        float* value = slot_value(session.slots[static_cast<std::size_t>(state.slot)], selected);
        if (selected == 0) {
            *value = std::clamp(*value * std::pow(2.0F, steps / 12.0F), 8.0F, 2'000.0F);
        } else if (selected == 6) {
            *value = std::clamp(*value + steps * 0.02F, -1.0F, 1.0F);
        } else {
            *value = std::clamp(*value + steps * 0.01F, 0.0F, 1.0F);
        }
        break;
    }
    case Page::effects: {
        auto& effect = session.slots[static_cast<std::size_t>(state.slot)]
            .effects[static_cast<std::size_t>(selected / 3)];
        float* value = effect_value(effect, selected % 3);
        *value = std::clamp(*value + steps * 0.01F, 0.0F, 1.0F);
        break;
    }
    case Page::master:
        if (selected == 0) {
            session.master_level = std::clamp(session.master_level + steps * 0.01F, 0.0F, 1.0F);
        } else {
            session.tempo_bpm = std::clamp(session.tempo_bpm + steps, 10.0F, 300.0F);
        }
        break;
    }
}

void start_adjust(cd::Session& session, UiState& state, int direction, Uint32 now) {
    state.held_direction = direction;
    state.held_since = now;
    state.last_repeat = now;
    adjust(session, state, static_cast<float>(direction));
}

bool repeat_adjust(cd::Session& session, UiState& state, Uint32 now) {
    if (state.held_direction == 0 || now - state.held_since < 330U) {
        return false;
    }
    const Uint32 elapsed = now - state.held_since;
    const Uint32 interval = elapsed >= 2'200U ? 16U : (elapsed >= 1'050U ? 35U : 70U);
    const float multiplier = elapsed >= 2'200U ? 4.0F : (elapsed >= 1'050U ? 2.0F : 1.0F);
    if (now - state.last_repeat < interval) {
        return false;
    }
    state.last_repeat = now;
    adjust(session, state, static_cast<float>(state.held_direction) * multiplier);
    return true;
}

void stop_adjust(UiState& state, int direction) noexcept {
    if (state.held_direction == direction) {
        state.held_direction = 0;
    }
}

void update_title(SDL_Window* window, const cd::Session& session, const UiState& state) {
    const std::string title = std::string{cd::tr(session.locale, cd::TextId::app_name)} + " - " +
        std::string{page_name(state.page, ru(session))} + " - " + current_label(session, state) + " " +
        value_text(session, state);
    SDL_SetWindowTitle(window, title.c_str());
}

void fill(SDL_Renderer* renderer, SDL_Rect rectangle, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rectangle);
}

void outline(SDL_Renderer* renderer, SDL_Rect rectangle, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(renderer, &rectangle);
}

SDL_Color react(SDL_Color color, float activity) noexcept {
    const float gain = 0.70F + std::clamp(activity, 0.0F, 1.0F) * 0.65F;
    return {
        static_cast<Uint8>(std::clamp(static_cast<float>(color.r) * gain, 0.0F, 255.0F)),
        static_cast<Uint8>(std::clamp(static_cast<float>(color.g) * gain, 0.0F, 255.0F)),
        static_cast<Uint8>(std::clamp(static_cast<float>(color.b) * gain, 0.0F, 255.0F)),
        color.a,
    };
}

void bar(SDL_Renderer* renderer, int x, int y, int width, int height, float value, SDL_Color color) {
    value = std::clamp(value, 0.0F, 1.0F);
    fill(renderer, {x, y, width, height}, {18, 15, 24, 255});
    fill(renderer, {x, y, static_cast<int>(std::lround(static_cast<float>(width) * value)), height}, color);
}

void scope(
    SDL_Renderer* renderer, int x, int y, int width, int height, float rms, float peak, float phase, int seed) {
    outline(renderer, {x, y, width, height}, {105, 92, 118, 255});
    const float level = std::clamp(rms * 4.2F, 0.0F, 1.0F);
    const int filled = static_cast<int>(std::lround(level * static_cast<float>(height - 4)));
    fill(renderer, {x + 2, y + height - filled - 2, width - 4, filled}, react(kInk, level));
    const int peak_y = y + height - 2 - static_cast<int>(std::lround(
        std::clamp(peak * 2.4F, 0.0F, 1.0F) * static_cast<float>(height - 4)));
    SDL_SetRenderDrawColor(renderer, 225, 77, 96, 255);
    SDL_RenderDrawLine(renderer, x + 2, peak_y, x + width - 3, peak_y);
    SDL_SetRenderDrawColor(renderer, 16, 12, 22, 210);
    int previous = y + height / 2;
    for (int column = 3; column < width - 4; column += 3) {
        const float wave = std::sin(
            phase * 6.2831853F + static_cast<float>(column) * 0.21F + static_cast<float>(seed));
        const int current = y + height / 2 +
            static_cast<int>(wave * level * static_cast<float>(height) * 0.36F);
        SDL_RenderDrawLine(renderer, x + column - 1, previous, x + column + 2, current);
        previous = current;
    }
}

float effective_amount(const cd::Session& session, const cd::EffectSettings& effect) noexcept {
    float value = effect.amount;
    if (effect.kind == cd::EffectKind::drive || effect.kind == cd::EffectKind::crusher) {
        value += session.performance.texture * session.performance.texture * 0.42F;
    } else if (effect.kind == cd::EffectKind::tremolo) {
        value += session.performance.pulse * session.performance.pulse * 0.20F;
    } else if (effect.kind == cd::EffectKind::delay) {
        value += session.performance.space * session.performance.space * 0.58F;
    }
    return std::clamp(value, 0.0F, 1.0F);
}

void draw_header(SDL_Renderer* renderer, const cd::Session& session, Page active) {
    cd::ui::draw_text(renderer, 10, 5, cd::tr(session.locale, cd::TextId::app_name), kInk, 2);
    const auto active_name = page_name(active, ru(session));
    cd::ui::draw_text(renderer, kWidth - 10 - cd::ui::text_width(active_name), 9, active_name, kDim);
    constexpr std::array<Page, 4> pages{Page::perform, Page::slot, Page::effects, Page::master};
    for (int index = 0; index < 4; ++index) {
        const int x = 10 + index * 124;
        const bool selected = pages[static_cast<std::size_t>(index)] == active;
        if (selected) {
            fill(renderer, {x, 26, 120, 16}, kPurple);
        } else {
            outline(renderer, {x, 26, 120, 16}, {58, 49, 72, 255});
        }
        cd::ui::draw_text(renderer, x + 5, 30, page_name(pages[static_cast<std::size_t>(index)], ru(session)),
            selected ? kInk : kDim);
    }
}

void draw_tracks(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state) {
    constexpr int panel_width = 117;
    for (int index = 0; index < 4; ++index) {
        const int x = 10 + index * 125;
        const float activity = std::clamp(telemetry.slot_rms[static_cast<std::size_t>(index)] * 5.0F, 0.0F, 1.0F);
        fill(renderer, {x, 48, panel_width, 306},
            index == state.slot ? react(kPurple, activity * 0.35F) : react(kPanel, activity * 0.45F));
        if (index == state.slot) {
            outline(renderer, {x + 1, 49, panel_width - 2, 304}, kInk);
        }
        const std::string title = std::string{ru(session) ? "СЛОТ " : "SLOT "} + std::to_string(index + 1);
        cd::ui::draw_text(renderer, x + 7, 56, title, kInk);
        const std::string style = std::string{ru(session) ? "ТЕСТ " : "TEST "} + static_cast<char>('A' + index);
        cd::ui::draw_text(renderer, x + 7, 69, style, kDim);
        scope(renderer, x + 8, 84, panel_width - 16, 103,
            telemetry.slot_rms[static_cast<std::size_t>(index)],
            telemetry.slot_peak[static_cast<std::size_t>(index)],
            telemetry.pulse_phase, index);
        const auto label = state.page == Page::perform
            ? macro_name(parameter(state), ru(session))
            : slot_name(parameter(state), session.locale);
        cd::ui::draw_text(renderer, x + 7, 197, label, kDim);
        cd::ui::draw_text(renderer, x + 7, 211,
            state.page == Page::perform ? value_text(session, state) : value_text(session, state, index), kInk);
        const auto& slot = session.slots[static_cast<std::size_t>(index)];
        for (int fx = 0; fx < 4; ++fx) {
            const int y = 238 + fx * 25;
            const auto& effect = slot.effects[static_cast<std::size_t>(fx)];
            const SDL_Color color = react(kFxColors[static_cast<std::size_t>(fx)], activity);
            cd::ui::draw_text(renderer, x + 7, y, effect_name(effect.kind, ru(session)), color);
            bar(renderer, x + 7, y + 11, panel_width - 14, 7, effective_amount(session, effect), color);
        }
        if (!slot.enabled) {
            fill(renderer, {x, 48, panel_width, 306}, {0, 0, 0, 168});
            cd::ui::draw_text(renderer, x + 7, 132, ru(session) ? "MUTE / ХВОСТ" : "MUTE / TAIL",
                {239, 112, 112, 255});
        }
    }
}

void draw_effects(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state) {
    constexpr int panel_width = 117;
    const int active_effect = parameter(state) / 3;
    const int active_field = parameter(state) % 3;
    const auto& slot = session.slots[static_cast<std::size_t>(state.slot)];
    for (int index = 0; index < 4; ++index) {
        const int x = 10 + index * 125;
        fill(renderer, {x, 48, panel_width, 306}, index == active_effect ? kPurple : kPanel);
        if (index == active_effect) {
            outline(renderer, {x + 1, 49, panel_width - 2, 304}, kInk);
        }
        const std::string title = "FX " + std::to_string(index + 1);
        const auto& effect = slot.effects[static_cast<std::size_t>(index)];
        cd::ui::draw_text(renderer, x + 7, 57, title, kInk);
        cd::ui::draw_text(renderer, x + 7, 72, effect_name(effect.kind, ru(session)),
            kFxColors[static_cast<std::size_t>(index)]);
        scope(renderer, x + 8, 91, panel_width - 16, 78,
            telemetry.slot_rms[static_cast<std::size_t>(state.slot)],
            telemetry.slot_peak[static_cast<std::size_t>(state.slot)],
            telemetry.pulse_phase, state.slot + index);
        for (int field = 0; field < 3; ++field) {
            const int y = 188 + field * 49;
            const bool selected = index == active_effect && field == active_field;
            cd::ui::draw_text(renderer, x + 7, y, effect_field(field, ru(session)), selected ? kInk : kDim);
            bar(renderer, x + 7, y + 14, panel_width - 14, 10, effect_value(effect, field),
                selected ? kInk : kFxColors[static_cast<std::size_t>(index)]);
            char number[12]{};
            std::snprintf(number, sizeof(number), "%d%%", static_cast<int>(std::lround(
                effect_value(effect, field) * 100.0F)));
            cd::ui::draw_text(renderer, x + 7, y + 28, number, selected ? kInk : kDim);
        }
    }
    const std::string hint = std::string{ru(session) ? "СЛОТ " : "SLOT "} + std::to_string(state.slot + 1) +
        (ru(session) ? "  E/START: ТИП" : "  E/START: TYPE");
    cd::ui::draw_text(renderer, 12, 357, hint, kDim);
}

void draw_master(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state) {
    fill(renderer, {10, 48, 492, 306}, kPanel);
    cd::ui::draw_text(renderer, 24, 60, ru(session) ? "ИТОГОВЫЙ СИГНАЛ" : "MASTER SIGNAL", kDim);
    bar(renderer, 24, 80, 464, 36, telemetry.master_rms * 3.3F, react(kInk, telemetry.master_rms * 5.0F));
    const int peak_x = 24 + static_cast<int>(std::lround(std::clamp(telemetry.master_peak, 0.0F, 1.0F) * 464.0F));
    SDL_SetRenderDrawColor(renderer, 226, 87, 104, 255);
    SDL_RenderDrawLine(renderer, peak_x, 78, peak_x, 119);
    const std::array<std::string, 3> labels{
        std::string{cd::tr(session.locale, cd::TextId::master)},
        ru(session) ? "ТЕМП" : "TEMPO",
        ru(session) ? "ФЕЙД ВЫХОДА" : "OUTPUT FADE",
    };
    const std::array<float, 3> values{
        session.master_level, (session.tempo_bpm - 10.0F) / 290.0F, session.performance.fade};
    for (int index = 0; index < 3; ++index) {
        const int y = 145 + index * 55;
        const bool selected = index < 2 && parameter(state) == index;
        cd::ui::draw_text(renderer, 24, y, labels[static_cast<std::size_t>(index)], selected ? kInk : kDim);
        bar(renderer, 160, y, 328, 15, values[static_cast<std::size_t>(index)], selected ? kInk : kPurple);
        char number[24]{};
        if (index == 1) {
            std::snprintf(number, sizeof(number), "%.0f BPM", static_cast<double>(session.tempo_bpm));
        } else {
            std::snprintf(number, sizeof(number), "%d%%", static_cast<int>(std::lround(
                values[static_cast<std::size_t>(index)] * 100.0F)));
        }
        cd::ui::draw_text(renderer, 24, y + 20, number, kInk);
    }
    if (state.auto_fade) {
        cd::ui::draw_text(renderer, 292, 286, ru(session) ? "АВТОФЕЙД" : "AUTO FADE", {91, 218, 179, 255});
    }
    for (int slot = 0; slot < 4; ++slot) {
        const float value = telemetry.slot_rms[static_cast<std::size_t>(slot)] * 4.0F;
        bar(renderer, 24 + slot * 116, 326, 104, 10, value, react(kFxColors[static_cast<std::size_t>(slot)], value));
    }
}

void draw(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state,
    Uint32 now) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 8, 7, 12, 255);
    SDL_RenderClear(renderer);
    draw_header(renderer, session, state.page);
    if (state.page == Page::perform || state.page == Page::slot) {
        draw_tracks(renderer, session, telemetry, state);
    } else if (state.page == Page::effects) {
        draw_effects(renderer, session, telemetry, state);
    } else {
        draw_master(renderer, session, telemetry, state);
    }
    const std::string help = ru(session)
        ? "TAB ЭКРАН  A/D ЗНАЧ.  SPACE MUTE  F ФЕЙД  K KILL"
        : "TAB PAGE  A/D VALUE  SPACE MUTE  F FADE  K KILL";
    cd::ui::draw_text(renderer, 10, 370, help, kDim);
    if (state.held_direction != 0 && now - state.held_since >= 1'050U) {
        cd::ui::draw_text(renderer, 466, 357, now - state.held_since >= 2'200U ? ">>>" : ">>", {239, 169, 80, 255});
    }
    if (now < state.kill_flash_until) {
        outline(renderer, {2, 2, kWidth - 4, kHeight - 4}, {242, 70, 82, 255});
        cd::ui::draw_text(renderer, 206, 180, ru(session) ? "KILL: ТИШИНА" : "KILL: SILENCE", {242, 70, 82, 255});
    }
    SDL_RenderPresent(renderer);
}

void toggle_fade(cd::Session& session, UiState& state) noexcept {
    state.fade_target = session.performance.fade > 0.5F ? 0.0F : 1.0F;
    state.auto_fade = true;
}

bool update_fade(cd::Session& session, UiState& state, float seconds) noexcept {
    if (!state.auto_fade) {
        return false;
    }
    const float amount = std::clamp(seconds, 0.0F, 0.1F) * 0.25F;
    float& fade = session.performance.fade;
    fade = fade < state.fade_target
        ? std::min(state.fade_target, fade + amount)
        : std::max(state.fade_target, fade - amount);
    if (fade == state.fade_target) {
        state.auto_fade = false;
    }
    return true;
}

void cycle_effect(cd::Session& session, const UiState& state) noexcept {
    if (state.page != Page::effects) {
        return;
    }
    auto& kind = session.slots[static_cast<std::size_t>(state.slot)]
        .effects[static_cast<std::size_t>(parameter(state) / 3)].kind;
    kind = static_cast<cd::EffectKind>((static_cast<int>(kind) + 1) % 6);
}

} // namespace

int main(int, char**) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow(
        "Cursed Drone", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 768,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer* renderer = window != nullptr
        ? SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)
        : nullptr;
    if (window == nullptr || renderer == nullptr) {
        std::fprintf(stderr, "SDL video: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(renderer, kWidth, kHeight);

    cd::Session session = cd::make_default_session();
    AudioBridge audio{};
    SDL_AudioSpec desired{};
    SDL_AudioSpec obtained{};
    desired.freq = 48'000;
    desired.format = AUDIO_F32SYS;
    desired.channels = 2;
    desired.samples = 512;
    desired.callback = audio_callback;
    desired.userdata = &audio;
    const SDL_AudioDeviceID device = SDL_OpenAudioDevice(
        nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (device == 0 || obtained.format != AUDIO_F32SYS || obtained.channels != 2) {
        std::fprintf(stderr, "SDL audio: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    audio.graph.prepare({static_cast<float>(obtained.freq), obtained.samples}, session);
    SDL_PauseAudioDevice(device, 0);

    SDL_GameController* controller = nullptr;
    for (int joystick = 0; joystick < SDL_NumJoysticks(); ++joystick) {
        if (SDL_IsGameController(joystick) == SDL_TRUE) {
            controller = SDL_GameControllerOpen(joystick);
            break;
        }
    }

    UiState state{};
    bool running = true;
    Uint32 previous_frame = SDL_GetTicks();
    update_title(window, session, state);
    while (running) {
        bool changed = false;
        SDL_Event event{};
        while (SDL_PollEvent(&event) != 0) {
            const Uint32 now = SDL_GetTicks();
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE: running = false; break;
                case SDLK_LEFT: state.slot = (state.slot + 3) % 4; changed = true; break;
                case SDLK_RIGHT: state.slot = (state.slot + 1) % 4; changed = true; break;
                case SDLK_UP:
                    parameter(state) = (parameter(state) + parameter_count(state.page) - 1) % parameter_count(state.page);
                    changed = true;
                    break;
                case SDLK_DOWN:
                    parameter(state) = (parameter(state) + 1) % parameter_count(state.page);
                    changed = true;
                    break;
                case SDLK_a: start_adjust(session, state, -1, now); changed = true; break;
                case SDLK_d: start_adjust(session, state, 1, now); changed = true; break;
                case SDLK_TAB:
                    state.page = static_cast<Page>((page_index(state.page) + 1) % 4);
                    changed = true;
                    break;
                case SDLK_1: state.page = Page::perform; changed = true; break;
                case SDLK_2: state.page = Page::slot; changed = true; break;
                case SDLK_3: state.page = Page::effects; changed = true; break;
                case SDLK_4: state.page = Page::master; changed = true; break;
                case SDLK_e: cycle_effect(session, state); changed = true; break;
                case SDLK_f: toggle_fade(session, state); changed = true; break;
                case SDLK_k:
                    audio.graph.panic();
                    state.kill_flash_until = now + 700U;
                    break;
                case SDLK_l:
                    session.locale = session.locale == cd::Locale::ru ? cd::Locale::en : cd::Locale::ru;
                    changed = true;
                    break;
                case SDLK_SPACE:
                    session.slots[static_cast<std::size_t>(state.slot)].enabled =
                        !session.slots[static_cast<std::size_t>(state.slot)].enabled;
                    changed = true;
                    break;
                default: break;
                }
            } else if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_a) stop_adjust(state, -1);
                if (event.key.keysym.sym == SDLK_d) stop_adjust(state, 1);
            } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                switch (event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT: state.slot = (state.slot + 3) % 4; changed = true; break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: state.slot = (state.slot + 1) % 4; changed = true; break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                    parameter(state) = (parameter(state) + parameter_count(state.page) - 1) % parameter_count(state.page);
                    changed = true;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    parameter(state) = (parameter(state) + 1) % parameter_count(state.page);
                    changed = true;
                    break;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: start_adjust(session, state, -1, now); changed = true; break;
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: start_adjust(session, state, 1, now); changed = true; break;
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
                    state.page = static_cast<Page>((page_index(state.page) + 1) % 4);
                    changed = true;
                    break;
                case SDL_CONTROLLER_BUTTON_Y:
                    session.locale = session.locale == cd::Locale::ru ? cd::Locale::en : cd::Locale::ru;
                    changed = true;
                    break;
                case SDL_CONTROLLER_BUTTON_BACK: toggle_fade(session, state); changed = true; break;
                case SDL_CONTROLLER_BUTTON_START: cycle_effect(session, state); changed = true; break;
                default: break;
                }
            } else if (event.type == SDL_CONTROLLERBUTTONUP) {
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER) stop_adjust(state, -1);
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) stop_adjust(state, 1);
            }
        }
        const Uint32 now = SDL_GetTicks();
        changed = repeat_adjust(session, state, now) || changed;
        changed = update_fade(session, state, static_cast<float>(now - previous_frame) * 0.001F) || changed;
        previous_frame = now;
        if (changed) {
            static_cast<void>(audio.graph.submit_session(session));
            update_title(window, session, state);
        }
        draw(renderer, session, audio.graph.telemetry(), state, now);
        SDL_Delay(8);
    }

    SDL_CloseAudioDevice(device);
    if (controller != nullptr) SDL_GameControllerClose(controller);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

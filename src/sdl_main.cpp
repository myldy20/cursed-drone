// SPDX-License-Identifier: GPL-3.0-or-later
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "bitmap_text.hpp"
#include "cursed_drone/audio.hpp"
#include "cursed_drone/i18n.hpp"
#include "cursed_drone/session.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <filesystem>
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

enum class Page { perform, slot, effects, master, setup };

struct AudioBridge {
    cd::AudioGraph graph{};
    std::atomic<float> cpu_load{0.0F};
    float sample_rate{48'000.0F};
};

struct UiState {
    Page page{Page::perform};
    int slot{0};
    std::array<int, 5> selected{};
    int effect_field{0};
    int held_direction{0};
    Uint32 held_since{0};
    Uint32 last_repeat{0};
    bool auto_fade{false};
    float fade_target{1.0F};
    Uint32 kill_flash_until{0};
};

void audio_callback(void* userdata, Uint8* bytes, int byte_count) {
    auto& bridge = *static_cast<AudioBridge*>(userdata);
    const Uint64 started = SDL_GetPerformanceCounter();
    auto* frames = reinterpret_cast<cd::StereoFrame*>(bytes);
    const auto frame_count = static_cast<std::size_t>(byte_count) / sizeof(cd::StereoFrame);
    bridge.graph.process({frames, frame_count});
    const double elapsed = static_cast<double>(SDL_GetPerformanceCounter() - started) /
        static_cast<double>(SDL_GetPerformanceFrequency());
    const double available = static_cast<double>(frame_count) / static_cast<double>(bridge.sample_rate);
    const float load = static_cast<float>(std::clamp(elapsed / available, 0.0, 4.0));
    const float previous = bridge.cpu_load.load(std::memory_order_relaxed);
    bridge.cpu_load.store(previous + (load - previous) * 0.12F, std::memory_order_relaxed);
}

bool ru(const cd::Session& session) noexcept { return session.locale == cd::Locale::ru; }

std::string_view page_name(Page page, bool russian) noexcept {
    switch (page) {
    case Page::perform: return russian ? "СЦЕНА" : "SCENE";
    case Page::slot: return russian ? "СЛОТЫ" : "SLOTS";
    case Page::effects: return "FX";
    case Page::master: return russian ? "МАСТЕР" : "MASTER";
    case Page::setup: return russian ? "НАСТР." : "SETUP";
    }
    return {};
}

std::string_view macro_name(int index, bool russian) noexcept {
    constexpr std::array<std::string_view, 5> r{
        "ТЕКСТУРА", "ПУЛЬС", "ХАОС", "ПРОСТРАНСТВО", "СОБЫТИЯ"};
    constexpr std::array<std::string_view, 5> e{
        "TEXTURE", "PULSE", "CHAOS", "SPACE", "EVENTS"};
    return (russian ? r : e)[static_cast<std::size_t>(index)];
}

std::string_view engine_name(cd::EngineKind kind, bool russian) noexcept {
    switch (kind) {
    case cd::EngineKind::diagnostic: return russian ? "ДИАГН." : "LEGACY";
    case cd::EngineKind::macro: return russian ? "ТОН" : "TONE";
    case cd::EngineKind::body: return russian ? "РЕЗОНАТОР" : "RESONATOR";
    case cd::EngineKind::grain: return russian ? "ЗЕРНО" : "GRAINLET";
    case cd::EngineKind::particle: return russian ? "ЧАСТИЦЫ" : "PARTICLES";
    }
    return {};
}

std::string_view slot_name(int index, const cd::SlotSettings& slot, cd::Locale locale) {
    const bool russian = locale == cd::Locale::ru;
    if (index == 0) return cd::tr(locale, cd::TextId::engine);
    if (index == 1) return cd::tr(locale, cd::TextId::frequency);
    if (index == 6) return cd::tr(locale, cd::TextId::level);
    if (index == 7) return cd::tr(locale, cd::TextId::pan);
    const int character = index - 2;
    switch (slot.engine) {
    case cd::EngineKind::macro: {
        constexpr std::array<std::string_view, 4> r{"ФОРМА", "РАССТРОЙКА", "ДРЕЙФ", "ШУМ"};
        constexpr std::array<std::string_view, 4> e{"SHAPE", "DETUNE", "DRIFT", "NOISE"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::body: {
        constexpr std::array<std::string_view, 4> r{"МАТЕРИАЛ", "ЯРКОСТЬ", "УДАРЫ", "ВОЗБУЖД."};
        constexpr std::array<std::string_view, 4> e{"MATERIAL", "BRIGHT", "STRIKES", "EXCITE"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::grain: {
        constexpr std::array<std::string_view, 4> r{"ФОРМАНТА", "ФОРМА", "РОССЫПЬ", "ЗЕРНО"};
        constexpr std::array<std::string_view, 4> e{"FORMANT", "SHAPE", "SCATTER", "GRAIN"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::particle: {
        constexpr std::array<std::string_view, 4> r{"РЕЗОНАНС", "РАЗБРОС", "ДВИЖЕНИЕ", "ПЛОТНОСТЬ"};
        constexpr std::array<std::string_view, 4> e{"RESONANCE", "SPREAD", "MOTION", "DENSITY"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::diagnostic:
        break;
    }
    constexpr std::array ids{cd::TextId::timbre, cd::TextId::color, cd::TextId::motion, cd::TextId::texture};
    return cd::tr(locale, ids[static_cast<std::size_t>(character)]);
}

std::string_view effect_name(cd::EffectKind kind, bool russian) noexcept {
    switch (kind) {
    case cd::EffectKind::bypass: return russian ? "ОБХОД" : "BYPASS";
    case cd::EffectKind::drive: return russian ? "ДРАЙВ" : "DRIVE";
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
    case Page::slot: return 8;
    case Page::effects: return 4;
    case Page::master: return 3;
    case Page::setup: return 3;
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
    default: return settings.events;
    }
}

float* macro_value(cd::PerformanceSettings& settings, int index) noexcept {
    switch (index) {
    case 0: return &settings.texture;
    case 1: return &settings.pulse;
    case 2: return &settings.chaos;
    case 3: return &settings.space;
    default: return &settings.events;
    }
}

float slot_value(const cd::SlotSettings& slot, int index) noexcept {
    switch (index) {
    case 1: return slot.frequency_hz;
    case 2: return slot.timbre;
    case 3: return slot.color;
    case 4: return slot.motion;
    case 5: return slot.texture;
    case 6: return slot.level;
    default: return slot.pan;
    }
}

float* slot_value(cd::SlotSettings& slot, int index) noexcept {
    switch (index) {
    case 1: return &slot.frequency_hz;
    case 2: return &slot.timbre;
    case 3: return &slot.color;
    case 4: return &slot.motion;
    case 5: return &slot.texture;
    case 6: return &slot.level;
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
        if (selected == 0) {
            return std::string{engine_name(
                session.slots[static_cast<std::size_t>(slot)].engine, ru(session))};
        }
        const float value = slot_value(session.slots[static_cast<std::size_t>(slot)], selected);
        if (selected == 1) {
            std::snprintf(result, sizeof(result), "%.1f Hz", static_cast<double>(value));
        } else if (selected == 7) {
            std::snprintf(result, sizeof(result), "%+.2f", static_cast<double>(value));
        } else {
            std::snprintf(result, sizeof(result), "%d%%", static_cast<int>(std::lround(value * 100.0F)));
        }
        break;
    }
    case Page::effects: {
        const auto& effect = session.slots[static_cast<std::size_t>(state.slot)]
            .effects[static_cast<std::size_t>(selected)];
        std::snprintf(result, sizeof(result), "%d%%", static_cast<int>(std::lround(
            effect_value(effect, state.effect_field) * 100.0F)));
        break;
    }
    case Page::master:
        if (selected == 0) {
            std::snprintf(result, sizeof(result), "%d%%", static_cast<int>(std::lround(
                session.master_level * 100.0F)));
        } else if (selected == 1) {
            std::snprintf(result, sizeof(result), "%.0f BPM", static_cast<double>(session.tempo_bpm));
        } else {
            std::snprintf(result, sizeof(result), "%d%%", static_cast<int>(std::lround(
                session.performance.fade * 100.0F)));
        }
        break;
    case Page::setup:
        if (selected == 0) {
            return session.locale == cd::Locale::ru ? "РУССКИЙ" : "ENGLISH";
        }
        std::snprintf(result, sizeof(result), "%.2f s", static_cast<double>(
            selected == 1 ? session.fade_in_seconds : session.fade_out_seconds));
        break;
    }
    return result;
}

std::string current_label(const cd::Session& session, const UiState& state) {
    switch (state.page) {
    case Page::perform: return std::string{macro_name(parameter(state), ru(session))};
    case Page::slot: return std::string{slot_name(
        parameter(state), session.slots[static_cast<std::size_t>(state.slot)], session.locale)};
    case Page::effects:
        return "FX " + std::to_string(parameter(state) + 1) + " " +
            std::string{effect_field(state.effect_field, ru(session))};
    case Page::master:
        if (parameter(state) == 0) return std::string{cd::tr(session.locale, cd::TextId::master)};
        if (parameter(state) == 1) return ru(session) ? "ТЕМП" : "TEMPO";
        return ru(session) ? "ФЕЙД ВЫХОДА" : "OUTPUT FADE";
    case Page::setup:
        if (parameter(state) == 0) return std::string{cd::tr(session.locale, cd::TextId::language)};
        return parameter(state) == 1
            ? (ru(session) ? "ФЕЙД ВХОДА" : "FADE IN")
            : (ru(session) ? "ФЕЙД ВЫХОДА" : "FADE OUT");
    }
    return {};
}

void adjust(cd::Session& session, UiState& state, float steps) {
    const int selected = parameter(state);
    switch (state.page) {
    case Page::perform: {
        float* value = macro_value(session.performance, selected);
        *value = std::clamp(*value + steps * 0.01F, 0.0F, 1.0F);
        break;
    }
    case Page::slot: {
        if (selected == 0) {
            constexpr std::array engines{
                cd::EngineKind::macro, cd::EngineKind::body,
                cd::EngineKind::grain, cd::EngineKind::particle};
            auto& engine = session.slots[static_cast<std::size_t>(state.slot)].engine;
            auto found = std::find(engines.begin(), engines.end(), engine);
            int index = found == engines.end() ? 0 : static_cast<int>(found - engines.begin());
            index = (index + (steps > 0.0F ? 1 : 3)) % 4;
            engine = engines[static_cast<std::size_t>(index)];
            break;
        }
        float* value = slot_value(session.slots[static_cast<std::size_t>(state.slot)], selected);
        if (selected == 1) {
            *value = std::clamp(*value * std::pow(2.0F, steps / 12.0F), 8.0F, 2'000.0F);
        } else if (selected == 7) {
            *value = std::clamp(*value + steps * 0.02F, -1.0F, 1.0F);
        } else {
            *value = std::clamp(*value + steps * 0.01F, 0.0F, 1.0F);
        }
        break;
    }
    case Page::effects: {
        auto& effect = session.slots[static_cast<std::size_t>(state.slot)]
            .effects[static_cast<std::size_t>(selected)];
        float* value = effect_value(effect, state.effect_field);
        *value = std::clamp(*value + steps * 0.01F, 0.0F, 1.0F);
        break;
    }
    case Page::master:
        if (selected == 0) {
            session.master_level = std::clamp(session.master_level + steps * 0.01F, 0.0F, 1.0F);
        } else if (selected == 1) {
            session.tempo_bpm = std::clamp(session.tempo_bpm + steps, 10.0F, 300.0F);
        } else {
            session.performance.fade = std::clamp(session.performance.fade + steps * 0.01F, 0.0F, 1.0F);
            state.auto_fade = false;
        }
        break;
    case Page::setup:
        if (selected == 0) {
            session.locale = session.locale == cd::Locale::ru ? cd::Locale::en : cd::Locale::ru;
        } else {
            float& seconds = selected == 1 ? session.fade_in_seconds : session.fade_out_seconds;
            seconds = std::clamp(seconds + steps * 0.25F, 0.25F, 30.0F);
        }
        break;
    }
}

void start_adjust(cd::Session& session, UiState& state, int direction, Uint32 now) {
    state.held_direction = direction;
    state.held_since = now;
    state.last_repeat = now;
    adjust(session, state, static_cast<float>(direction));
    if ((state.page == Page::slot && parameter(state) == 0) ||
        (state.page == Page::setup && parameter(state) == 0)) {
        state.held_direction = 0;
    }
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
    SDL_Renderer* renderer,
    int x,
    int y,
    int width,
    int height,
    const std::array<float, cd::kScopePointCount>& samples,
    float rms,
    float peak) {
    outline(renderer, {x, y, width, height}, {105, 92, 118, 255});
    fill(renderer, {x + 1, y + 1, width - 2, height - 2}, {18, 15, 24, 255});
    const int meter_height = 7;
    const int wave_height = height - meter_height - 3;
    const int center = y + wave_height / 2 + 1;
    SDL_SetRenderDrawColor(renderer, 63, 55, 74, 255);
    SDL_RenderDrawLine(renderer, x + 2, center, x + width - 3, center);
    const float visual_gain = std::min(8.0F, 0.86F / std::max(peak, 0.04F));
    SDL_SetRenderDrawColor(renderer, kInk.r, kInk.g, kInk.b, kInk.a);
    int previous = center;
    for (int column = 2; column < width - 2; ++column) {
        const std::size_t point = std::min(
            cd::kScopePointCount - 1U,
            static_cast<std::size_t>(column - 2) * cd::kScopePointCount /
                static_cast<std::size_t>(std::max(1, width - 4)));
        const float sample = std::clamp(samples[point] * visual_gain, -1.0F, 1.0F);
        const int current = center - static_cast<int>(std::lround(
            sample * static_cast<float>(wave_height - 5) * 0.5F));
        SDL_RenderDrawLine(renderer, x + column - 1, previous, x + column, current);
        previous = current;
    }
    const int meter_y = y + height - meter_height - 1;
    const float rms_meter = std::clamp(rms * 4.2F, 0.0F, 1.0F);
    fill(renderer, {x + 2, meter_y, width - 4, meter_height - 1}, {35, 30, 46, 255});
    fill(renderer, {x + 2, meter_y, static_cast<int>(std::lround(
        static_cast<float>(width - 4) * rms_meter)), meter_height - 1},
        react(kInk, rms_meter));
    const int peak_x = x + 2 + static_cast<int>(std::lround(
        static_cast<float>(width - 5) * std::clamp(peak * 2.4F, 0.0F, 1.0F)));
    SDL_SetRenderDrawColor(renderer, 225, 77, 96, 255);
    SDL_RenderDrawLine(renderer, peak_x, meter_y - 1, peak_x, meter_y + meter_height - 1);
}

void draw_header(SDL_Renderer* renderer, const cd::Session& session, const UiState& state) {
    cd::ui::draw_text(renderer, 10, 5, cd::tr(session.locale, cd::TextId::app_name), kInk, 2);
    std::string status{page_name(state.page, ru(session))};
    SDL_Color status_color = kDim;
    if (state.auto_fade) {
        status = std::string{ru(session) ? "ФЕЙД " : "FADE "} +
            (state.fade_target > session.performance.fade ? "↑ " : "↓ ") +
            std::to_string(static_cast<int>(std::lround(session.performance.fade * 100.0F))) + "%";
        status_color = {91, 218, 179, 255};
    }
    cd::ui::draw_text(renderer, kWidth - 10 - cd::ui::text_width(status), 9, status, status_color);
    constexpr std::array<Page, 5> pages{
        Page::perform, Page::slot, Page::effects, Page::master, Page::setup};
    for (int index = 0; index < 5; ++index) {
        const int x = 10 + index * 99;
        const bool selected = pages[static_cast<std::size_t>(index)] == state.page;
        if (selected) {
            fill(renderer, {x, 26, 95, 16}, kPurple);
        } else {
            outline(renderer, {x, 26, 95, 16}, {58, 49, 72, 255});
        }
        cd::ui::draw_text(renderer, x + 4, 30, page_name(pages[static_cast<std::size_t>(index)], ru(session)),
            selected ? kInk : kDim);
    }
}

std::string_view macro_description(int index, bool russian) noexcept {
    constexpr std::array<std::string_view, 5> r{
        "ГАРМОНИКИ · ФИЛЬТР · DRIVE",
        "ОГИБАЮЩАЯ · ТРЕМОЛО · СЛОИ",
        "ВЫСОТА · УРОВЕНЬ · ПАНОРАМА",
        "ДЕЛЕЙ · ОБР.СВЯЗЬ · СТЕРЕО",
        "УДАРЫ · ПАЧКИ · ПАУЗЫ",
    };
    constexpr std::array<std::string_view, 5> e{
        "HARMONICS · FILTER · DRIVE",
        "ENVELOPE · TREMOLO · LAYERS",
        "PITCH · LEVEL · PAN · CHANCE",
        "DELAY · FEEDBACK · STEREO",
        "STRIKES · BURSTS · GAPS",
    };
    return (russian ? r : e)[static_cast<std::size_t>(index)];
}

void draw_scene(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state) {
    fill(renderer, {10, 48, 492, 306}, kPanel);
    for (int index = 0; index < 5; ++index) {
        const int y = 58 + index * 56;
        const bool selected = parameter(state) == index;
        if (selected) {
            fill(renderer, {18, y - 5, 308, 51}, {73, 46, 104, 255});
            outline(renderer, {18, y - 5, 308, 51}, kInk);
        }
        cd::ui::draw_text(renderer, 26, y, macro_name(index, ru(session)), selected ? kInk : kDim);
        char number[12]{};
        std::snprintf(number, sizeof(number), "%d%%", static_cast<int>(std::lround(
            macro_value(session.performance, index) * 100.0F)));
        cd::ui::draw_text(renderer, 294 - cd::ui::text_width(number), y, number, selected ? kInk : kDim);
        bar(renderer, 26, y + 14, 276, 9, macro_value(session.performance, index),
            selected ? kInk : kFxColors[static_cast<std::size_t>(index % 4)]);
        cd::ui::draw_text(renderer, 26, y + 29, macro_description(index, ru(session)), selected ? kInk : kDim);
    }

    cd::ui::draw_text(renderer, 342, 59, ru(session) ? "ИСТОЧНИКИ" : "SOURCES", kDim);
    for (int index = 0; index < 4; ++index) {
        const int y = 82 + index * 59;
        const auto& slot = session.slots[static_cast<std::size_t>(index)];
        const bool active = state.slot == index;
        if (active) outline(renderer, {334, y - 8, 158, 49}, kInk);
        const std::string title = std::to_string(index + 1) + "  " +
            std::string{engine_name(slot.engine, ru(session))};
        cd::ui::draw_text(renderer, 342, y, title, active ? kInk : kDim);
        const float meter = std::clamp(telemetry.slot_rms[static_cast<std::size_t>(index)] * 4.2F, 0.0F, 1.0F);
        bar(renderer, 342, y + 16, 142, 8, meter, react(kFxColors[static_cast<std::size_t>(index)], meter));
        cd::ui::draw_text(renderer, 342, y + 29,
            slot.enabled ? (ru(session) ? "ЗВУЧИТ" : "RUNNING") : "MUTE", slot.enabled ? kDim : kFxColors[0]);
    }
    const float chaos = std::clamp(telemetry.chaos_activity, 0.0F, 1.0F);
    cd::ui::draw_text(renderer, 342, 323, ru(session) ? "АКТИВН. ХАОСА" : "CHAOS ACTIVITY", kDim);
    bar(renderer, 342, 338, 142, 7, chaos, react(kFxColors[0], chaos));
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
        cd::ui::draw_text(renderer, x + 7, 69,
            engine_name(session.slots[static_cast<std::size_t>(index)].engine, ru(session)), kDim);
        scope(renderer, x + 8, 84, panel_width - 16, 103,
            telemetry.slot_scope[static_cast<std::size_t>(index)],
            telemetry.slot_rms[static_cast<std::size_t>(index)],
            telemetry.slot_peak[static_cast<std::size_t>(index)]);
        const auto& slot = session.slots[static_cast<std::size_t>(index)];
        const auto label = slot_name(parameter(state), slot, session.locale);
        cd::ui::draw_text(renderer, x + 7, 197, label, kDim);
        cd::ui::draw_text(renderer, x + 7, 211, value_text(session, state, index), kInk);
        for (int character = 0; character < 4; ++character) {
            const int y = 238 + character * 25;
            const int slot_parameter = character + 2;
            const SDL_Color color = react(kFxColors[static_cast<std::size_t>(character)], activity);
            cd::ui::draw_text(renderer, x + 7, y, slot_name(slot_parameter, slot, session.locale), color);
            bar(renderer, x + 7, y + 11, panel_width - 14, 7, slot_value(slot, slot_parameter), color);
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
    const int active_effect = parameter(state);
    const int active_field = state.effect_field;
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
            telemetry.slot_scope[static_cast<std::size_t>(state.slot)],
            telemetry.slot_rms[static_cast<std::size_t>(state.slot)],
            telemetry.slot_peak[static_cast<std::size_t>(state.slot)]);
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
        " · FX " + std::to_string(active_effect + 1) + " · " +
        std::string{effect_field(active_field, ru(session))};
    cd::ui::draw_text(renderer, 12, 357, hint, kDim);
}

void draw_master(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state,
    float cpu_load) {
    fill(renderer, {10, 48, 492, 306}, kPanel);
    cd::ui::draw_text(renderer, 24, 58, ru(session) ? "ИТОГОВЫЙ СИГНАЛ" : "MASTER SIGNAL", kDim);
    cd::ui::draw_text(renderer, 316, 58, ru(session) ? "RMS БЕЖ. · ПИК КРАСН." : "RMS CREAM · PEAK RED", kDim);
    scope(renderer, 24, 74, 464, 72, telemetry.master_scope, telemetry.master_rms, telemetry.master_peak);
    const std::array<std::string, 3> labels{
        ru(session) ? "ОБЩАЯ ГРОМКОСТЬ" : "MASTER LEVEL",
        ru(session) ? "ТЕМП ПУЛЬСА / СОБЫТИЙ" : "PULSE / EVENT TEMPO",
        ru(session) ? "ФЕЙД ВЫХОДА" : "OUTPUT FADE",
    };
    const std::array<float, 3> values{
        session.master_level, (session.tempo_bpm - 10.0F) / 290.0F, session.performance.fade};
    for (int index = 0; index < 3; ++index) {
        const int y = 164 + index * 51;
        const bool selected = parameter(state) == index;
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
        cd::ui::draw_text(renderer, 372, 318,
            state.fade_target > session.performance.fade
                ? (ru(session) ? "ФЕЙД ↑" : "FADE IN ↑")
                : (ru(session) ? "ФЕЙД ↓" : "FADE OUT ↓"),
            {91, 218, 179, 255});
    }
    char cpu[24]{};
    std::snprintf(cpu, sizeof(cpu), "DSP %.1f%%", static_cast<double>(cpu_load * 100.0F));
    cd::ui::draw_text(renderer, 24, 318, cpu, cpu_load < 0.75F ? kDim : kFxColors[0]);
    for (int slot = 0; slot < 4; ++slot) {
        const float value = telemetry.slot_rms[static_cast<std::size_t>(slot)] * 4.0F;
        bar(renderer, 24 + slot * 116, 336, 104, 8, value, react(kFxColors[static_cast<std::size_t>(slot)], value));
    }
}

void draw_setup(SDL_Renderer* renderer, const cd::Session& session, const UiState& state, float cpu_load) {
    fill(renderer, {10, 48, 492, 306}, kPanel);
    const std::array<std::string, 3> labels{
        std::string{cd::tr(session.locale, cd::TextId::language)},
        ru(session) ? "СКОРОСТЬ ФЕЙДА ВВЕРХ" : "FADE-IN TIME",
        ru(session) ? "СКОРОСТЬ ФЕЙДА ВНИЗ" : "FADE-OUT TIME",
    };
    for (int index = 0; index < 3; ++index) {
        const int y = 72 + index * 70;
        const bool selected = parameter(state) == index;
        if (selected) fill(renderer, {22, y - 10, 466, 55}, {73, 46, 104, 255});
        cd::ui::draw_text(renderer, 32, y, labels[static_cast<std::size_t>(index)], selected ? kInk : kDim);
        std::string shown_value;
        if (index == 0) {
            shown_value = session.locale == cd::Locale::ru ? "РУССКИЙ" : "ENGLISH";
        } else {
            char seconds_text[16]{};
            std::snprintf(seconds_text, sizeof(seconds_text), "%.2f s", static_cast<double>(
                index == 1 ? session.fade_in_seconds : session.fade_out_seconds));
            shown_value = seconds_text;
        }
        cd::ui::draw_text(renderer, 338, y, shown_value, selected ? kInk : kDim);
        if (index > 0) {
            const float seconds = index == 1 ? session.fade_in_seconds : session.fade_out_seconds;
            bar(renderer, 32, y + 20, 436, 10, (seconds - 0.25F) / 29.75F,
                selected ? kInk : kPurple);
        }
    }
    char cpu[48]{};
    std::snprintf(cpu, sizeof(cpu), "%s: %.1f%%", ru(session) ? "ЗАГРУЗКА DSP" : "DSP LOAD",
        static_cast<double>(cpu_load * 100.0F));
    cd::ui::draw_text(renderer, 32, 292, cpu, cpu_load < 0.75F ? kDim : kFxColors[0]);
    cd::ui::draw_text(renderer, 32, 316,
        ru(session) ? "A/D: ИЗМЕНИТЬ · НАСТРОЙКИ СОХРАНЯЮТСЯ АВТОМАТИЧЕСКИ"
                    : "A/D: CHANGE · SETTINGS ARE SAVED AUTOMATICALLY",
        kDim);
}

void draw(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state,
    Uint32 now,
    float cpu_load) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 8, 7, 12, 255);
    SDL_RenderClear(renderer);
    draw_header(renderer, session, state);
    if (state.page == Page::perform) {
        draw_scene(renderer, session, telemetry, state);
    } else if (state.page == Page::slot) {
        draw_tracks(renderer, session, telemetry, state);
    } else if (state.page == Page::effects) {
        draw_effects(renderer, session, telemetry, state);
    } else if (state.page == Page::master) {
        draw_master(renderer, session, telemetry, state, cpu_load);
    } else {
        draw_setup(renderer, session, state, cpu_load);
    }
    std::string help;
    if (state.page == Page::effects) {
        help = ru(session)
            ? "↑/↓ FX  ←/→ ПОЛЕ  S СЛОТ  A/D ЗНАЧ.  E ТИП"
            : "↑/↓ FX  ←/→ FIELD  S SLOT  A/D VALUE  E TYPE";
    } else if (state.page == Page::master || state.page == Page::setup) {
        help = ru(session)
            ? "↑/↓ ПАРАМ.  A/D ЗНАЧ.  TAB ЭКРАН  F ФЕЙД  K KILL"
            : "↑/↓ PARAM  A/D VALUE  TAB PAGE  F FADE  K KILL";
    } else {
        help = ru(session)
            ? "←/→ СЛОТ  ↑/↓ ПАРАМ.  A/D ЗНАЧ.  F ФЕЙД  K KILL"
            : "←/→ SLOT  ↑/↓ PARAM  A/D VALUE  F FADE  K KILL";
    }
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
    const float duration = state.fade_target > session.performance.fade
        ? session.fade_in_seconds : session.fade_out_seconds;
    const float amount = std::clamp(seconds, 0.0F, 0.1F) / std::max(0.25F, duration);
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
        .effects[static_cast<std::size_t>(parameter(state))].kind;
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
    if (window != nullptr && renderer == nullptr) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (window == nullptr || renderer == nullptr) {
        std::fprintf(stderr, "SDL video: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(renderer, kWidth, kHeight);

    cd::Session session = cd::make_default_session();
    std::filesystem::path autosave_path;
    if (char* preference_path = SDL_GetPrefPath("myldy20", "cursed-drone")) {
        autosave_path = std::filesystem::path{preference_path} / "autosave.cdrone";
        SDL_free(preference_path);
        if (std::filesystem::exists(autosave_path)) {
            std::string load_error;
            if (!cd::load_session(autosave_path, session, load_error)) {
                std::fprintf(stderr, "autosave load: %s\n", load_error.c_str());
            }
        }
    }
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
    audio.sample_rate = static_cast<float>(obtained.freq);
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
    bool save_pending = false;
    Uint32 changed_at = 0;
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
                case SDLK_LEFT:
                    if (state.page == Page::effects) state.effect_field = (state.effect_field + 2) % 3;
                    else state.slot = (state.slot + 3) % 4;
                    changed = true;
                    break;
                case SDLK_RIGHT:
                    if (state.page == Page::effects) state.effect_field = (state.effect_field + 1) % 3;
                    else state.slot = (state.slot + 1) % 4;
                    changed = true;
                    break;
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
                    state.page = static_cast<Page>((page_index(state.page) + 1) % 5);
                    changed = true;
                    break;
                case SDLK_1: state.page = Page::perform; changed = true; break;
                case SDLK_2: state.page = Page::slot; changed = true; break;
                case SDLK_3: state.page = Page::effects; changed = true; break;
                case SDLK_4: state.page = Page::master; changed = true; break;
                case SDLK_5: state.page = Page::setup; changed = true; break;
                case SDLK_s:
                    if (state.page == Page::effects) {
                        state.slot = (state.slot + 1) % 4;
                        changed = true;
                    }
                    break;
                case SDLK_e: cycle_effect(session, state); changed = true; break;
                case SDLK_f: toggle_fade(session, state); changed = true; break;
                case SDLK_k:
                    audio.graph.panic();
                    state.kill_flash_until = now + 700U;
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
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                    if (state.page == Page::effects) state.effect_field = (state.effect_field + 2) % 3;
                    else state.slot = (state.slot + 3) % 4;
                    changed = true;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                    if (state.page == Page::effects) state.effect_field = (state.effect_field + 1) % 3;
                    else state.slot = (state.slot + 1) % 4;
                    changed = true;
                    break;
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
                    state.page = static_cast<Page>((page_index(state.page) + 1) % 5);
                    changed = true;
                    break;
                case SDL_CONTROLLER_BUTTON_Y:
                    if (state.page == Page::effects) {
                        state.slot = (state.slot + 1) % 4;
                        changed = true;
                    }
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
            save_pending = true;
            changed_at = now;
        }
        if (save_pending && !autosave_path.empty() && now - changed_at >= 750U) {
            std::string save_error;
            if (cd::save_session(session, autosave_path, save_error)) {
                save_pending = false;
            } else {
                std::fprintf(stderr, "autosave: %s\n", save_error.c_str());
                changed_at = now;
            }
        }
        draw(renderer, session, audio.graph.telemetry(), state, now,
            audio.cpu_load.load(std::memory_order_relaxed));
        SDL_Delay(8);
    }

    if (save_pending && !autosave_path.empty()) {
        std::string save_error;
        if (!cd::save_session(session, autosave_path, save_error)) {
            std::fprintf(stderr, "autosave: %s\n", save_error.c_str());
        }
    }
    SDL_CloseAudioDevice(device);
    if (controller != nullptr) SDL_GameControllerClose(controller);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

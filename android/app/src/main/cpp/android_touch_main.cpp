// SPDX-License-Identifier: GPL-3.0-or-later
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "bitmap_text.hpp"
#include "cursed_drone/audio.hpp"
#include "cursed_drone/catalog.hpp"
#include "cursed_drone/scala.hpp"
#include "cursed_drone/session.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#ifndef CURSED_DRONE_VERSION
#define CURSED_DRONE_VERSION "dev"
#endif

namespace cd = cursed_drone;

namespace {

constexpr SDL_Color kBackground{8, 7, 12, 255};
constexpr SDL_Color kPanel{24, 20, 32, 255};
constexpr SDL_Color kPanelRaised{35, 29, 46, 255};
constexpr SDL_Color kPanelActive{67, 43, 91, 255};
constexpr SDL_Color kInk{238, 226, 197, 255};
constexpr SDL_Color kDim{151, 143, 158, 255};
constexpr SDL_Color kBorder{84, 72, 96, 255};
constexpr SDL_Color kPurple{117, 67, 171, 255};
constexpr SDL_Color kGreen{91, 218, 179, 255};
constexpr SDL_Color kRed{225, 77, 96, 255};
constexpr std::array<SDL_Color, 4> kActorColors{{
    {216, 88, 88, 255}, {224, 154, 63, 255}, {80, 169, 154, 255}, {91, 122, 187, 255},
}};

enum class Page { place, actor, fx, master, memory };
enum class ActorSection { sound, events, modulation };
enum class PickerKind {
    none, scene, engine, effect, plaits_model, output, scale, mod_wave, mod_destination
};
enum class Action {
    none, page, fade, actor_select, actor_toggle, actor_section,
    scene_picker, engine_picker, actor_trigger, actor_fx_select, actor_fx_picker,
    master_fx_select, master_fx_picker, euclidean_toggle,
    mod_select, mod_toggle, mod_source_cycle,
    memory_select, memory_load, memory_save, landscape_reset, locale_toggle,
    picker_item, picker_previous, picker_next, picker_close, slider
};
enum class SliderKind {
    none,
    place_texture, place_pulse, place_chaos, place_space, place_events,
    actor_frequency, actor_timbre, actor_color, actor_motion, actor_texture,
    actor_level, actor_pan, actor_event_density, tuning_root, euclidean_steps, euclidean_pulses,
    euclidean_rotation, euclidean_probability, mod_rate, mod_depth,
    mod_offset, mod_cross, actor_fx_amount, actor_fx_tone, actor_fx_feedback,
    master_level, tempo, master_fx_amount, master_fx_tone, master_fx_feedback,
    fade_in, fade_out
};

struct AudioBridge {
    cd::AudioGraph graph{};
    std::atomic<float> cpu_load{0.0F};
    float sample_rate{48'000.0F};
};

struct HitTarget {
    SDL_Rect rect{};
    Action action{Action::none};
    int a{0};
    int b{0};
    SliderKind slider{SliderKind::none};
};

struct UiState {
    Page page{Page::place};
    ActorSection actor_section{ActorSection::sound};
    int actor{0};
    int actor_fx{0};
    int master_fx{0};
    int modulator{0};
    int memory_slot{0};
    int pending_trigger{-1};
    std::array<bool, cd::kMemorySlots> memory_present{};
    PickerKind picker{PickerKind::none};
    int picker_page{0};
    bool picker_master{false};
    int picker_effect{0};
    bool fade_active{false};
    float fade_target{1.0F};
    std::string toast{};
    Uint32 toast_until{0};
    std::vector<HitTarget> hits{};
    bool finger_down{false};
    SDL_FingerID finger_id{0};
    HitTarget pressed{};
    bool slider_active{false};
};

std::filesystem::path g_data_root{};
std::filesystem::path g_autosave_path{};
std::array<std::filesystem::path, cd::kMemorySlots> g_memory_paths{};
std::vector<cd::ParsedScale> g_scales{};

bool ru(const cd::Session& session) noexcept { return session.locale == cd::Locale::ru; }

void fill(SDL_Renderer* renderer, SDL_Rect rect, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

void outline(SDL_Renderer* renderer, SDL_Rect rect, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(renderer, &rect);
}

bool contains(const SDL_Rect& rect, int x, int y) noexcept {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
}

int font_scale_for(int height) noexcept { return std::clamp(height / 360, 1, 4); }

std::string clipped(std::string text, int max_width, int scale) {
    if (cd::ui::text_width(text, scale) <= max_width) return text;
    while (text.size() > 3U && cd::ui::text_width(text + "...", scale) > max_width) {
        text.pop_back();
    }
    return text + "...";
}

void text(SDL_Renderer* renderer, int x, int y, std::string_view value,
    SDL_Color color, int scale) {
    cd::ui::draw_text(renderer, x, y, value, color, scale);
}

void centered_text(SDL_Renderer* renderer, const SDL_Rect& rect,
    std::string value, SDL_Color color, int scale) {
    value = clipped(std::move(value), rect.w - 16, scale);
    const int width = cd::ui::text_width(value, scale);
    const int height = 8 * scale;
    text(renderer, rect.x + (rect.w - width) / 2,
        rect.y + (rect.h - height) / 2, value, color, scale);
}

void add_hit(UiState& state, SDL_Rect rect, Action action, int a = 0, int b = 0,
    SliderKind slider = SliderKind::none) {
    state.hits.push_back({rect, action, a, b, slider});
}

void button(SDL_Renderer* renderer, UiState& state, SDL_Rect rect,
    std::string label, bool active, Action action, int a, int b, int scale,
    SDL_Color accent = kPurple) {
    fill(renderer, rect, active ? kPanelActive : kPanelRaised);
    outline(renderer, rect, active ? accent : kBorder);
    centered_text(renderer, rect, std::move(label), active ? kInk : kDim, scale);
    add_hit(state, rect, action, a, b);
}

void meter(SDL_Renderer* renderer, SDL_Rect rect, float value, SDL_Color color) {
    value = std::clamp(value, 0.0F, 1.0F);
    fill(renderer, rect, {18, 15, 24, 255});
    fill(renderer, {rect.x, rect.y, static_cast<int>(std::lround(rect.w * value)), rect.h}, color);
}

void slider(SDL_Renderer* renderer, UiState& state, SDL_Rect rect,
    std::string label, std::string value, float normalized, SliderKind kind,
    int a, int b, int scale, SDL_Color color = kPurple) {
    fill(renderer, rect, kPanelRaised);
    outline(renderer, rect, kBorder);
    const int label_y = rect.y + std::max(8, rect.h / 7);
    text(renderer, rect.x + 14, label_y, clipped(label, rect.w / 2, scale), kDim, scale);
    const int value_width = cd::ui::text_width(value, scale);
    text(renderer, rect.x + rect.w - 14 - value_width, label_y, value, kInk, scale);
    const int bar_h = std::max(10, rect.h / 7);
    SDL_Rect bar_rect{rect.x + 14, rect.y + rect.h - bar_h - 12, rect.w - 28, bar_h};
    meter(renderer, bar_rect, normalized, color);
    const int knob_x = bar_rect.x + static_cast<int>(std::lround(
        static_cast<float>(bar_rect.w) * std::clamp(normalized, 0.0F, 1.0F)));
    fill(renderer, {knob_x - 3, bar_rect.y - 5, 7, bar_rect.h + 10}, kInk);
    add_hit(state, rect, Action::slider, a, b, kind);
}

std::string percent(float value) {
    char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "%d%%",
        static_cast<int>(std::lround(value * 100.0F)));
    return buffer;
}

std::string signed_percent(float value) {
    char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "%+.0f%%", static_cast<double>(value * 100.0F));
    return buffer;
}

std::string decimal(float value, const char* suffix) {
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%.2f %s", static_cast<double>(value), suffix);
    return buffer;
}

std::string scene_name(cd::SceneKind scene, bool russian) {
    switch (scene) {
    case cd::SceneKind::derelict: return russian ? "ЗАБРОШЕННОЕ" : "DERELICT";
    case cd::SceneKind::factory: return russian ? "ЦЕХ" : "FACTORY";
    case cd::SceneKind::wasteland: return russian ? "ПУСТОШЬ" : "WASTELAND";
    case cd::SceneKind::wet_cave: return russian ? "МОКРАЯ ПЕЩЕРА" : "WET CAVE";
    case cd::SceneKind::metro: return russian ? "ВАГОН МЕТРО" : "METRO CAR";
    case cd::SceneKind::nursery: return russian ? "СЛОМАННАЯ ДЕТСКАЯ" : "BROKEN NURSERY";
    case cd::SceneKind::bunker: return russian ? "БУНКЕР" : "BUNKER";
    case cd::SceneKind::power_grid: return russian ? "ПОДСТАНЦИЯ" : "POWER GRID";
    case cd::SceneKind::deep_water: return russian ? "ГЛУБИНА" : "DEEP WATER";
    case cd::SceneKind::ash_field: return russian ? "ПЕПЕЛ" : "ASH FIELD";
    }
    return {};
}

std::string engine_name(cd::EngineKind value, bool russian) {
    switch (value) {
    case cd::EngineKind::diagnostic: return russian ? "КЛАССИЧЕСКИЙ" : "LEGACY";
    case cd::EngineKind::macro: return russian ? "ТОН" : "TONE";
    case cd::EngineKind::body: return russian ? "РЕЗОНАТОР" : "RESONATOR";
    case cd::EngineKind::grain: return russian ? "ЗЕРНО" : "GRAINLET";
    case cd::EngineKind::particle: return russian ? "ЧАСТИЦЫ" : "PARTICLES";
    case cd::EngineKind::derelict_bed: return russian ? "ФОН" : "ROOM BED";
    case cd::EngineKind::footsteps: return russian ? "ШАГИ" : "FOOTSTEPS";
    case cd::EngineKind::door: return russian ? "ДВЕРЬ" : "DOOR";
    case cd::EngineKind::pipe: return russian ? "ТРУБА" : "PIPE";
    case cd::EngineKind::motor: return russian ? "МОТОР" : "MOTOR";
    case cd::EngineKind::machinery: return russian ? "СТАНОК" : "MACHINE";
    case cd::EngineKind::crowd: return russian ? "ТОЛПА" : "CROWD";
    case cd::EngineKind::metal: return russian ? "МЕТАЛЛ" : "METAL";
    case cd::EngineKind::wind: return russian ? "ВЕТЕР" : "WIND";
    case cd::EngineKind::birds: return russian ? "ПТИЦЫ" : "BIRDS";
    case cd::EngineKind::insects: return russian ? "НАСЕКОМЫЕ" : "INSECTS";
    case cd::EngineKind::signal: return russian ? "СИГНАЛ" : "SIGNAL";
    case cd::EngineKind::cave_air: return russian ? "ВОЗДУХ" : "CAVE AIR";
    case cd::EngineKind::water_drip: return russian ? "КАПЛИ" : "DRIPS";
    case cd::EngineKind::water_flow: return russian ? "ПОТОК" : "FLOW";
    case cd::EngineKind::stone: return russian ? "КАМЕНЬ" : "STONE";
    case cd::EngineKind::metro_traction: return russian ? "ТЯГА" : "TRACTION";
    case cd::EngineKind::rail_joint: return russian ? "СТЫКИ" : "RAIL JOINTS";
    case cd::EngineKind::brake: return russian ? "ТОРМОЗ" : "BRAKE";
    case cd::EngineKind::carriage: return russian ? "КУЗОВ" : "CARRIAGE";
    case cd::EngineKind::music_box: return russian ? "ШКАТУЛКА" : "MUSIC BOX";
    case cd::EngineKind::toy_voice: return russian ? "ИГРУШЕЧНЫЙ ГОЛОС" : "TOY VOICE";
    case cd::EngineKind::toy_gears: return russian ? "ШЕСТЕРНИ" : "TOY GEARS";
    case cd::EngineKind::lullaby: return russian ? "КОЛЫБЕЛЬНАЯ" : "LULLABY";
    case cd::EngineKind::sub_drone: return russian ? "САБ-ДРОН" : "SUB DRONE";
    case cd::EngineKind::tape_drone: return russian ? "ЛЕНТОЧНЫЙ ДРОН" : "TAPE DRONE";
    case cd::EngineKind::bowed_metal: return russian ? "СМЫЧКОВЫЙ МЕТАЛЛ" : "BOWED METAL";
    case cd::EngineKind::earth_rumble: return russian ? "ГУЛ ЗЕМЛИ" : "EARTH RUMBLE";
    case cd::EngineKind::plaits: return "PLAITS";
    }
    return {};
}

std::string effect_name(cd::EffectKind value, bool russian) {
    switch (value) {
    case cd::EffectKind::bypass: return russian ? "ПУСТО" : "EMPTY";
    case cd::EffectKind::drive: return russian ? "ДРАЙВ" : "DRIVE";
    case cd::EffectKind::lowpass: return russian ? "ФНЧ" : "LOWPASS";
    case cd::EffectKind::highpass: return russian ? "ФВЧ" : "HIGHPASS";
    case cd::EffectKind::tremolo: return russian ? "ТРЕМОЛО" : "TREMOLO";
    case cd::EffectKind::delay: return russian ? "ДЕЛЕЙ" : "DELAY";
    case cd::EffectKind::crusher: return russian ? "КРАШЕР" : "CRUSHER";
    case cd::EffectKind::wavefolder: return russian ? "ФОЛДЕР" : "FOLDER";
    case cd::EffectKind::ringmod: return russian ? "КОЛЬЦО" : "RING MOD";
    case cd::EffectKind::comb: return russian ? "ГРЕБЕНКА" : "COMB";
    case cd::EffectKind::chorus: return russian ? "ХОРУС" : "CHORUS";
    case cd::EffectKind::flanger: return russian ? "ФЛЭНЖЕР" : "FLANGER";
    case cd::EffectKind::phaser: return russian ? "ФЕЙЗЕР" : "PHASER";
    case cd::EffectKind::diffuser: return russian ? "ДИФФУЗОР" : "DIFFUSER";
    case cd::EffectKind::ahdr: return "AHDR";
    case cd::EffectKind::tape_void: return russian ? "ЛЕНТОПУСТОТА" : "TAPE VOID";
    case cd::EffectKind::black_hole: return russian ? "ЧЕРНАЯ ДЫРА" : "BLACK HOLE";
    case cd::EffectKind::ritual_gate: return russian ? "РИТУАЛЬНЫЙ ГЕЙТ" : "RITUAL GATE";
    case cd::EffectKind::rust_cloud: return russian ? "ОБЛАКО РЖАВЧИНЫ" : "RUST CLOUD";
    case cd::EffectKind::deep_sea: return russian ? "ГЛУБИНА" : "DEEP SEA";
    case cd::EffectKind::granular_reverse: return russian ? "ОБРАТНЫЕ ЗЕРНА" : "REVERSE GRAINS";
    }
    return {};
}

std::string plaits_model_name(cd::PlaitsModel value) {
    constexpr std::array<std::string_view, 16> names{{
        "FILTER TONE", "PHASE TONE", "WAVE TERRAIN", "STRING MACHINE",
        "CHIP TONE", "ANALOG", "WAVESHAPER", "FM", "GRAIN", "ADDITIVE",
        "WAVETABLE", "CHORD", "SWARM", "NOISE", "STRING", "MODAL BODY",
    }};
    return std::string{names[static_cast<std::size_t>(value)]};
}

std::string output_name(cd::PlaitsOutputMode value) {
    constexpr std::array<std::string_view, 4> names{{"MAIN", "AUX", "MIX", "STEREO"}};
    return std::string{names[static_cast<std::size_t>(value)]};
}

std::string wave_name(cd::ModWave value, bool russian) {
    constexpr std::array<std::string_view, 4> en{{"SINE", "TRIANGLE", "S&H", "RANDOM WALK"}};
    constexpr std::array<std::string_view, 4> rr{{"СИНУС", "ТРЕУГОЛЬНИК", "СЛУЧ. ШАГ", "БЛУЖДАНИЕ"}};
    return std::string{(russian ? rr : en)[static_cast<std::size_t>(value)]};
}

std::string destination_name(cd::ModDestination value, bool russian) {
    constexpr std::array<std::string_view, 11> en{{
        "PITCH", "TIMBRE", "BODY", "MOTION", "TEXTURE", "LEVEL", "PAN",
        "FX1", "FX2", "FX3", "FX4"}};
    constexpr std::array<std::string_view, 11> rr{{
        "ВЫСОТА", "ТЕМБР", "ТЕЛО", "ДВИЖЕНИЕ", "ТЕКСТУРА", "УРОВЕНЬ", "ПАНОРАМА",
        "FX1", "FX2", "FX3", "FX4"}};
    return std::string{(russian ? rr : en)[static_cast<std::size_t>(value)]};
}

std::string page_name(Page page, bool russian) {
    switch (page) {
    case Page::place: return russian ? "МЕСТО" : "PLACE";
    case Page::actor: return russian ? "АКТЕР" : "ACTOR";
    case Page::fx: return "FX";
    case Page::master: return russian ? "МАСТЕР" : "MASTER";
    case Page::memory: return russian ? "ПАМЯТЬ" : "MEMORY";
    }
    return {};
}

std::string macro_name(int index, bool russian) {
    constexpr std::array<std::string_view, 5> en{{
        "MATERIAL", "ACTIVITY", "TENSION", "DISTANCE", "EVOLUTION"}};
    constexpr std::array<std::string_view, 5> rr{{
        "МАТЕРИАЛ", "АКТИВНОСТЬ", "НАПРЯЖЕНИЕ", "ДИСТАНЦИЯ", "РАЗВИТИЕ"}};
    return std::string{(russian ? rr : en)[static_cast<std::size_t>(index)]};
}

int effect_parameter_count(cd::EffectKind kind) noexcept {
    switch (kind) {
    case cd::EffectKind::bypass: return 0;
    case cd::EffectKind::drive: return 1;
    case cd::EffectKind::lowpass:
    case cd::EffectKind::highpass:
    case cd::EffectKind::tremolo:
    case cd::EffectKind::crusher:
    case cd::EffectKind::wavefolder:
    case cd::EffectKind::ringmod: return 2;
    default: return 3;
    }
}

void audio_callback(void* userdata, Uint8* bytes, int byte_count) {
    auto& bridge = *static_cast<AudioBridge*>(userdata);
    const Uint64 started = SDL_GetPerformanceCounter();
    auto* frames = reinterpret_cast<cd::StereoFrame*>(bytes);
    const auto frame_count = static_cast<std::size_t>(byte_count) / sizeof(cd::StereoFrame);
    bridge.graph.process({frames, frame_count});
    const double elapsed = static_cast<double>(SDL_GetPerformanceCounter() - started) /
        static_cast<double>(SDL_GetPerformanceFrequency());
    const double available = static_cast<double>(frame_count) / bridge.sample_rate;
    const float load = static_cast<float>(std::clamp(elapsed / available, 0.0, 4.0));
    const float previous = bridge.cpu_load.load(std::memory_order_relaxed);
    bridge.cpu_load.store(previous + (load - previous) * 0.12F, std::memory_order_relaxed);
}

int parse_argument(int argc, char** argv, const char* prefix, int fallback) {
    const std::string prefix_string{prefix};
    for (int i = 1; i < argc; ++i) {
        const std::string argument{argv[i] == nullptr ? "" : argv[i]};
        if (argument.rfind(prefix_string, 0) != 0) continue;
        try {
            const int value = std::stoi(argument.substr(prefix_string.size()));
            return value > 0 ? value : fallback;
        } catch (...) {
            return fallback;
        }
    }
    return fallback;
}

int next_power_of_two(int value) noexcept {
    int result = 1;
    while (result < value && result < 32768) result <<= 1;
    return result;
}

bool copy_asset(const char* source_name, const std::filesystem::path& destination) {
    SDL_RWops* source = SDL_RWFromFile(source_name, "rb");
    if (source == nullptr) return false;
    const Sint64 count = SDL_RWsize(source);
    if (count <= 0) { SDL_RWclose(source); return false; }
    std::vector<unsigned char> bytes(static_cast<std::size_t>(count));
    const std::size_t read = SDL_RWread(source, bytes.data(), 1, bytes.size());
    SDL_RWclose(source);
    if (read != bytes.size()) return false;
    std::error_code error;
    std::filesystem::create_directories(destination.parent_path(), error);
    std::ofstream output(destination, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    return output.good();
}

void prepare_storage() {
    char* path = SDL_GetPrefPath("myldy20", "cursed-drone");
    if (path == nullptr) return;
    g_data_root = std::filesystem::path{path};
    SDL_free(path);
    std::error_code error;
    std::filesystem::create_directories(g_data_root / "scales", error);
    copy_asset("branding/cursed-drone-splash.bmp",
        g_data_root / "branding" / "cursed-drone-splash.bmp");
    copy_asset("scales/19-edo.scl", g_data_root / "scales" / "19-edo.scl");
    copy_asset("scales/just-minor.scl", g_data_root / "scales" / "just-minor.scl");
    g_autosave_path = g_data_root / "autosave.cdrone";
    for (std::size_t i = 0; i < cd::kMemorySlots; ++i) {
        g_memory_paths[i] = g_data_root / ("memory-" + std::to_string(i + 1U) + ".cdrone");
    }
    g_scales = cd::load_scala_scales({g_data_root / "scales"});
    g_scales.insert(g_scales.begin(), cd::equal_temperament_scale());
}

void show_splash(SDL_Renderer* renderer, int width, int height) {
    SDL_Surface* surface = SDL_LoadBMP("branding/cursed-drone-splash.bmp");
    SDL_Texture* texture = surface != nullptr ? SDL_CreateTextureFromSurface(renderer, surface) : nullptr;
    if (surface != nullptr) SDL_FreeSurface(surface);
    fill(renderer, {0, 0, width, height}, kBackground);
    if (texture != nullptr) {
        SDL_SetTextureColorMod(texture, kInk.r, kInk.g, kInk.b);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_DestroyTexture(texture);
    } else {
        const int scale = font_scale_for(height) + 1;
        centered_text(renderer, {0, 0, width, height}, "CURSED DRONE", kInk, scale);
    }
    SDL_RenderPresent(renderer);
    SDL_Delay(900);
}

void set_toast(UiState& state, std::string message) {
    state.toast = std::move(message);
    state.toast_until = SDL_GetTicks() + 1800U;
}

float normalized_frequency(float hz) noexcept {
    return std::clamp(std::log(std::max(hz, 20.0F) / 20.0F) / std::log(44.0F), 0.0F, 1.0F);
}

float frequency_from_normalized(float normalized) noexcept {
    return 20.0F * std::pow(44.0F, std::clamp(normalized, 0.0F, 1.0F));
}

float normalized_rate(float rate) noexcept {
    return std::clamp(std::log(std::max(rate, 0.01F) / 0.01F) / std::log(2000.0F), 0.0F, 1.0F);
}

float rate_from_normalized(float normalized) noexcept {
    return 0.01F * std::pow(2000.0F, std::clamp(normalized, 0.0F, 1.0F));
}

void set_slider_value(cd::Session& session, const UiState& state,
    const HitTarget& hit, float normalized) {
    normalized = std::clamp(normalized, 0.0F, 1.0F);
    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    auto& mod = slot.modulators[static_cast<std::size_t>(state.modulator)];
    auto& actor_fx = slot.effects[static_cast<std::size_t>(state.actor_fx)];
    auto& master_fx = session.master_effects[static_cast<std::size_t>(state.master_fx)];
    switch (hit.slider) {
    case SliderKind::place_texture: session.performance.texture = normalized; break;
    case SliderKind::place_pulse: session.performance.pulse = normalized; break;
    case SliderKind::place_chaos: session.performance.chaos = normalized; break;
    case SliderKind::place_space: session.performance.space = normalized; break;
    case SliderKind::place_events: session.performance.events = normalized; break;
    case SliderKind::actor_frequency: slot.frequency_hz = frequency_from_normalized(normalized); break;
    case SliderKind::actor_timbre: slot.timbre = normalized; break;
    case SliderKind::actor_color: slot.color = normalized; break;
    case SliderKind::actor_motion: slot.motion = normalized; break;
    case SliderKind::actor_texture: slot.texture = normalized; break;
    case SliderKind::actor_level: slot.level = normalized; break;
    case SliderKind::actor_pan: slot.pan = normalized * 2.0F - 1.0F; break;
    case SliderKind::actor_event_density: slot.event_density = normalized; break;
    case SliderKind::tuning_root: slot.tuning.root_midi = static_cast<int>(std::lround(normalized * 127.0F)); break;
    case SliderKind::euclidean_steps:
        slot.euclidean.steps = 1 + static_cast<int>(std::lround(normalized * 31.0F));
        slot.euclidean.pulses = std::min(slot.euclidean.pulses, slot.euclidean.steps);
        slot.euclidean.rotation = std::min(slot.euclidean.rotation, slot.euclidean.steps - 1);
        break;
    case SliderKind::euclidean_pulses:
        slot.euclidean.pulses = static_cast<int>(std::lround(normalized * slot.euclidean.steps)); break;
    case SliderKind::euclidean_rotation:
        slot.euclidean.rotation = static_cast<int>(std::lround(normalized * std::max(1, slot.euclidean.steps - 1))); break;
    case SliderKind::euclidean_probability: slot.euclidean.probability = normalized; break;
    case SliderKind::mod_rate: mod.rate_hz = rate_from_normalized(normalized); break;
    case SliderKind::mod_depth: mod.depth = normalized; break;
    case SliderKind::mod_offset: mod.offset = normalized * 2.0F - 1.0F; break;
    case SliderKind::mod_cross: mod.rate_mod_amount = normalized * 2.0F - 1.0F; break;
    case SliderKind::actor_fx_amount: actor_fx.amount = normalized; break;
    case SliderKind::actor_fx_tone: actor_fx.tone = normalized; break;
    case SliderKind::actor_fx_feedback: actor_fx.feedback = normalized; break;
    case SliderKind::master_level: session.master_level = normalized; break;
    case SliderKind::tempo: session.tempo_bpm = 10.0F + normalized * 290.0F; break;
    case SliderKind::master_fx_amount: master_fx.amount = normalized; break;
    case SliderKind::master_fx_tone: master_fx.tone = normalized; break;
    case SliderKind::master_fx_feedback: master_fx.feedback = normalized; break;
    case SliderKind::fade_in: session.fade_in_seconds = 0.2F + normalized * 19.8F; break;
    case SliderKind::fade_out: session.fade_out_seconds = 0.2F + normalized * 19.8F; break;
    case SliderKind::none: break;
    }
    session.scene_modified = true;
}

int picker_count(PickerKind picker) {
    switch (picker) {
    case PickerKind::scene: return static_cast<int>(cd::catalog::scenes.size());
    case PickerKind::engine: return static_cast<int>(cd::catalog::engines.size());
    case PickerKind::effect: return static_cast<int>(cd::catalog::effects.size());
    case PickerKind::plaits_model: return static_cast<int>(cd::catalog::plaits_models.size());
    case PickerKind::output: return static_cast<int>(cd::catalog::plaits_outputs.size());
    case PickerKind::scale: return static_cast<int>(g_scales.size());
    case PickerKind::mod_wave: return static_cast<int>(cd::catalog::mod_waves.size());
    case PickerKind::mod_destination: return static_cast<int>(cd::catalog::mod_destinations.size());
    case PickerKind::none: return 0;
    }
    return 0;
}

std::string picker_label(PickerKind picker, int index, const cd::Session& session) {
    switch (picker) {
    case PickerKind::scene: return scene_name(cd::catalog::scenes[static_cast<std::size_t>(index)], ru(session));
    case PickerKind::engine: return engine_name(cd::catalog::engines[static_cast<std::size_t>(index)], ru(session));
    case PickerKind::effect: return effect_name(cd::catalog::effects[static_cast<std::size_t>(index)], ru(session));
    case PickerKind::plaits_model: return plaits_model_name(cd::catalog::plaits_models[static_cast<std::size_t>(index)]);
    case PickerKind::output: return output_name(cd::catalog::plaits_outputs[static_cast<std::size_t>(index)]);
    case PickerKind::scale: return g_scales[static_cast<std::size_t>(index)].name;
    case PickerKind::mod_wave: return wave_name(cd::catalog::mod_waves[static_cast<std::size_t>(index)], ru(session));
    case PickerKind::mod_destination: return destination_name(cd::catalog::mod_destinations[static_cast<std::size_t>(index)], ru(session));
    case PickerKind::none: return {};
    }
    return {};
}

int current_picker_index(const UiState& state, const cd::Session& session) {
    const auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    const auto& mod = slot.modulators[static_cast<std::size_t>(state.modulator)];
    switch (state.picker) {
    case PickerKind::scene:
        for (int i = 0; i < static_cast<int>(cd::catalog::scenes.size()); ++i) if (cd::catalog::scenes[static_cast<std::size_t>(i)] == session.scene) return i;
        break;
    case PickerKind::engine:
        for (int i = 0; i < static_cast<int>(cd::catalog::engines.size()); ++i) if (cd::catalog::engines[static_cast<std::size_t>(i)] == slot.engine) return i;
        break;
    case PickerKind::effect: {
        const auto kind = state.picker_master
            ? session.master_effects[static_cast<std::size_t>(state.picker_effect)].kind
            : slot.effects[static_cast<std::size_t>(state.picker_effect)].kind;
        for (int i = 0; i < static_cast<int>(cd::catalog::effects.size()); ++i) if (cd::catalog::effects[static_cast<std::size_t>(i)] == kind) return i;
        break;
    }
    case PickerKind::plaits_model: return static_cast<int>(slot.plaits_model);
    case PickerKind::output: return static_cast<int>(slot.plaits_output);
    case PickerKind::scale:
        for (int i = 0; i < static_cast<int>(g_scales.size()); ++i) {
            if (g_scales[static_cast<std::size_t>(i)].name == std::string{slot.tuning.name.data()}) return i;
        }
        break;
    case PickerKind::mod_wave: return static_cast<int>(mod.wave);
    case PickerKind::mod_destination: return static_cast<int>(mod.destination);
    case PickerKind::none: break;
    }
    return 0;
}

void apply_picker_item(cd::Session& session, UiState& state, int index) {
    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    auto& mod = slot.modulators[static_cast<std::size_t>(state.modulator)];
    switch (state.picker) {
    case PickerKind::scene: cd::apply_scene_recipe(session, cd::catalog::scenes[static_cast<std::size_t>(index)]); break;
    case PickerKind::engine: slot.engine = cd::catalog::engines[static_cast<std::size_t>(index)]; session.scene_modified = true; break;
    case PickerKind::effect:
        if (state.picker_master) session.master_effects[static_cast<std::size_t>(state.picker_effect)].kind = cd::catalog::effects[static_cast<std::size_t>(index)];
        else slot.effects[static_cast<std::size_t>(state.picker_effect)].kind = cd::catalog::effects[static_cast<std::size_t>(index)];
        session.scene_modified = true;
        break;
    case PickerKind::plaits_model: slot.plaits_model = cd::catalog::plaits_models[static_cast<std::size_t>(index)]; break;
    case PickerKind::output: slot.plaits_output = cd::catalog::plaits_outputs[static_cast<std::size_t>(index)]; break;
    case PickerKind::scale: cd::apply_scale(slot.tuning, g_scales[static_cast<std::size_t>(index)]); break;
    case PickerKind::mod_wave: mod.wave = cd::catalog::mod_waves[static_cast<std::size_t>(index)]; break;
    case PickerKind::mod_destination: mod.destination = cd::catalog::mod_destinations[static_cast<std::size_t>(index)]; break;
    case PickerKind::none: break;
    }
    state.picker = PickerKind::none;
    state.picker_page = 0;
}

void refresh_memory_cache(UiState& state) {
    for (std::size_t slot = 0; slot < cd::kMemorySlots; ++slot) {
        std::error_code error;
        state.memory_present[slot] =
            std::filesystem::is_regular_file(g_memory_paths[slot], error);
    }
}

bool execute_action(cd::Session& session, UiState& state, const HitTarget& hit) {
    switch (hit.action) {
    case Action::page: state.page = static_cast<Page>(hit.a); return false;
    case Action::fade:
        state.fade_active = true;
        state.fade_target = session.performance.fade > 0.02F ? 0.0F : 1.0F;
        return false;
    case Action::actor_select:
        state.actor = hit.a;
        if (hit.b != 0) state.page = Page::actor;
        return false;
    case Action::actor_toggle:
        session.slots[static_cast<std::size_t>(hit.a)].enabled =
            !session.slots[static_cast<std::size_t>(hit.a)].enabled;
        session.scene_modified = true;
        return true;
    case Action::actor_section: state.actor_section = static_cast<ActorSection>(hit.a); return false;
    case Action::scene_picker: state.picker = PickerKind::scene; state.picker_page = 0; return false;
    case Action::engine_picker: state.picker = PickerKind::engine; state.picker_page = 0; return false;
    case Action::actor_trigger: state.pending_trigger = hit.a; return false;
    case Action::actor_fx_select: state.actor_fx = hit.a; return false;
    case Action::actor_fx_picker:
        state.picker = PickerKind::effect; state.picker_master = false;
        state.picker_effect = state.actor_fx; state.picker_page = 0; return false;
    case Action::master_fx_select: state.master_fx = hit.a; return false;
    case Action::master_fx_picker:
        state.picker = PickerKind::effect; state.picker_master = true;
        state.picker_effect = state.master_fx; state.picker_page = 0; return false;
    case Action::euclidean_toggle: {
        auto& enabled = session.slots[static_cast<std::size_t>(state.actor)].euclidean.enabled;
        enabled = !enabled; return true;
    }
    case Action::mod_select: state.modulator = hit.a; return false;
    case Action::mod_toggle: {
        auto& enabled = session.slots[static_cast<std::size_t>(state.actor)]
            .modulators[static_cast<std::size_t>(state.modulator)].enabled;
        enabled = !enabled; return true;
    }
    case Action::mod_source_cycle: {
        auto& source = session.slots[static_cast<std::size_t>(state.actor)]
            .modulators[static_cast<std::size_t>(state.modulator)].rate_mod_source;
        source = source >= 3 ? -1 : source + 1; return true;
    }
    case Action::memory_select: state.memory_slot = hit.a; return false;
    case Action::memory_load: {
        if (!state.memory_present[static_cast<std::size_t>(state.memory_slot)]) {
            set_toast(state, ru(session) ? "СЛОТ ПУСТ" : "SLOT IS EMPTY"); return false;
        }
        cd::Session loaded{};
        std::string error;
        if (cd::load_session(g_memory_paths[static_cast<std::size_t>(state.memory_slot)], loaded, error)) {
            session = loaded;
            set_toast(state, ru(session) ? "СЛОТ ЗАГРУЖЕН" : "SLOT LOADED");
            return true;
        }
        set_toast(state, ru(session) ? "ОШИБКА ЗАГРУЗКИ" : "LOAD FAILED");
        return false;
    }
    case Action::memory_save: {
        std::string error;
        if (cd::save_session(session, g_memory_paths[static_cast<std::size_t>(state.memory_slot)], error)) {
            state.memory_present[static_cast<std::size_t>(state.memory_slot)] = true;
            set_toast(state, ru(session) ? "СЛОТ СОХРАНЕН" : "SLOT SAVED");
        } else set_toast(state, ru(session) ? "ОШИБКА СОХРАНЕНИЯ" : "SAVE FAILED");
        return false;
    }
    case Action::landscape_reset:
        cd::apply_scene_recipe(session, session.scene);
        set_toast(state, ru(session) ? "МЕСТО ВОССТАНОВЛЕНО" : "PLACE RESTORED");
        return true;
    case Action::locale_toggle:
        session.locale = session.locale == cd::Locale::ru ? cd::Locale::en : cd::Locale::ru;
        return true;
    case Action::picker_item: apply_picker_item(session, state, hit.a); return true;
    case Action::picker_previous: state.picker_page = std::max(0, state.picker_page - 1); return false;
    case Action::picker_next: state.picker_page += 1; return false;
    case Action::picker_close: state.picker = PickerKind::none; state.picker_page = 0; return false;
    case Action::slider:
    case Action::none: return false;
    }
    return false;
}

void draw_header(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, int width, int header_h, int scale) {
    fill(renderer, {0, 0, width, header_h}, {14, 12, 20, 255});
    constexpr std::array<Page, 5> pages{{Page::place, Page::actor, Page::fx, Page::master, Page::memory}};
    const int fade_w = std::max(150, width / 7);
    const int tabs_w = width - fade_w;
    const int tab_w = tabs_w / 5;
    for (int i = 0; i < 5; ++i) {
        SDL_Rect rect{i * tab_w, 0, i == 4 ? tabs_w - i * tab_w : tab_w, header_h};
        const bool active = state.page == pages[static_cast<std::size_t>(i)];
        fill(renderer, rect, active ? kPanelActive : kPanel);
        if (active) fill(renderer, {rect.x, rect.y + rect.h - 5, rect.w, 5}, kPurple);
        centered_text(renderer, rect, page_name(pages[static_cast<std::size_t>(i)], ru(session)),
            active ? kInk : kDim, scale);
        add_hit(state, rect, Action::page, static_cast<int>(pages[static_cast<std::size_t>(i)]));
    }
    SDL_Rect fade_rect{tabs_w, 0, fade_w, header_h};
    fill(renderer, fade_rect, state.fade_active ? kPanelActive : kPanelRaised);
    outline(renderer, fade_rect, kGreen);
    const std::string fade_label = std::string{"FADE "} + percent(session.performance.fade) +
        "   V" CURSED_DRONE_VERSION;
    centered_text(renderer, fade_rect, fade_label, kGreen, scale);
    add_hit(state, fade_rect, Action::fade);
    const int meter_h = std::max(6, header_h / 12);
    meter(renderer, {fade_rect.x + 12, fade_rect.y + fade_rect.h - meter_h - 8,
        fade_rect.w - 24, meter_h}, telemetry.master_rms * 4.0F, kGreen);
}

void draw_actor_strip(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, SDL_Rect area, int scale, bool open_actor) {
    const int gap = std::max(8, area.w / 100);
    const int card_w = (area.w - gap * 3) / 4;
    for (int actor = 0; actor < 4; ++actor) {
        SDL_Rect card{area.x + actor * (card_w + gap), area.y, card_w, area.h};
        const bool selected = actor == state.actor;
        fill(renderer, card, selected ? kPanelActive : kPanelRaised);
        outline(renderer, card, selected ? kActorColors[static_cast<std::size_t>(actor)] : kBorder);
        SDL_Rect title_rect{card.x + 10, card.y + 8, card.w - 20, std::max(26, card.h / 4)};
        centered_text(renderer, title_rect,
            std::to_string(actor + 1) + "  " + engine_name(session.slots[static_cast<std::size_t>(actor)].engine, ru(session)),
            selected ? kInk : kDim, scale);
        meter(renderer, {card.x + 12, card.y + card.h / 2 - 4, card.w - 24, 8},
            telemetry.slot_rms[static_cast<std::size_t>(actor)] * 4.2F,
            kActorColors[static_cast<std::size_t>(actor)]);
        const int mute_h = std::max(34, card.h / 3);
        SDL_Rect mute_rect{card.x + 8, card.y + card.h - mute_h - 8, card.w - 16, mute_h};
        const bool enabled = session.slots[static_cast<std::size_t>(actor)].enabled;
        fill(renderer, mute_rect, enabled ? kPanel : kRed);
        outline(renderer, mute_rect, enabled ? kBorder : kRed);
        centered_text(renderer, mute_rect,
            enabled ? (ru(session) ? "ЗАГЛУШИТЬ" : "MUTE") : (ru(session) ? "ВКЛЮЧИТЬ" : "UNMUTE"),
            enabled ? kDim : kInk, scale);
        add_hit(state, mute_rect, Action::actor_toggle, actor);
        SDL_Rect card_hit{card.x, card.y, card.w, std::max(1, card.h - mute_h - 12)};
        add_hit(state, card_hit, Action::actor_select, actor, open_actor ? 1 : 0);
    }
}

void draw_place(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, SDL_Rect area, int scale) {
    const int pad = std::max(12, area.h / 45);
    SDL_Rect scene_rect{area.x + pad, area.y + pad, area.w - 2 * pad, std::max(64, area.h / 10)};
    fill(renderer, scene_rect, kPanelActive);
    outline(renderer, scene_rect, kPurple);
    centered_text(renderer, scene_rect,
        std::string{ru(session) ? "МЕСТО: " : "PLACE: "} + scene_name(session.scene, ru(session)) + "  ▼",
        kInk, scale + (scale < 3 ? 1 : 0));
    add_hit(state, scene_rect, Action::scene_picker);

    const int body_y = scene_rect.y + scene_rect.h + pad;
    const int body_h = area.y + area.h - body_y - pad;
    const int macro_w = static_cast<int>(std::lround(area.w * 0.58F));
    SDL_Rect macro_area{area.x + pad, body_y, macro_w - 2 * pad, body_h};
    SDL_Rect actors_area{area.x + macro_w, body_y, area.w - macro_w - pad, body_h};
    const int row_gap = std::max(8, pad / 2);
    const int row_h = (macro_area.h - row_gap * 4) / 5;
    const std::array<float, 5> values{{
        session.performance.texture, session.performance.pulse, session.performance.chaos,
        session.performance.space, session.performance.events}};
    const std::array<SliderKind, 5> kinds{{
        SliderKind::place_texture, SliderKind::place_pulse, SliderKind::place_chaos,
        SliderKind::place_space, SliderKind::place_events}};
    for (int i = 0; i < 5; ++i) {
        slider(renderer, state,
            {macro_area.x, macro_area.y + i * (row_h + row_gap), macro_area.w, row_h},
            macro_name(i, ru(session)), percent(values[static_cast<std::size_t>(i)]),
            values[static_cast<std::size_t>(i)], kinds[static_cast<std::size_t>(i)], 0, 0,
            scale, kActorColors[static_cast<std::size_t>(i % 4)]);
    }
    const int actor_gap = std::max(10, pad / 2);
    const int actor_w = (actors_area.w - actor_gap) / 2;
    const int actor_h = (actors_area.h - actor_gap) / 2;
    for (int actor = 0; actor < 4; ++actor) {
        const int col = actor % 2;
        const int row = actor / 2;
        SDL_Rect card{actors_area.x + col * (actor_w + actor_gap),
            actors_area.y + row * (actor_h + actor_gap), actor_w, actor_h};
        const auto& slot = session.slots[static_cast<std::size_t>(actor)];
        fill(renderer, card, actor == state.actor ? kPanelActive : kPanelRaised);
        outline(renderer, card, kActorColors[static_cast<std::size_t>(actor)]);
        SDL_Rect top{card.x + 8, card.y + 6, card.w - 16, std::max(28, card.h / 4)};
        centered_text(renderer, top,
            std::to_string(actor + 1) + "  " + engine_name(slot.engine, ru(session)), kInk, scale);
        meter(renderer, {card.x + 12, card.y + card.h / 2 - 5, card.w - 24, 10},
            telemetry.slot_rms[static_cast<std::size_t>(actor)] * 4.2F,
            kActorColors[static_cast<std::size_t>(actor)]);
        const int mute_h = std::max(36, card.h / 4);
        SDL_Rect mute{card.x + 8, card.y + card.h - mute_h - 8, card.w - 16, mute_h};
        fill(renderer, mute, slot.enabled ? kPanel : kRed);
        outline(renderer, mute, slot.enabled ? kBorder : kRed);
        centered_text(renderer, mute,
            slot.enabled ? (ru(session) ? "ЗАГЛУШИТЬ" : "MUTE") : (ru(session) ? "ВКЛЮЧИТЬ" : "UNMUTE"),
            slot.enabled ? kDim : kInk, scale);
        add_hit(state, mute, Action::actor_toggle, actor);
        add_hit(state, {card.x, card.y, card.w, card.h - mute_h - 12}, Action::actor_select, actor, 1);
    }
}

void draw_actor_sound(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    SDL_Rect area, int scale) {
    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    const int gap = std::max(10, area.h / 50);
    const int col_w = (area.w - gap) / 2;
    const int row_h = (area.h - gap * 3) / 4;
    SDL_Rect engine_rect{area.x, area.y, col_w, row_h};
    fill(renderer, engine_rect, kPanelActive);
    outline(renderer, engine_rect, kActorColors[static_cast<std::size_t>(state.actor)]);
    centered_text(renderer, engine_rect,
        std::string{ru(session) ? "ДВИЖОК: " : "ENGINE: "} + engine_name(slot.engine, ru(session)) + "  ▼",
        kInk, scale);
    add_hit(state, engine_rect, Action::engine_picker);
    slider(renderer, state, {area.x + col_w + gap, area.y, col_w, row_h},
        ru(session) ? "ВЫСОТА" : "PITCH", decimal(slot.frequency_hz, "HZ"),
        normalized_frequency(slot.frequency_hz), SliderKind::actor_frequency, 0, 0, scale,
        kActorColors[static_cast<std::size_t>(state.actor)]);
    const std::array<std::string, 6> labels{{
        ru(session) ? "ХАРАКТЕР" : "CHARACTER", ru(session) ? "ТЕЛО" : "BODY",
        ru(session) ? "ДВИЖЕНИЕ" : "MOTION", ru(session) ? "ТЕКСТУРА" : "TEXTURE",
        ru(session) ? "УРОВЕНЬ" : "LEVEL", ru(session) ? "ПАНОРАМА" : "PAN"}};
    const std::array<float, 6> values{{slot.timbre, slot.color, slot.motion, slot.texture, slot.level, (slot.pan + 1.0F) * 0.5F}};
    const std::array<SliderKind, 6> kinds{{
        SliderKind::actor_timbre, SliderKind::actor_color, SliderKind::actor_motion,
        SliderKind::actor_texture, SliderKind::actor_level, SliderKind::actor_pan}};
    for (int i = 0; i < 6; ++i) {
        const int row = 1 + i / 2;
        const int col = i % 2;
        slider(renderer, state,
            {area.x + col * (col_w + gap), area.y + row * (row_h + gap), col_w, row_h},
            labels[static_cast<std::size_t>(i)],
            i == 5 ? signed_percent(slot.pan) : percent(values[static_cast<std::size_t>(i)]),
            values[static_cast<std::size_t>(i)], kinds[static_cast<std::size_t>(i)], 0, 0,
            scale, kActorColors[static_cast<std::size_t>(state.actor)]);
    }
}

void draw_actor_events(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    SDL_Rect area, int scale) {
    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    const int gap = std::max(8, area.h / 60);
    const int col_w = (area.w - gap) / 2;
    const int row_h = (area.h - gap * 4) / 5;
    const SDL_Color accent = kActorColors[static_cast<std::size_t>(state.actor)];

    const bool can_trigger = cd::supports_manual_trigger(slot.engine) && slot.enabled;
    std::string trigger_label;
    if (!slot.enabled) trigger_label = ru(session) ? "СНАЧАЛА ВКЛЮЧИТЕ" : "UNMUTE FIRST";
    else if (can_trigger) trigger_label = ru(session) ? "ЗАПУСТИТЬ СОБЫТИЕ" : "TRIGGER NOW";
    else trigger_label = ru(session) ? "НЕПРЕРЫВНЫЙ ДВИЖОК" : "CONTINUOUS ENGINE";
    button(renderer, state, {area.x, area.y, col_w, row_h}, trigger_label, false,
        can_trigger ? Action::actor_trigger : Action::none, state.actor, 0, scale, accent);

    SDL_Rect event_rate_rect{area.x + col_w + gap, area.y, col_w, row_h};
    if (cd::supports_event_rate(slot.engine)) {
        const float density = cd::effective_event_density(slot.event_density, session.performance.events);
        const float rate = cd::event_rate_hz(session.tempo_bpm, density, slot.motion);
        const float maximum = cd::event_max_wait_seconds(session.tempo_bpm, density, slot.motion);
        char value[64]{};
        if (rate < 0.25F) {
            std::snprintf(value, sizeof(value), "%d%%   %.1f/M   FMAX %.0fS",
                static_cast<int>(std::lround(slot.event_density * 100.0F)),
                static_cast<double>(rate * 60.0F), static_cast<double>(maximum));
        } else {
            std::snprintf(value, sizeof(value), "%d%%   %.2f/S   FMAX %.1fS",
                static_cast<int>(std::lround(slot.event_density * 100.0F)),
                static_cast<double>(rate), static_cast<double>(maximum));
        }
        slider(renderer, state, event_rate_rect,
            ru(session) ? "ЧАСТОТА СОБЫТИЙ" : "EVENT RATE", value,
            slot.event_density, SliderKind::actor_event_density, 0, 0, scale, accent);
    } else {
        fill(renderer, event_rate_rect, kPanelRaised); outline(renderer, event_rate_rect, kBorder);
        centered_text(renderer, event_rate_rect,
            slot.engine == cd::EngineKind::plaits
                ? (ru(session) ? "РИТМ: EUCLIDEAN" : "RHYTHM: EUCLIDEAN")
                : (ru(session) ? "ЧАСТОТА: Н/Д" : "EVENT RATE: N/A"),
            kDim, scale);
    }

    const int row1 = area.y + row_h + gap;
    button(renderer, state, {area.x, row1, col_w, row_h},
        std::string{ru(session) ? "EUCLIDEAN: " : "EUCLIDEAN: "} +
            (slot.euclidean.enabled ? (ru(session) ? "ВКЛ" : "ON") : (ru(session) ? "ВЫКЛ" : "OFF")),
        slot.euclidean.enabled, Action::euclidean_toggle, 0, 0, scale, accent);
    SDL_Rect scale_rect{area.x + col_w + gap, row1, col_w, row_h};
    fill(renderer, scale_rect, kPanelRaised); outline(renderer, scale_rect, kBorder);
    centered_text(renderer, scale_rect,
        std::string{ru(session) ? "СТРОЙ: " : "TUNING: "} + std::string{slot.tuning.name.data()} + "  ▼",
        kInk, scale);
    add_hit(state, scale_rect, Action::actor_section, 99, 0);

    const int row2 = row1 + row_h + gap;
    slider(renderer, state, {area.x, row2, col_w, row_h},
        ru(session) ? "ОСНОВА" : "ROOT", std::to_string(slot.tuning.root_midi),
        static_cast<float>(slot.tuning.root_midi) / 127.0F, SliderKind::tuning_root, 0, 0,
        scale, accent);
    slider(renderer, state, {area.x + col_w + gap, row2, col_w, row_h},
        ru(session) ? "ШАГИ" : "STEPS", std::to_string(slot.euclidean.steps),
        static_cast<float>(slot.euclidean.steps - 1) / 31.0F, SliderKind::euclidean_steps, 0, 0,
        scale, accent);

    const int row3 = row2 + row_h + gap;
    slider(renderer, state, {area.x, row3, col_w, row_h},
        ru(session) ? "ИМПУЛЬСЫ" : "PULSES", std::to_string(slot.euclidean.pulses),
        static_cast<float>(slot.euclidean.pulses) / std::max(1, slot.euclidean.steps),
        SliderKind::euclidean_pulses, 0, 0, scale, accent);
    slider(renderer, state, {area.x + col_w + gap, row3, col_w, row_h},
        ru(session) ? "СДВИГ" : "ROTATE", std::to_string(slot.euclidean.rotation),
        static_cast<float>(slot.euclidean.rotation) / std::max(1, slot.euclidean.steps - 1),
        SliderKind::euclidean_rotation, 0, 0, scale, accent);

    const int row4 = row3 + row_h + gap;
    slider(renderer, state, {area.x, row4, col_w, row_h},
        ru(session) ? "ВЕРОЯТНОСТЬ" : "PROBABILITY", percent(slot.euclidean.probability),
        slot.euclidean.probability, SliderKind::euclidean_probability, 0, 0, scale, accent);
    SDL_Rect right_bottom{area.x + col_w + gap, row4, col_w, row_h};
    if (slot.engine == cd::EngineKind::plaits) {
        const int half_h = (right_bottom.h - gap) / 2;
        SDL_Rect model_rect{right_bottom.x, right_bottom.y, right_bottom.w, half_h};
        fill(renderer, model_rect, kPanelRaised); outline(renderer, model_rect, kBorder);
        centered_text(renderer, model_rect, "MODEL: " + plaits_model_name(slot.plaits_model) + "  ▼", kInk, scale);
        add_hit(state, model_rect, Action::actor_section, 97, 0);
        SDL_Rect output_rect{right_bottom.x, right_bottom.y + half_h + gap, right_bottom.w,
            right_bottom.h - half_h - gap};
        fill(renderer, output_rect, kPanelRaised); outline(renderer, output_rect, kBorder);
        centered_text(renderer, output_rect, "OUTPUT: " + output_name(slot.plaits_output) + "  ▼", kInk, scale);
        add_hit(state, output_rect, Action::actor_section, 98, 0);
    } else {
        fill(renderer, right_bottom, kPanelRaised); outline(renderer, right_bottom, kBorder);
        centered_text(renderer, right_bottom,
            ru(session) ? "ГЛОБАЛЬНЫЕ СОБЫТИЯ ВЛИЯЮТ НА ТЕМП" : "GLOBAL EVENTS ALSO SHAPE RATE",
            kDim, scale);
    }
}

void draw_actor_modulation(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    SDL_Rect area, int scale) {
    auto& slot = session.slots[static_cast<std::size_t>(state.actor)];
    auto& mod = slot.modulators[static_cast<std::size_t>(state.modulator)];
    const int gap = std::max(10, area.h / 50);
    const int tabs_h = std::max(58, area.h / 9);
    const int tab_w = (area.w - gap * 3) / 4;
    for (int i = 0; i < 4; ++i) {
        button(renderer, state, {area.x + i * (tab_w + gap), area.y, tab_w, tabs_h},
            "MOD " + std::to_string(i + 1), i == state.modulator, Action::mod_select, i, 0,
            scale, kActorColors[static_cast<std::size_t>(state.actor)]);
    }
    SDL_Rect body{area.x, area.y + tabs_h + gap, area.w, area.h - tabs_h - gap};
    const int col_w = (body.w - gap) / 2;
    const int row_h = (body.h - gap * 3) / 4;
    button(renderer, state, {body.x, body.y, col_w, row_h},
        std::string{ru(session) ? "МОДУЛЯТОР: " : "MODULATOR: "} + (mod.enabled ? "ON" : "OFF"),
        mod.enabled, Action::mod_toggle, 0, 0, scale,
        kActorColors[static_cast<std::size_t>(state.actor)]);
    SDL_Rect wave_rect{body.x + col_w + gap, body.y, col_w, row_h};
    fill(renderer, wave_rect, kPanelRaised); outline(renderer, wave_rect, kBorder);
    centered_text(renderer, wave_rect,
        std::string{ru(session) ? "ФОРМА: " : "SHAPE: "} + wave_name(mod.wave, ru(session)) + "  ▼", kInk, scale);
    add_hit(state, wave_rect, Action::actor_section, 95, 0);
    SDL_Rect destination_rect{body.x, body.y + row_h + gap, col_w, row_h};
    fill(renderer, destination_rect, kPanelRaised); outline(renderer, destination_rect, kBorder);
    centered_text(renderer, destination_rect,
        std::string{ru(session) ? "ЦЕЛЬ: " : "TARGET: "} + destination_name(mod.destination, ru(session)) + "  ▼", kInk, scale);
    add_hit(state, destination_rect, Action::actor_section, 96, 0);
    slider(renderer, state, {body.x + col_w + gap, body.y + row_h + gap, col_w, row_h},
        ru(session) ? "СКОРОСТЬ" : "RATE", decimal(mod.rate_hz, "HZ"),
        normalized_rate(mod.rate_hz), SliderKind::mod_rate, 0, 0, scale,
        kActorColors[static_cast<std::size_t>(state.actor)]);
    slider(renderer, state, {body.x, body.y + 2 * (row_h + gap), col_w, row_h},
        ru(session) ? "ГЛУБИНА" : "DEPTH", percent(mod.depth), mod.depth,
        SliderKind::mod_depth, 0, 0, scale, kActorColors[static_cast<std::size_t>(state.actor)]);
    slider(renderer, state, {body.x + col_w + gap, body.y + 2 * (row_h + gap), col_w, row_h},
        ru(session) ? "СМЕЩЕНИЕ" : "OFFSET", signed_percent(mod.offset), (mod.offset + 1.0F) * 0.5F,
        SliderKind::mod_offset, 0, 0, scale, kActorColors[static_cast<std::size_t>(state.actor)]);
    const std::string source = mod.rate_mod_source < 0 ? "NONE" : "MOD " + std::to_string(mod.rate_mod_source + 1);
    button(renderer, state, {body.x, body.y + 3 * (row_h + gap), col_w, row_h},
        std::string{ru(session) ? "ИСТОЧНИК СКОРОСТИ: " : "RATE SOURCE: "} + source,
        mod.rate_mod_source >= 0, Action::mod_source_cycle, 0, 0, scale,
        kActorColors[static_cast<std::size_t>(state.actor)]);
    slider(renderer, state, {body.x + col_w + gap, body.y + 3 * (row_h + gap), col_w, row_h},
        ru(session) ? "ГЛУБИНА СВЯЗИ" : "CROSS AMOUNT", signed_percent(mod.rate_mod_amount),
        (mod.rate_mod_amount + 1.0F) * 0.5F, SliderKind::mod_cross, 0, 0, scale,
        kActorColors[static_cast<std::size_t>(state.actor)]);
}

void draw_actor(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, SDL_Rect area, int scale) {
    const int pad = std::max(10, area.h / 50);
    const int actors_h = std::max(100, area.h / 5);
    draw_actor_strip(renderer, session, state, telemetry,
        {area.x + pad, area.y + pad, area.w - 2 * pad, actors_h}, scale, false);
    const int tabs_y = area.y + pad + actors_h + pad;
    const int tabs_h = std::max(54, area.h / 12);
    const int tab_w = (area.w - 2 * pad) / 3;
    constexpr std::array<ActorSection, 3> sections{{
        ActorSection::sound, ActorSection::events, ActorSection::modulation}};
    for (int i = 0; i < 3; ++i) {
        const auto section = sections[static_cast<std::size_t>(i)];
        std::string label;
        if (section == ActorSection::sound) label = ru(session) ? "ЗВУК" : "SOUND";
        else if (section == ActorSection::events) label = ru(session) ? "СОБЫТИЯ" : "EVENTS";
        else label = ru(session) ? "МОДУЛЯЦИЯ" : "MODULATION";
        button(renderer, state, {area.x + pad + i * tab_w, tabs_y,
            i == 2 ? area.x + area.w - pad - (area.x + pad + i * tab_w) : tab_w, tabs_h},
            label, state.actor_section == section, Action::actor_section,
            static_cast<int>(section), 0, scale, kActorColors[static_cast<std::size_t>(state.actor)]);
    }
    SDL_Rect editor{area.x + pad, tabs_y + tabs_h + pad,
        area.w - 2 * pad, area.y + area.h - (tabs_y + tabs_h + 2 * pad)};
    if (state.actor_section == ActorSection::sound) draw_actor_sound(renderer, session, state, editor, scale);
    else if (state.actor_section == ActorSection::events) draw_actor_events(renderer, session, state, editor, scale);
    else draw_actor_modulation(renderer, session, state, editor, scale);
}

void draw_fx_editor(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    SDL_Rect area, int scale, bool master) {
    auto& effect = master
        ? session.master_effects[static_cast<std::size_t>(state.master_fx)]
        : session.slots[static_cast<std::size_t>(state.actor)].effects[static_cast<std::size_t>(state.actor_fx)];
    const int gap = std::max(12, area.h / 45);
    const int type_h = std::max(80, area.h / 5);
    SDL_Rect type_rect{area.x, area.y, area.w, type_h};
    fill(renderer, type_rect, kPanelActive); outline(renderer, type_rect, kPurple);
    centered_text(renderer, type_rect,
        std::string{ru(session) ? "ТИП ЭФФЕКТА: " : "EFFECT TYPE: "} + effect_name(effect.kind, ru(session)) + "  ▼",
        kInk, scale + (scale < 3 ? 1 : 0));
    add_hit(state, type_rect, master ? Action::master_fx_picker : Action::actor_fx_picker);
    const int count = std::max(1, effect_parameter_count(effect.kind));
    const int sliders_y = type_rect.y + type_rect.h + gap;
    const int sliders_h = area.y + area.h - sliders_y;
    const int slider_gap = gap;
    const int slider_w = (area.w - slider_gap * (count - 1)) / count;
    const std::array<std::string, 3> labels{{
        ru(session) ? "ИНТЕНСИВНОСТЬ" : "AMOUNT",
        ru(session) ? "ХАРАКТЕР" : "TONE",
        ru(session) ? "ОБРАТНАЯ СВЯЗЬ" : "FEEDBACK"}};
    const std::array<float, 3> values{{effect.amount, effect.tone, effect.feedback}};
    const std::array<SliderKind, 3> actor_kinds{{SliderKind::actor_fx_amount, SliderKind::actor_fx_tone, SliderKind::actor_fx_feedback}};
    const std::array<SliderKind, 3> master_kinds{{SliderKind::master_fx_amount, SliderKind::master_fx_tone, SliderKind::master_fx_feedback}};
    for (int i = 0; i < count; ++i) {
        slider(renderer, state,
            {area.x + i * (slider_w + slider_gap), sliders_y, slider_w, sliders_h},
            labels[static_cast<std::size_t>(i)], percent(values[static_cast<std::size_t>(i)]),
            values[static_cast<std::size_t>(i)],
            master ? master_kinds[static_cast<std::size_t>(i)] : actor_kinds[static_cast<std::size_t>(i)],
            0, 0, scale, kActorColors[static_cast<std::size_t>(i)]);
    }
    if (effect.kind == cd::EffectKind::bypass) {
        centered_text(renderer, {area.x, sliders_y, area.w, sliders_h},
            ru(session) ? "ЭФФЕКТ ОТКЛЮЧЕН" : "EFFECT BYPASSED", kDim, scale + 1);
    }
}

void draw_fx(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, SDL_Rect area, int scale) {
    const int pad = std::max(10, area.h / 50);
    const int actors_h = std::max(92, area.h / 7);
    draw_actor_strip(renderer, session, state, telemetry,
        {area.x + pad, area.y + pad, area.w - 2 * pad, actors_h}, scale, false);
    const int fx_y = area.y + 2 * pad + actors_h;
    const int fx_h = std::max(70, area.h / 9);
    const int gap = pad;
    const int fx_w = (area.w - 2 * pad - gap * 3) / 4;
    for (int i = 0; i < 4; ++i) {
        const auto& effect = session.slots[static_cast<std::size_t>(state.actor)].effects[static_cast<std::size_t>(i)];
        button(renderer, state, {area.x + pad + i * (fx_w + gap), fx_y, fx_w, fx_h},
            "FX " + std::to_string(i + 1) + "  " + effect_name(effect.kind, ru(session)),
            i == state.actor_fx, Action::actor_fx_select, i, 0, scale,
            kActorColors[static_cast<std::size_t>(i)]);
    }
    draw_fx_editor(renderer, session, state,
        {area.x + pad, fx_y + fx_h + pad, area.w - 2 * pad,
            area.y + area.h - (fx_y + fx_h + 2 * pad)}, scale, false);
}

void draw_master(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    SDL_Rect area, int scale) {
    const int pad = std::max(10, area.h / 50);
    const int gap = pad;
    const int top_h = std::max(90, area.h / 6);
    const int half_w = (area.w - 3 * pad) / 2;
    slider(renderer, state, {area.x + pad, area.y + pad, half_w, top_h},
        ru(session) ? "ОБЩИЙ УРОВЕНЬ" : "MASTER LEVEL", percent(session.master_level),
        session.master_level, SliderKind::master_level, 0, 0, scale, kGreen);
    slider(renderer, state, {area.x + 2 * pad + half_w, area.y + pad, half_w, top_h},
        ru(session) ? "ТЕМП" : "TEMPO", std::to_string(static_cast<int>(std::lround(session.tempo_bpm))) + " BPM",
        (session.tempo_bpm - 10.0F) / 290.0F, SliderKind::tempo, 0, 0, scale, kGreen);
    const int fx_y = area.y + 2 * pad + top_h;
    const int fx_h = std::max(70, area.h / 9);
    const int fx_w = (area.w - 2 * pad - gap * 3) / 4;
    for (int i = 0; i < 4; ++i) {
        const auto& effect = session.master_effects[static_cast<std::size_t>(i)];
        button(renderer, state, {area.x + pad + i * (fx_w + gap), fx_y, fx_w, fx_h},
            "FX " + std::to_string(i + 1) + "  " + effect_name(effect.kind, ru(session)),
            i == state.master_fx, Action::master_fx_select, i, 0, scale,
            kActorColors[static_cast<std::size_t>(i)]);
    }
    draw_fx_editor(renderer, session, state,
        {area.x + pad, fx_y + fx_h + pad, area.w - 2 * pad,
            area.y + area.h - (fx_y + fx_h + 2 * pad)}, scale, true);
}

void draw_memory(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    SDL_Rect area, int scale) {
    const int pad = std::max(10, area.h / 50);
    const int gap = pad;
    const int slots_h = static_cast<int>(std::lround(area.h * 0.48F));
    const int slot_w = (area.w - 2 * pad - 3 * gap) / 4;
    const int slot_h = (slots_h - gap) / 2;
    for (int i = 0; i < 8; ++i) {
        const int col = i % 4;
        const int row = i / 4;
        SDL_Rect rect{area.x + pad + col * (slot_w + gap), area.y + pad + row * (slot_h + gap), slot_w, slot_h};
        button(renderer, state, rect,
            std::string{ru(session) ? "СЛОТ " : "SLOT "} + std::to_string(i + 1) +
                (state.memory_present[static_cast<std::size_t>(i)]
                    ? (ru(session) ? "  СОХРАНЕН" : "  SAVED")
                    : (ru(session) ? "  ПУСТ" : "  EMPTY")),
            i == state.memory_slot, Action::memory_select, i, 0, scale,
            state.memory_present[static_cast<std::size_t>(i)] ? kGreen : kPurple);
    }
    const int action_y = area.y + pad + slots_h + gap;
    const int action_h = std::max(64, area.h / 10);
    const int action_w = (area.w - 2 * pad - 2 * gap) / 3;
    button(renderer, state, {area.x + pad, action_y, action_w, action_h},
        ru(session) ? "ЗАГРУЗИТЬ" : "LOAD", false, Action::memory_load, 0, 0, scale, kGreen);
    button(renderer, state, {area.x + pad + action_w + gap, action_y, action_w, action_h},
        ru(session) ? "СОХРАНИТЬ" : "SAVE", false, Action::memory_save, 0, 0, scale, kGreen);
    button(renderer, state, {area.x + pad + 2 * (action_w + gap), action_y, action_w, action_h},
        ru(session) ? "СБРОСИТЬ МЕСТО" : "RESET PLACE", false, Action::landscape_reset, 0, 0, scale, kRed);
    const int settings_y = action_y + action_h + gap;
    const int settings_h = area.y + area.h - settings_y - pad;
    const int setting_w = (area.w - 2 * pad - 2 * gap) / 3;
    button(renderer, state, {area.x + pad, settings_y, setting_w, settings_h},
        std::string{ru(session) ? "ЯЗЫК: РУССКИЙ" : "LANGUAGE: ENGLISH"}, true,
        Action::locale_toggle, 0, 0, scale, kPurple);
    slider(renderer, state, {area.x + pad + setting_w + gap, settings_y, setting_w, settings_h},
        ru(session) ? "ФЕЙД ВХОДА" : "FADE IN", decimal(session.fade_in_seconds, "S"),
        (session.fade_in_seconds - 0.2F) / 19.8F, SliderKind::fade_in, 0, 0, scale, kGreen);
    slider(renderer, state, {area.x + pad + 2 * (setting_w + gap), settings_y, setting_w, settings_h},
        ru(session) ? "ФЕЙД ВЫХОДА" : "FADE OUT", decimal(session.fade_out_seconds, "S"),
        (session.fade_out_seconds - 0.2F) / 19.8F, SliderKind::fade_out, 0, 0, scale, kGreen);
}

void draw_picker(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    int width, int height, int scale) {
    fill(renderer, {0, 0, width, height}, {8, 7, 12, 248});
    const int pad = std::max(16, height / 45);
    const int title_h = std::max(70, height / 10);
    SDL_Rect title{pad, pad, width - 2 * pad, title_h};
    fill(renderer, title, kPanelActive); outline(renderer, title, kPurple);
    std::string title_text;
    switch (state.picker) {
    case PickerKind::scene: title_text = ru(session) ? "ВЫБЕРИТЕ МЕСТО" : "CHOOSE PLACE"; break;
    case PickerKind::engine: title_text = ru(session) ? "ВЫБЕРИТЕ ДВИЖОК" : "CHOOSE ENGINE"; break;
    case PickerKind::effect: title_text = ru(session) ? "ВЫБЕРИТЕ ЭФФЕКТ" : "CHOOSE EFFECT"; break;
    case PickerKind::plaits_model: title_text = "CHOOSE PLAITS MODEL"; break;
    case PickerKind::output: title_text = "CHOOSE OUTPUT"; break;
    case PickerKind::scale: title_text = ru(session) ? "ВЫБЕРИТЕ СТРОЙ" : "CHOOSE TUNING"; break;
    case PickerKind::mod_wave: title_text = ru(session) ? "ВЫБЕРИТЕ ФОРМУ" : "CHOOSE SHAPE"; break;
    case PickerKind::mod_destination: title_text = ru(session) ? "ВЫБЕРИТЕ ЦЕЛЬ" : "CHOOSE TARGET"; break;
    case PickerKind::none: return;
    }
    centered_text(renderer, title, title_text, kInk, scale + (scale < 3 ? 1 : 0));
    SDL_Rect close_rect{title.x + title.w - title_h, title.y, title_h, title_h};
    fill(renderer, close_rect, kRed); centered_text(renderer, close_rect, "X", kInk, scale + 1);
    add_hit(state, close_rect, Action::picker_close);

    constexpr int columns = 3;
    constexpr int rows = 4;
    constexpr int page_size = columns * rows;
    const int count = picker_count(state.picker);
    const int max_page = std::max(0, (count - 1) / page_size);
    state.picker_page = std::clamp(state.picker_page, 0, max_page);
    const int grid_y = title.y + title.h + pad;
    const int footer_h = std::max(62, height / 11);
    const int grid_h = height - grid_y - footer_h - 2 * pad;
    const int gap = pad;
    const int item_w = (width - 2 * pad - gap * (columns - 1)) / columns;
    const int item_h = (grid_h - gap * (rows - 1)) / rows;
    const int selected = current_picker_index(state, session);
    const int first = state.picker_page * page_size;
    for (int local = 0; local < page_size; ++local) {
        const int index = first + local;
        if (index >= count) break;
        const int col = local % columns;
        const int row = local / columns;
        SDL_Rect rect{pad + col * (item_w + gap), grid_y + row * (item_h + gap), item_w, item_h};
        button(renderer, state, rect, picker_label(state.picker, index, session),
            index == selected, Action::picker_item, index, 0, scale,
            index == selected ? kGreen : kPurple);
    }
    const int footer_y = height - footer_h - pad;
    const int nav_w = (width - 3 * pad) / 2;
    button(renderer, state, {pad, footer_y, nav_w, footer_h},
        ru(session) ? "◀ ПРЕДЫДУЩИЕ" : "◀ PREVIOUS", state.picker_page > 0,
        Action::picker_previous, 0, 0, scale, kPurple);
    button(renderer, state, {2 * pad + nav_w, footer_y, nav_w, footer_h},
        ru(session) ? "СЛЕДУЮЩИЕ ▶" : "NEXT ▶", state.picker_page < max_page,
        Action::picker_next, 0, 0, scale, kPurple);
}

void draw(SDL_Renderer* renderer, cd::Session& session, UiState& state,
    const cd::AudioTelemetry& telemetry, int width, int height) {
    state.hits.clear();
    fill(renderer, {0, 0, width, height}, kBackground);
    const int scale = font_scale_for(height);
    const int header_h = std::max(72, height / 10);
    draw_header(renderer, session, state, telemetry, width, header_h, scale);
    SDL_Rect content{0, header_h, width, height - header_h};
    switch (state.page) {
    case Page::place: draw_place(renderer, session, state, telemetry, content, scale); break;
    case Page::actor: draw_actor(renderer, session, state, telemetry, content, scale); break;
    case Page::fx: draw_fx(renderer, session, state, telemetry, content, scale); break;
    case Page::master: draw_master(renderer, session, state, content, scale); break;
    case Page::memory: draw_memory(renderer, session, state, content, scale); break;
    }
    if (state.picker != PickerKind::none) draw_picker(renderer, session, state, width, height, scale);
    if (!state.toast.empty() && SDL_GetTicks() < state.toast_until) {
        const int toast_h = std::max(58, height / 12);
        SDL_Rect rect{width / 5, height - toast_h - 18, width * 3 / 5, toast_h};
        fill(renderer, rect, {12, 10, 17, 245}); outline(renderer, rect, kGreen);
        centered_text(renderer, rect, state.toast, kInk, scale);
    }
    SDL_RenderPresent(renderer);
}

HitTarget hit_at(const UiState& state, int x, int y) {
    for (auto it = state.hits.rbegin(); it != state.hits.rend(); ++it) {
        if (contains(it->rect, x, y)) return *it;
    }
    return {};
}

void open_special_picker(UiState& state, int sentinel) {
    if (sentinel == 99) state.picker = PickerKind::scale;
    else if (sentinel == 98) state.picker = PickerKind::output;
    else if (sentinel == 97) state.picker = PickerKind::plaits_model;
    else if (sentinel == 96) state.picker = PickerKind::mod_destination;
    else if (sentinel == 95) state.picker = PickerKind::mod_wave;
    state.picker_page = 0;
}

bool handle_press(cd::Session& session, UiState& state, HitTarget hit, int x) {
    if (hit.action == Action::slider) {
        state.pressed = hit;
        state.slider_active = true;
        const float normalized = static_cast<float>(x - hit.rect.x) /
            static_cast<float>(std::max(1, hit.rect.w));
        set_slider_value(session, state, hit, normalized);
        return true;
    }
    state.pressed = hit;
    return false;
}

bool handle_release(cd::Session& session, UiState& state, int x, int y) {
    state.slider_active = false;
    if (state.pressed.action == Action::none || !contains(state.pressed.rect, x, y)) {
        state.pressed = {};
        return false;
    }
    HitTarget hit = state.pressed;
    state.pressed = {};
    if (hit.action == Action::actor_section && hit.a >= 95) {
        open_special_picker(state, hit.a);
        return false;
    }
    return execute_action(session, state, hit);
}

bool update_fade(cd::Session& session, UiState& state, float seconds) {
    if (!state.fade_active) return false;
    const float duration = state.fade_target > session.performance.fade
        ? session.fade_in_seconds : session.fade_out_seconds;
    const float step = seconds / std::max(0.05F, duration);
    if (state.fade_target > session.performance.fade) {
        session.performance.fade = std::min(state.fade_target, session.performance.fade + step);
    } else {
        session.performance.fade = std::max(state.fade_target, session.performance.fade - step);
    }
    if (std::abs(session.performance.fade - state.fade_target) < 0.0001F) {
        session.performance.fade = state.fade_target;
        state.fade_active = false;
    }
    return true;
}

} // namespace

extern "C" int SDL_main(int argc, char** argv) {
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    prepare_storage();
    SDL_Window* window = SDL_CreateWindow("Cursed Drone", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, 1280, 720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer* renderer = window != nullptr
        ? SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)
        : nullptr;
    if (renderer == nullptr && window != nullptr) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (window == nullptr || renderer == nullptr) {
        std::fprintf(stderr, "SDL video: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);
    show_splash(renderer, width, height);

    cd::Session session = cd::make_default_session();
    if (!g_autosave_path.empty() && std::filesystem::exists(g_autosave_path)) {
        std::string error;
        if (!cd::load_session(g_autosave_path, session, error)) {
            std::fprintf(stderr, "autosave load: %s\n", error.c_str());
        }
    }
    session.performance.morph = 0.0F;
    session.performance.morph_target = session.scene;

    const int native_rate = parse_argument(argc, argv, "--android-audio-rate=", 48'000);
    const int burst = parse_argument(argc, argv, "--android-audio-burst=", 256);
    AudioBridge audio{};
    SDL_AudioSpec desired{};
    SDL_AudioSpec obtained{};
    desired.freq = native_rate;
    desired.format = AUDIO_F32SYS;
    desired.channels = 2;
    const int callback_frames = std::clamp(
        next_power_of_two(std::max(512, burst * 3)), 512, 2048);
    desired.samples = static_cast<Uint16>(callback_frames);
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

    UiState state{};
    refresh_memory_cache(state);
    bool running = true;
    bool changed = false;
    bool save_pending = false;
    Uint32 changed_at = 0;
    Uint32 previous = SDL_GetTicks();
    while (running) {
        SDL_GetRendererOutputSize(renderer, &width, &height);
        SDL_Event event{};
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_AC_BACK) {
                    if (state.picker != PickerKind::none) state.picker = PickerKind::none;
                    else if (state.page != Page::place) state.page = Page::place;
                    else running = false;
                }
            } else if (event.type == SDL_FINGERDOWN && !state.finger_down) {
                state.finger_down = true;
                state.finger_id = event.tfinger.fingerId;
                const int x = static_cast<int>(std::lround(event.tfinger.x * width));
                const int y = static_cast<int>(std::lround(event.tfinger.y * height));
                const HitTarget hit = hit_at(state, x, y);
                changed = handle_press(session, state, hit, x) || changed;
            } else if (event.type == SDL_FINGERMOTION && state.finger_down &&
                event.tfinger.fingerId == state.finger_id && state.slider_active) {
                const int x = static_cast<int>(std::lround(event.tfinger.x * width));
                const float normalized = static_cast<float>(x - state.pressed.rect.x) /
                    static_cast<float>(std::max(1, state.pressed.rect.w));
                set_slider_value(session, state, state.pressed, normalized);
                changed = true;
            } else if (event.type == SDL_FINGERUP && state.finger_down &&
                event.tfinger.fingerId == state.finger_id) {
                const int x = static_cast<int>(std::lround(event.tfinger.x * width));
                const int y = static_cast<int>(std::lround(event.tfinger.y * height));
                changed = handle_release(session, state, x, y) || changed;
                state.finger_down = false;
            }
        }
        const Uint32 now = SDL_GetTicks();
        changed = update_fade(session, state, static_cast<float>(now - previous) * 0.001F) || changed;
        previous = now;
        if (state.pending_trigger >= 0) {
            audio.graph.trigger_slot(static_cast<std::size_t>(state.pending_trigger));
            set_toast(state, ru(session) ? "СОБЫТИЕ ЗАПУЩЕНО" : "EVENT TRIGGERED");
            state.pending_trigger = -1;
        }
        if (changed) {
            static_cast<void>(audio.graph.submit_session(session));
            save_pending = true;
            changed_at = now;
            changed = false;
        }
        if (save_pending && !g_autosave_path.empty() && now - changed_at >= 750U) {
            std::string error;
            if (cd::save_session(session, g_autosave_path, error)) save_pending = false;
            else changed_at = now;
        }
        draw(renderer, session, state, audio.graph.telemetry(), width, height);
        // Present-vsync is the frame limiter on hardware; a minimal yield avoids
        // adding another half-frame of touch latency after SDL_RenderPresent.
        SDL_Delay(1);
    }

    if (save_pending && !g_autosave_path.empty()) {
        std::string error;
        static_cast<void>(cd::save_session(session, g_autosave_path, error));
    }
    SDL_CloseAudioDevice(device);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

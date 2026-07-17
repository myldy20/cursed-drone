// SPDX-License-Identifier: GPL-3.0-or-later
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "bitmap_text.hpp"
#include "cursed_drone/audio.hpp"
#include "cursed_drone/i18n.hpp"
#include "cursed_drone/scala.hpp"
#include "cursed_drone/session.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

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
enum class Picker { none, scene, engine, effect };

struct AudioBridge {
    cd::AudioGraph graph{};
    std::atomic<float> cpu_load{0.0F};
    float sample_rate{48'000.0F};
};

struct UiState {
    Page page{Page::perform};
    int slot{0};
    std::array<int, 5> selected{};
    std::array<int, 4> slot_selected{};
    int effect_field{0};
    Picker picker{Picker::none};
    int picker_group{0};
    int picker_item{0};
    bool scene_track_focus{false};
    int held_direction{0};
    Uint32 held_since{0};
    Uint32 last_repeat{0};
    bool auto_fade{false};
    float fade_target{1.0F};
    Uint32 kill_flash_until{0};
    int displayed_cpu_percent{0};
    Uint32 cpu_display_updated_at{0};
    bool start_held{false};
    bool select_held{false};
    bool back_held{false};
    bool back_long_action{false};
    Uint32 back_held_since{0};
    bool lab_open{false};
    int lab_tab{0};
    int lab_field{0};
    int lab_modulator{0};
    bool stage_mode{false};
    int stage_field{0};
};

std::vector<cd::ParsedScale> g_scales{};

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

bool handheld() noexcept {
    const char* value = std::getenv("CURSED_DRONE_HANDHELD");
    return value != nullptr && value[0] != '\0' && value[0] != '0';
}

bool environment_enabled(const char* name) noexcept {
    const char* value = std::getenv(name);
    return value != nullptr && value[0] != '\0' && value[0] != '0';
}

std::filesystem::path find_brand_asset(std::string_view filename) {
    const std::string file{filename};
    std::error_code error;
    const auto usable = [&error](const std::filesystem::path& path) {
        error.clear();
        return std::filesystem::is_regular_file(path, error);
    };

    if (const char* asset_root = std::getenv("CURSED_DRONE_ASSET_DIR")) {
        const std::filesystem::path candidate = std::filesystem::path{asset_root} / file;
        if (usable(candidate)) return candidate;
    }

    const std::array<std::filesystem::path, 3> roots{
        std::filesystem::path{"assets/branding"},
        std::filesystem::path{"assets"},
        std::filesystem::path{"."},
    };
    for (const auto& root : roots) {
        const auto candidate = root / file;
        if (usable(candidate)) return candidate;
    }
    return {};
}

bool show_startup_splash(SDL_Renderer* renderer) {
    if (environment_enabled("CURSED_DRONE_SKIP_SPLASH")) return true;

    SDL_Texture* texture = nullptr;
    if (const auto path = find_brand_asset("cursed-drone-splash.bmp"); !path.empty()) {
        if (SDL_Surface* surface = SDL_LoadBMP(path.string().c_str())) {
            texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
            if (texture != nullptr) {
                // The packaged BMP is deliberately 1-bit. Tinting white to the UI cream
                // keeps the file tiny while preserving the approved branding.
                SDL_SetTextureColorMod(texture, kInk.r, kInk.g, kInk.b);
                SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
            }
        } else {
            std::fprintf(stderr, "splash load: %s\n", SDL_GetError());
        }
    }

    SDL_SetRenderDrawColor(renderer, 8, 7, 12, 255);
    SDL_RenderClear(renderer);
    if (texture != nullptr) {
        const SDL_Rect destination{0, 0, kWidth, kHeight};
        SDL_RenderCopy(renderer, texture, nullptr, &destination);
    } else {
        const std::string_view title{"CURSED DRONE"};
        const std::string_view credit{"DEVELOPED BY MYLDY DESIGN  @MYLDY20"};
        cd::ui::draw_text(renderer,
            (kWidth - cd::ui::text_width(title, 3)) / 2, 142, title, kInk, 3);
        cd::ui::draw_text(renderer,
            (kWidth - cd::ui::text_width(credit)) / 2, 218, credit, kDim);
    }
    SDL_RenderPresent(renderer);

    const Uint32 shown_at = SDL_GetTicks();
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
    }
    if (texture != nullptr) SDL_DestroyTexture(texture);
    return keep_running;
}

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
        "МАТЕРИАЛ", "АКТИВНОСТЬ", "НАПРЯЖЕНИЕ", "ДИСТАНЦИЯ", "РАЗВИТИЕ"};
    constexpr std::array<std::string_view, 5> e{
        "MATERIAL", "ACTIVITY", "TENSION", "DISTANCE", "EVOLUTION"};
    return (russian ? r : e)[static_cast<std::size_t>(index)];
}

std::string_view scene_name(cd::SceneKind scene, bool russian) noexcept {
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

std::string_view engine_name(cd::EngineKind kind, bool russian) noexcept {
    switch (kind) {
    case cd::EngineKind::diagnostic: return russian ? "ДИАГН." : "LEGACY";
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
    case cd::EngineKind::toy_voice: return russian ? "ГОЛОС" : "TOY VOICE";
    case cd::EngineKind::toy_gears: return russian ? "ШЕСТЕРНИ" : "GEARS";
    case cd::EngineKind::lullaby: return russian ? "КОЛЫБЕЛЬ" : "LULLABY";
    case cd::EngineKind::sub_drone: return russian ? "САБ-ДРОН" : "SUB DRONE";
    case cd::EngineKind::tape_drone: return russian ? "ЛЕНТА" : "TAPE DRONE";
    case cd::EngineKind::bowed_metal: return russian ? "СМЫЧОК" : "BOWED METAL";
    case cd::EngineKind::earth_rumble: return russian ? "ГУД ЗЕМЛИ" : "EARTH RUMBLE";
    case cd::EngineKind::plaits: return russian ? "МАКРО-АКТЕР" : "MACRO ACTOR";
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
    case cd::EngineKind::derelict_bed: {
        constexpr std::array<std::string_view, 4> r{"ТЕЛО", "ВОЗДУХ", "ДРЕЙФ", "ИЗНОС"};
        constexpr std::array<std::string_view, 4> e{"BODY", "AIR", "DRIFT", "WEAR"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::footsteps: {
        constexpr std::array<std::string_view, 4> r{"ПОВЕРХН.", "ЯРКОСТЬ", "СКОРОСТЬ", "КРОШКА"};
        constexpr std::array<std::string_view, 4> e{"SURFACE", "BRIGHT", "SPEED", "DEBRIS"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::door: {
        constexpr std::array<std::string_view, 4> r{"ДЕРЕВО", "ТОН", "ДВИЖЕНИЕ", "ТРЕНИЕ"};
        constexpr std::array<std::string_view, 4> e{"WOOD", "PITCH", "MOTION", "FRICTION"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::pipe: {
        constexpr std::array<std::string_view, 4> r{"РЕЗОНАНС", "ВОЗДУХ", "ПОРЫВЫ", "СТУКИ"};
        constexpr std::array<std::string_view, 4> e{"RESONANCE", "AIR", "GUSTS", "KNOCKS"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::motor: {
        constexpr std::array<std::string_view, 4> r{"РОТОР", "ЩЕТКИ", "НАГРУЗКА", "ИЗНОС"};
        constexpr std::array<std::string_view, 4> e{"ROTOR", "BRUSHES", "LOAD", "WEAR"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::machinery: {
        constexpr std::array<std::string_view, 4> r{"КОРПУС", "МЕТАЛЛ", "ЦИКЛ", "ВЫХЛОП"};
        constexpr std::array<std::string_view, 4> e{"BODY", "METAL", "CYCLE", "EXHAUST"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::crowd: {
        constexpr std::array<std::string_view, 4> r{"ФОРМАНТЫ", "ЯРКОСТЬ", "ФРАЗЫ", "МАССА"};
        constexpr std::array<std::string_view, 4> e{"FORMANTS", "BRIGHT", "PHRASES", "MASS"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::metal: {
        constexpr std::array<std::string_view, 4> r{"МАТЕРИАЛ", "ЯРКОСТЬ", "РАЗМАХ", "ТРЕНИЕ"};
        constexpr std::array<std::string_view, 4> e{"MATERIAL", "BRIGHT", "SWEEP", "FRICTION"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::wind: {
        constexpr std::array<std::string_view, 4> r{"ПОРЫВ", "ВЫСОТА", "ДВИЖЕНИЕ", "ПЫЛЬ"};
        constexpr std::array<std::string_view, 4> e{"GUST", "HEIGHT", "MOTION", "DUST"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::birds: {
        constexpr std::array<std::string_view, 4> r{"ГОЛОС", "КОНТУР", "ФРАЗЫ", "ЗЕРНО"};
        constexpr std::array<std::string_view, 4> e{"VOICE", "CONTOUR", "PHRASES", "GRAIN"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::insects: {
        constexpr std::array<std::string_view, 4> r{"ВИД", "ВЫСОТА", "РОЙ", "ШОРОХ"};
        constexpr std::array<std::string_view, 4> e{"SPECIES", "HEIGHT", "SWARM", "SCRATCH"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::signal: {
        constexpr std::array<std::string_view, 4> r{"ФОРМА", "ОБЕРТОН", "СКОРОСТЬ", "ПЫЛЬ"};
        constexpr std::array<std::string_view, 4> e{"SHAPE", "OVERTONE", "SPEED", "DUST"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::cave_air: {
        constexpr std::array<std::string_view, 4> r{"ПОЛОСТЬ", "ВЛАГА", "ДЫХАНИЕ", "ТУМАН"};
        constexpr std::array<std::string_view, 4> e{"CAVITY", "MOISTURE", "BREATH", "MIST"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::water_drip: {
        constexpr std::array<std::string_view, 4> r{"РАЗМЕР", "ВЫСОТА", "ИНТЕРВАЛ", "КАПЕЛЬ"};
        constexpr std::array<std::string_view, 4> e{"SIZE", "PITCH", "INTERVAL", "DENSITY"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::water_flow: {
        constexpr std::array<std::string_view, 4> r{"РУСЛО", "ПЕНА", "ТЕЧЕНИЕ", "ПУЗЫРИ"};
        constexpr std::array<std::string_view, 4> e{"CHANNEL", "FOAM", "CURRENT", "BUBBLES"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::stone: {
        constexpr std::array<std::string_view, 4> r{"ПОРОДА", "РАЗМЕР", "УДАРЫ", "КРОШКА"};
        constexpr std::array<std::string_view, 4> e{"ROCK", "SIZE", "IMPACTS", "DEBRIS"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::metro_traction: {
        constexpr std::array<std::string_view, 4> r{"РОТОР", "ИНВЕРТОР", "РАЗГОН", "ПОДШИП."};
        constexpr std::array<std::string_view, 4> e{"ROTOR", "INVERTER", "ACCEL", "BEARING"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::rail_joint: {
        constexpr std::array<std::string_view, 4> r{"РЕЛЬС", "ЗВОН", "СКОРОСТЬ", "СТЫК"};
        constexpr std::array<std::string_view, 4> e{"RAIL", "RING", "SPEED", "JOINT"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::brake: {
        constexpr std::array<std::string_view, 4> r{"КОЛОДКА", "ВИЗГ", "ДЛИНА", "ТРЕНИЕ"};
        constexpr std::array<std::string_view, 4> e{"PAD", "SCREECH", "LENGTH", "FRICTION"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::carriage: {
        constexpr std::array<std::string_view, 4> r{"КУЗОВ", "ДРЕБЕЗГ", "КАЧКА", "БОЛТЫ"};
        constexpr std::array<std::string_view, 4> e{"BODY", "RATTLE", "SWAY", "BOLTS"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::music_box: {
        constexpr std::array<std::string_view, 4> r{"ЯЗЫЧОК", "БЛЕСК", "ФРАЗЫ", "КОРПУС"};
        constexpr std::array<std::string_view, 4> e{"TINE", "SHIMMER", "PHRASES", "BODY"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::toy_voice: {
        constexpr std::array<std::string_view, 4> r{"ГЛАСНЫЕ", "СХЕМА", "СЛОГИ", "ПОЛОМКА"};
        constexpr std::array<std::string_view, 4> e{"VOWELS", "CIRCUIT", "SYLLABLES", "DAMAGE"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::toy_gears: {
        constexpr std::array<std::string_view, 4> r{"МОТОР", "ЗУБЬЯ", "СКОРОСТЬ", "БАТАРЕЯ"};
        constexpr std::array<std::string_view, 4> e{"MOTOR", "TEETH", "SPEED", "BATTERY"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::lullaby: {
        constexpr std::array<std::string_view, 4> r{"СТРУНА", "СТЕКЛО", "ТЕМП", "РАСПАД"};
        constexpr std::array<std::string_view, 4> e{"STRING", "GLASS", "PACE", "DECAY"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::sub_drone: {
        constexpr std::array<std::string_view, 4> r{"ТЕЛО", "ОБЕРТОН", "ДЫХАНИЕ", "ВОЗДУХ"};
        constexpr std::array<std::string_view, 4> e{"BODY", "OVERTONE", "BREATH", "AIR"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::tape_drone: {
        constexpr std::array<std::string_view, 4> r{"ЛЕНТА", "ТОН", "ПЛАВАНИЕ", "ИЗНОС"};
        constexpr std::array<std::string_view, 4> e{"TAPE", "TONE", "WOW", "WEAR"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::bowed_metal: {
        constexpr std::array<std::string_view, 4> r{"КОРПУС", "МОДЫ", "СМЫЧОК", "ТРЕНИЕ"};
        constexpr std::array<std::string_view, 4> e{"BODY", "MODES", "BOW", "FRICTION"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::earth_rumble: {
        constexpr std::array<std::string_view, 4> r{"МАССА", "ПОРОДА", "СДВИГ", "УДАРЫ"};
        constexpr std::array<std::string_view, 4> e{"MASS", "GROUND", "HEAVE", "IMPACTS"};
        return (russian ? r : e)[static_cast<std::size_t>(character)];
    }
    case cd::EngineKind::plaits: {
        constexpr std::array<std::string_view, 4> r{"ГАРМОНИКИ", "ТЕМБР", "МОРФ", "РАСПАД"};
        constexpr std::array<std::string_view, 4> e{"HARMONICS", "TIMBRE", "MORPH", "DECAY"};
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
    case cd::EffectKind::bypass: return russian ? "ПУСТО" : "EMPTY";
    case cd::EffectKind::drive: return russian ? "ДРАЙВ" : "DRIVE";
    case cd::EffectKind::lowpass: return russian ? "ФИЛЬТР" : "LOWPASS";
    case cd::EffectKind::highpass: return russian ? "ВЧ-ФИЛЬТР" : "HIGHPASS";
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

int effect_field_count(cd::EffectKind kind) noexcept {
    switch (kind) {
    case cd::EffectKind::bypass: return 0;
    case cd::EffectKind::drive: return 1;
    case cd::EffectKind::lowpass:
    case cd::EffectKind::highpass:
    case cd::EffectKind::tremolo:
    case cd::EffectKind::crusher:
    case cd::EffectKind::wavefolder:
    case cd::EffectKind::ringmod: return 2;
    case cd::EffectKind::delay:
    case cd::EffectKind::comb:
    case cd::EffectKind::chorus:
    case cd::EffectKind::flanger:
    case cd::EffectKind::phaser:
    case cd::EffectKind::diffuser:
    case cd::EffectKind::ahdr:
    case cd::EffectKind::tape_void:
    case cd::EffectKind::black_hole:
    case cd::EffectKind::ritual_gate:
    case cd::EffectKind::rust_cloud:
    case cd::EffectKind::deep_sea:
    case cd::EffectKind::granular_reverse: return 3;
    }
    return 0;
}

std::string_view effect_field(cd::EffectKind kind, int index, bool russian) noexcept {
    switch (kind) {
    case cd::EffectKind::bypass: return russian ? "НЕТ ПАРАМЕТРОВ" : "NO PARAMETERS";
    case cd::EffectKind::drive: return russian ? "ПЕРЕГРУЗ" : "DRIVE";
    case cd::EffectKind::lowpass:
    case cd::EffectKind::highpass:
        return index == 0 ? (russian ? "ПОДМЕСЬ" : "MIX") : (russian ? "СРЕЗ" : "CUTOFF");
    case cd::EffectKind::tremolo:
        return index == 0 ? (russian ? "ГЛУБИНА" : "DEPTH") : (russian ? "СКОРОСТЬ" : "RATE");
    case cd::EffectKind::delay:
        if (index == 0) return russian ? "ПОДМЕСЬ" : "MIX";
        if (index == 1) return russian ? "ВРЕМЯ" : "TIME";
        return russian ? "ПОВТОРЫ" : "FEEDBACK";
    case cd::EffectKind::crusher:
        return index == 0 ? (russian ? "ДРОБЛЕНИЕ" : "CRUSH") : (russian ? "ЧАСТОТА" : "RATE");
    case cd::EffectKind::wavefolder:
        return index == 0 ? (russian ? "СКЛАДКИ" : "FOLD") : (russian ? "ГЛУБИНА" : "DEPTH");
    case cd::EffectKind::ringmod:
        return index == 0 ? (russian ? "ПОДМЕСЬ" : "MIX") : (russian ? "ЧАСТОТА" : "RATE");
    case cd::EffectKind::comb:
        if (index == 0) return russian ? "ПОДМЕСЬ" : "MIX";
        if (index == 1) return russian ? "РАЗМЕР" : "SIZE";
        return russian ? "РЕЗОНАНС" : "RESONANCE";
    case cd::EffectKind::chorus:
        if (index == 0) return russian ? "ПОДМЕСЬ" : "MIX";
        if (index == 1) return russian ? "СКОРОСТЬ" : "RATE";
        return russian ? "ГЛУБИНА" : "DEPTH";
    case cd::EffectKind::flanger:
    case cd::EffectKind::phaser:
        if (index == 0) return russian ? "ПОДМЕСЬ" : "MIX";
        if (index == 1) return russian ? "СКОРОСТЬ" : "RATE";
        return russian ? "ОБР. СВЯЗЬ" : "FEEDBACK";
    case cd::EffectKind::diffuser:
        if (index == 0) return russian ? "ПОДМЕСЬ" : "MIX";
        if (index == 1) return russian ? "РАЗМЕР" : "SIZE";
        return russian ? "РАСПАД" : "DECAY";
    case cd::EffectKind::ahdr:
        if (index == 0) return russian ? "ГЛУБИНА" : "DEPTH";
        if (index == 1) return russian ? "СКОРОСТЬ" : "RATE";
        return russian ? "ФОРМА" : "SHAPE";
    case cd::EffectKind::tape_void:
    case cd::EffectKind::black_hole:
    case cd::EffectKind::ritual_gate:
    case cd::EffectKind::rust_cloud:
    case cd::EffectKind::deep_sea:
        if (index == 0) return russian ? "ИНТЕНСИВН." : "INTENSITY";
        if (index == 1) return russian ? "ХАРАКТЕР" : "CHARACTER";
        return russian ? "ДВИЖЕНИЕ" : "MOTION";
    case cd::EffectKind::granular_reverse:
        if (index == 0) return russian ? "ПОДМЕСЬ" : "MIX";
        if (index == 1) return russian ? "ЗЕРНО" : "GRAIN";
        return russian ? "ОБР. СВЯЗЬ" : "FEEDBACK";
    }
    return {};
}

constexpr std::array<std::array<cd::EngineKind, 4>, 8> kEngineGroups{{
    {cd::EngineKind::macro, cd::EngineKind::body, cd::EngineKind::grain, cd::EngineKind::particle},
    {cd::EngineKind::derelict_bed, cd::EngineKind::footsteps, cd::EngineKind::door, cd::EngineKind::pipe},
    {cd::EngineKind::motor, cd::EngineKind::machinery, cd::EngineKind::crowd, cd::EngineKind::metal},
    {cd::EngineKind::wind, cd::EngineKind::birds, cd::EngineKind::insects, cd::EngineKind::signal},
    {cd::EngineKind::cave_air, cd::EngineKind::water_drip, cd::EngineKind::water_flow, cd::EngineKind::stone},
    {cd::EngineKind::metro_traction, cd::EngineKind::rail_joint, cd::EngineKind::brake, cd::EngineKind::carriage},
    {cd::EngineKind::music_box, cd::EngineKind::toy_voice, cd::EngineKind::toy_gears, cd::EngineKind::lullaby},
    {cd::EngineKind::sub_drone, cd::EngineKind::tape_drone, cd::EngineKind::bowed_metal, cd::EngineKind::earth_rumble},
}};

constexpr std::array<cd::EffectKind, 15> kBasicEffects{
    cd::EffectKind::bypass, cd::EffectKind::drive, cd::EffectKind::lowpass,
    cd::EffectKind::highpass, cd::EffectKind::tremolo, cd::EffectKind::delay,
    cd::EffectKind::crusher, cd::EffectKind::wavefolder, cd::EffectKind::ringmod,
    cd::EffectKind::comb, cd::EffectKind::chorus, cd::EffectKind::flanger,
    cd::EffectKind::phaser, cd::EffectKind::diffuser, cd::EffectKind::ahdr};

constexpr std::array<cd::EffectKind, 6> kCompoundEffects{
    cd::EffectKind::tape_void, cd::EffectKind::black_hole,
    cd::EffectKind::ritual_gate, cd::EffectKind::rust_cloud,
    cd::EffectKind::deep_sea, cd::EffectKind::granular_reverse};

int effect_group_size(int group) noexcept {
    return group == 0 ? static_cast<int>(kBasicEffects.size())
                      : static_cast<int>(kCompoundEffects.size());
}

cd::EffectKind effect_at(int group, int item) noexcept {
    if (group == 0) return kBasicEffects[static_cast<std::size_t>(item)];
    return kCompoundEffects[static_cast<std::size_t>(item)];
}

std::string_view effect_group_name(int group, bool russian) noexcept {
    if (group == 0) return russian ? "БАЗОВЫЕ" : "BASIC";
    return russian ? "СОСТАВНЫЕ" : "COMPOUND";
}

constexpr std::array<cd::SceneKind, 10> kScenes{
    cd::SceneKind::derelict, cd::SceneKind::factory, cd::SceneKind::wasteland,
    cd::SceneKind::wet_cave, cd::SceneKind::metro, cd::SceneKind::nursery,
    cd::SceneKind::bunker, cd::SceneKind::power_grid, cd::SceneKind::deep_water,
    cd::SceneKind::ash_field};

int parameter(const UiState& state) noexcept;

std::string_view engine_group_name(int group, bool russian) noexcept {
    constexpr std::array<std::string_view, 8> r{
        "ОБЩИЕ", "ЗАБРОШЕННОЕ", "ЦЕХ", "ПУСТОШЬ", "ПЕЩЕРА", "МЕТРО", "ДЕТСКАЯ", "ДРОНЫ"};
    constexpr std::array<std::string_view, 8> e{
        "GENERAL", "DERELICT", "FACTORY", "WASTELAND", "CAVE", "METRO", "NURSERY", "DRONES"};
    return (russian ? r : e)[static_cast<std::size_t>(group)];
}

void open_engine_picker(UiState& state, const cd::Session& session) noexcept {
    const auto current = session.slots[static_cast<std::size_t>(state.slot)].engine;
    state.picker = Picker::engine;
    state.picker_group = 0;
    state.picker_item = 0;
    for (int group = 0; group < static_cast<int>(kEngineGroups.size()); ++group) {
        for (int item = 0; item < 4; ++item) {
            if (kEngineGroups[static_cast<std::size_t>(group)][static_cast<std::size_t>(item)] == current) {
                state.picker_group = group;
                state.picker_item = item;
            }
        }
    }
}

void open_scene_picker(UiState& state, const cd::Session& session) noexcept {
    state.picker = Picker::scene;
    state.picker_group = 0;
    state.picker_item = 0;
    for (int item = 0; item < static_cast<int>(kScenes.size()); ++item) {
        if (kScenes[static_cast<std::size_t>(item)] == session.scene) state.picker_item = item;
    }
}

void open_effect_picker(UiState& state, const cd::Session& session) noexcept {
    const auto current = session.slots[static_cast<std::size_t>(state.slot)]
        .effects[static_cast<std::size_t>(parameter(state))].kind;
    state.picker = Picker::effect;
    state.picker_group = 0;
    state.picker_item = 0;
    for (int group = 0; group < 2; ++group) {
        for (int item = 0; item < effect_group_size(group); ++item) {
            if (effect_at(group, item) == current) {
                state.picker_group = group;
                state.picker_item = item;
            }
        }
    }
}

void move_picker(UiState& state, int horizontal, int vertical) noexcept {
    if (state.picker == Picker::scene) {
        state.picker_item = (state.picker_item + horizontal + vertical +
            static_cast<int>(kScenes.size())) % static_cast<int>(kScenes.size());
    } else if (state.picker == Picker::engine) {
        const int groups = static_cast<int>(kEngineGroups.size());
        state.picker_group = (state.picker_group + horizontal + groups) % groups;
        state.picker_item = (state.picker_item + vertical + 4) % 4;
    } else if (state.picker == Picker::effect) {
        if (horizontal != 0) {
            state.picker_group = (state.picker_group + horizontal + 2) % 2;
            state.picker_item = std::min(state.picker_item, effect_group_size(state.picker_group) - 1);
        }
        if (vertical != 0) {
            const int effects = effect_group_size(state.picker_group);
            state.picker_item = (state.picker_item + vertical + effects) % effects;
        }
    }
}

void confirm_picker(UiState& state, cd::Session& session) noexcept {
    if (state.picker == Picker::scene) {
        cd::apply_scene_recipe(session, kScenes[static_cast<std::size_t>(state.picker_item)]);
    } else if (state.picker == Picker::engine) {
        session.slots[static_cast<std::size_t>(state.slot)].engine =
            kEngineGroups[static_cast<std::size_t>(state.picker_group)]
                [static_cast<std::size_t>(state.picker_item)];
        session.scene_modified = true;
    } else if (state.picker == Picker::effect) {
        session.slots[static_cast<std::size_t>(state.slot)]
            .effects[static_cast<std::size_t>(parameter(state))].kind =
            effect_at(state.picker_group, state.picker_item);
        state.effect_field = 0;
        session.scene_modified = true;
    }
    state.picker = Picker::none;
}

int page_index(Page page) noexcept { return static_cast<int>(page); }

int parameter_count(Page page) noexcept {
    switch (page) {
    case Page::perform: return 5;
    case Page::slot: return 8;
    case Page::effects: return 4;
    case Page::master: return 2;
    case Page::setup: return 3;
    }
    return 1;
}

int& parameter(UiState& state) noexcept {
    if (state.page == Page::slot) {
        return state.slot_selected[static_cast<std::size_t>(state.slot)];
    }
    return state.selected[static_cast<std::size_t>(page_index(state.page))];
}

int parameter(const UiState& state) noexcept {
    if (state.page == Page::slot) {
        return state.slot_selected[static_cast<std::size_t>(state.slot)];
    }
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
            (state.scene_track_focus
                    ? session.slots[static_cast<std::size_t>(state.slot)].level
                    : macro_value(session.performance, selected)) * 100.0F)));
        break;
    case Page::slot: {
        const int slot = slot_override >= 0 ? slot_override : state.slot;
        if (selected == 0) {
            return std::string{engine_name(
                session.slots[static_cast<std::size_t>(slot)].engine, ru(session))};
        }
        const float value = slot_value(session.slots[static_cast<std::size_t>(slot)], selected);
        if (selected == 1) {
            std::snprintf(result, sizeof(result), "%.1f HZ", static_cast<double>(value));
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
        if (effect_field_count(effect.kind) == 0) {
            return std::string{effect_name(effect.kind, ru(session))};
        }
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
        }
        break;
    case Page::setup:
        if (selected == 0) {
            return session.locale == cd::Locale::ru ? "РУССКИЙ" : "ENGLISH";
        }
        std::snprintf(result, sizeof(result), "%.2f S", static_cast<double>(
            selected == 1 ? session.fade_in_seconds : session.fade_out_seconds));
        break;
    }
    return result;
}

std::string current_label(const cd::Session& session, const UiState& state) {
    switch (state.page) {
    case Page::perform:
        if (state.scene_track_focus) {
            return std::string{ru(session) ? "УРОВЕНЬ ДОРОЖКИ " : "TRACK LEVEL "} +
                std::to_string(state.slot + 1);
        }
        return std::string{macro_name(parameter(state), ru(session))};
    case Page::slot: return std::string{slot_name(
        parameter(state), session.slots[static_cast<std::size_t>(state.slot)], session.locale)};
    case Page::effects:
    {
        const auto kind = session.slots[static_cast<std::size_t>(state.slot)]
            .effects[static_cast<std::size_t>(parameter(state))].kind;
        return "FX " + std::to_string(parameter(state) + 1) + " " +
            std::string{effect_field(kind, state.effect_field, ru(session))};
    }
    case Page::master:
        if (parameter(state) == 0) return std::string{cd::tr(session.locale, cd::TextId::master)};
        return ru(session) ? "ТЕМП ПРОЦЕССОВ" : "PROCESS TEMPO";
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
        if (state.scene_track_focus) {
            auto& level = session.slots[static_cast<std::size_t>(state.slot)].level;
            level = std::clamp(level + steps * 0.01F, 0.0F, 1.0F);
            session.scene_modified = true;
        } else {
            float* value = macro_value(session.performance, selected);
            *value = std::clamp(*value + steps * 0.01F, 0.0F, 1.0F);
        }
        break;
    }
    case Page::slot: {
        if (selected == 0) {
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
        session.scene_modified = true;
        break;
    }
    case Page::effects: {
        auto& effect = session.slots[static_cast<std::size_t>(state.slot)]
            .effects[static_cast<std::size_t>(selected)];
        if (effect_field_count(effect.kind) == 0) {
            break;
        }
        float* value = effect_value(effect, state.effect_field);
        *value = std::clamp(*value + steps * 0.01F, 0.0F, 1.0F);
        session.scene_modified = true;
        break;
    }
    case Page::master:
        if (selected == 0) {
            session.master_level = std::clamp(session.master_level + steps * 0.01F, 0.0F, 1.0F);
        } else if (selected == 1) {
            session.tempo_bpm = std::clamp(session.tempo_bpm + steps, 10.0F, 300.0F);
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
        (state.page == Page::effects && effect_field_count(
            session.slots[static_cast<std::size_t>(state.slot)]
                .effects[static_cast<std::size_t>(parameter(state))].kind) == 0) ||
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

void draw_myldy_mark(SDL_Renderer* renderer, int x, int y, int scale, SDL_Color color) {
    const int s = std::max(1, scale);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    fill(renderer, {x, y, 4 * s, 15 * s}, color);
    fill(renderer, {x + 7 * s, y, 3 * s, 15 * s}, color);
    fill(renderer, {x + 18 * s, y, 3 * s, 15 * s}, color);
    fill(renderer, {x + 24 * s, y, 4 * s, 15 * s}, color);
    for (int offset = 0; offset < 3 * s; ++offset) {
        SDL_RenderDrawLine(renderer,
            x + 9 * s + offset, y,
            x + 14 * s + offset, y + 9 * s);
        SDL_RenderDrawLine(renderer,
            x + 14 * s + offset, y + 9 * s,
            x + 19 * s + offset, y);
    }
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


void segmented_output_meter(SDL_Renderer* renderer, int x, int y, int width, int height,
    float rms, float peak) {
    constexpr int segments = 10;
    constexpr int gap = 2;
    const int segment_width = (width - gap * (segments - 1)) / segments;
    const float level = std::clamp(rms * 4.0F, 0.0F, 1.0F);
    const float peak_level = std::clamp(peak * 1.8F, 0.0F, 1.0F);
    for (int index = 0; index < segments; ++index) {
        const float threshold = static_cast<float>(index + 1) / static_cast<float>(segments);
        SDL_Color color = threshold > 0.82F ? SDL_Color{225, 77, 96, 255}
            : (threshold > 0.62F ? SDL_Color{224, 154, 63, 255} : SDL_Color{80, 169, 154, 255});
        if (level < threshold) color = {38, 33, 48, 255};
        fill(renderer, {x + index * (segment_width + gap), y, segment_width, height}, color);
        if (peak_level >= threshold && peak_level < threshold + 0.1F) {
            outline(renderer, {x + index * (segment_width + gap), y, segment_width, height}, kInk);
        }
    }
}

void draw_header(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state) {
    cd::ui::draw_text(renderer, 10, 5, cd::tr(session.locale, cd::TextId::app_name), kInk, 2);
    std::string status;
    SDL_Color status_color = kDim;
    if (state.auto_fade) {
        status = std::string{ru(session) ? "ФЕЙД " : "FADE "} +
            (state.fade_target > session.performance.fade ? "IN " : "OUT ") +
            std::to_string(static_cast<int>(std::lround(session.performance.fade * 100.0F))) + "%";
        status_color = {91, 218, 179, 255};
    } else {
        status = "DSP " + std::to_string(state.displayed_cpu_percent) + "%";
        if (state.displayed_cpu_percent >= 75) status_color = kFxColors[0];
    }
    const int meter_width = 80;
    const int meter_x = kWidth - 10 - meter_width;
    cd::ui::draw_text(renderer, meter_x - 12 - cd::ui::text_width(status), 9, status, status_color);
    segmented_output_meter(renderer, meter_x, 8, meter_width, 9,
        telemetry.master_rms, telemetry.master_peak);
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

std::string_view macro_endpoint(int index, bool high, bool russian) noexcept {
    constexpr std::array<std::string_view, 5> r_low{
        "ГЛАДКО", "РЕДКО", "СПОКОЙНО", "БЛИЗКО", "СТАТИЧНО"};
    constexpr std::array<std::string_view, 5> r_high{
        "ГРУБО", "ПЛОТНО", "НЕСТАБИЛЬНО", "ДАЛЕКО", "МЕНЯЕТСЯ"};
    constexpr std::array<std::string_view, 5> e_low{
        "SMOOTH", "SPARSE", "STABLE", "NEAR", "STATIC"};
    constexpr std::array<std::string_view, 5> e_high{
        "ROUGH", "BUSY", "UNSTABLE", "FAR", "CHANGING"};
    return (russian ? (high ? r_high : r_low) : (high ? e_high : e_low))
        [static_cast<std::size_t>(index)];
}

void draw_scene(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state) {
    fill(renderer, {10, 48, 492, 306}, kPanel);
    const std::string landscape = std::string{ru(session) ? "ЛАНДШАФТ: " : "LANDSCAPE: "} +
        std::string{scene_name(session.scene, ru(session))} + (session.scene_modified ? " *" : "");
    cd::ui::draw_text(renderer, 22, 56, landscape, session.scene_modified ? kFxColors[1] : kInk);
    cd::ui::draw_text(renderer, 350, 56,
        session.scene_modified ? (ru(session) ? "ИЗМЕНЕН" : "MODIFIED")
                               : (ru(session) ? "ПРЕСЕТ" : "PRESET"),
        session.scene_modified ? kFxColors[1] : kDim);
    for (int index = 0; index < 5; ++index) {
        const int y = 78 + index * 34;
        const bool selected = !state.scene_track_focus && parameter(state) == index;
        if (selected) {
            fill(renderer, {18, y - 4, 476, 31}, {73, 46, 104, 255});
            outline(renderer, {18, y - 4, 476, 31}, kInk);
        }
        cd::ui::draw_text(renderer, 26, y, macro_name(index, ru(session)), selected ? kInk : kDim);
        char number[12]{};
        std::snprintf(number, sizeof(number), "%d%%", static_cast<int>(std::lround(
            macro_value(session.performance, index) * 100.0F)));
        cd::ui::draw_text(renderer, 168 - cd::ui::text_width(number), y, number, selected ? kInk : kDim);
        bar(renderer, 190, y, 294, 9, macro_value(session.performance, index),
            selected ? kInk : kFxColors[static_cast<std::size_t>(index % 4)]);
        const auto low = macro_endpoint(index, false, ru(session));
        const auto high = macro_endpoint(index, true, ru(session));
        cd::ui::draw_text(renderer, 190, y + 13, low, selected ? kInk : kDim);
        cd::ui::draw_text(renderer, 484 - cd::ui::text_width(high), y + 13, high, selected ? kInk : kDim);
    }

    const char* track_help = handheld()
        ? (ru(session) ? "ДОРОЖКИ  LT/RT ВЫБОР  L/R УРОВЕНЬ  B MUTE"
                       : "TRACKS  LT/RT SELECT  L/R LEVEL  B MUTE")
        : (ru(session) ? "ДОРОЖКИ  LT/RT ВЫБОР  A/D УРОВЕНЬ  SPACE MUTE"
                       : "TRACKS  LT/RT SELECT  A/D LEVEL  SPACE MUTE");
    cd::ui::draw_text(renderer, 22, 250, track_help,
        state.scene_track_focus ? kInk : kDim);
    for (int index = 0; index < 4; ++index) {
        const int x = 18 + index * 119;
        const auto& slot = session.slots[static_cast<std::size_t>(index)];
        const bool active = state.slot == index;
        fill(renderer, {x, 264, 111, 78}, active
            ? (state.scene_track_focus ? SDL_Color{90, 53, 126, 255} : SDL_Color{73, 46, 104, 255})
            : SDL_Color{27, 23, 36, 255});
        if (active) outline(renderer, {x, 264, 111, 78}, state.scene_track_focus ? kInk : kDim);
        const std::string title = std::to_string(index + 1) + " " +
            std::string{engine_name(slot.engine, ru(session))};
        cd::ui::draw_text(renderer, x + 6, 272, title, active ? kInk : kDim);
        const float meter = std::clamp(telemetry.slot_rms[static_cast<std::size_t>(index)] * 4.2F, 0.0F, 1.0F);
        bar(renderer, x + 6, 291, 99, 10, meter, react(kFxColors[static_cast<std::size_t>(index)], meter));
        char level[12]{};
        std::snprintf(level, sizeof(level), "LVL %d%%", static_cast<int>(std::lround(slot.level * 100.0F)));
        cd::ui::draw_text(renderer, x + 6, 309, level,
            active && state.scene_track_focus ? kInk : kDim);
        if (!slot.enabled) {
            cd::ui::draw_text(renderer, x + 6, 325, "MUTE", kFxColors[0]);
        }
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
        cd::ui::draw_text(renderer, x + 7, 69,
            engine_name(session.slots[static_cast<std::size_t>(index)].engine, ru(session)), kDim);
        scope(renderer, x + 8, 84, panel_width - 16, 62,
            telemetry.slot_scope[static_cast<std::size_t>(index)],
            telemetry.slot_rms[static_cast<std::size_t>(index)],
            telemetry.slot_peak[static_cast<std::size_t>(index)]);
        const auto& slot = session.slots[static_cast<std::size_t>(index)];
        const int selected_parameter = state.slot_selected[static_cast<std::size_t>(index)];
        for (int slot_parameter = 0; slot_parameter < 8; ++slot_parameter) {
            const int y = 153 + slot_parameter * 24;
            const bool selected = index == state.slot && selected_parameter == slot_parameter;
            if (selected) fill(renderer, {x + 4, y - 3, panel_width - 8, 22}, {73, 46, 104, 255});
            SDL_Color color = selected ? kInk : react(
                kFxColors[static_cast<std::size_t>(slot_parameter % 4)], activity);
            std::string label;
            std::string shown;
            float normalized = 0.0F;
            char number[20]{};
            if (slot_parameter == 0) {
                label = ru(session) ? "ДВИЖ" : "ENG";
                shown = std::string{engine_name(slot.engine, ru(session))};
            } else {
                label = std::string{slot_name(slot_parameter, slot, session.locale)};
                const float value = slot_value(slot, slot_parameter);
                if (slot_parameter == 1) {
                    std::snprintf(number, sizeof(number), "%.0fH", static_cast<double>(value));
                    normalized = std::clamp(std::log2(value / 8.0F) / std::log2(2000.0F / 8.0F), 0.0F, 1.0F);
                } else if (slot_parameter == 7) {
                    std::snprintf(number, sizeof(number), "%+.0f", static_cast<double>(value * 100.0F));
                    normalized = value * 0.5F + 0.5F;
                } else {
                    std::snprintf(number, sizeof(number), "%d%%", static_cast<int>(std::lround(value * 100.0F)));
                    normalized = value;
                }
                shown = number;
            }
            cd::ui::draw_text(renderer, x + 6, y, label, color);
            cd::ui::draw_text(renderer, x + panel_width - 6 - cd::ui::text_width(shown), y, shown, color);
            if (slot_parameter > 0) {
                bar(renderer, x + 6, y + 12, panel_width - 12, 5, normalized, color);
            }
        }
        if (!slot.enabled) {
            fill(renderer, {x, 48, panel_width, 306}, {0, 0, 0, 168});
            cd::ui::draw_text(renderer, x + 7, 122, ru(session) ? "MUTE / ХВОСТ" : "MUTE / TAIL",
                {239, 112, 112, 255});
        }
    }
}

void effect_visual(
    SDL_Renderer* renderer,
    int x,
    int y,
    int width,
    int height,
    const cd::EffectSettings& effect,
    SDL_Color color) {
    fill(renderer, {x, y, width, height}, {18, 15, 24, 255});
    outline(renderer, {x, y, width, height}, {105, 92, 118, 255});
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    const int left = x + 5;
    const int right = x + width - 6;
    const int top = y + 5;
    const int bottom = y + height - 6;
    const int center = (top + bottom) / 2;
    if (effect.kind == cd::EffectKind::bypass) {
        SDL_RenderDrawLine(renderer, left, center, right, center);
        return;
    }
    if (effect.kind == cd::EffectKind::delay) {
        const int spacing = 7 + static_cast<int>(std::lround(effect.tone * 18.0F));
        const int count = std::max(2, (right - left) / spacing + 1);
        for (int tap = 0; tap < count; ++tap) {
            const float decay = tap == 0 ? (1.0F - effect.amount * 0.35F)
                : effect.amount * std::pow(0.28F + effect.feedback * 0.67F, static_cast<float>(tap - 1));
            const int tap_x = left + tap * spacing;
            const int tap_height = static_cast<int>(std::lround(decay * static_cast<float>(height - 14)));
            SDL_RenderDrawLine(renderer, tap_x, bottom, tap_x, bottom - tap_height);
        }
        return;
    }
    int previous_y = center;
    for (int column = 0; column <= right - left; ++column) {
        const float t = static_cast<float>(column) / static_cast<float>(std::max(1, right - left));
        float value = 0.0F;
        switch (effect.kind) {
        case cd::EffectKind::drive: {
            const float input = t * 2.0F - 1.0F;
            value = std::tanh(input * (1.0F + effect.amount * 8.0F));
            break;
        }
        case cd::EffectKind::lowpass: {
            const float cutoff = 0.06F + effect.tone * 0.86F;
            const float wet = 1.0F / (1.0F + std::pow(t / cutoff, 2.0F + effect.amount * 7.0F));
            value = ((1.0F - effect.amount) + wet * effect.amount) * 2.0F - 1.0F;
            break;
        }
        case cd::EffectKind::highpass: {
            const float cutoff = 0.06F + effect.tone * 0.86F;
            const float wet = 1.0F - 1.0F / (1.0F + std::pow(t / cutoff, 2.0F + effect.amount * 7.0F));
            value = ((1.0F - effect.amount) + wet * effect.amount) * 2.0F - 1.0F;
            break;
        }
        case cd::EffectKind::tremolo:
            value = std::sin(t * 6.2831853F * (1.0F + effect.tone * 3.0F)) * effect.amount;
            break;
        case cd::EffectKind::crusher: {
            const float horizontal_steps = 3.0F + (1.0F - effect.tone) * 26.0F;
            const float held_t = std::floor(t * horizontal_steps) / horizontal_steps;
            const float levels = 2.0F + (1.0F - effect.amount) * 18.0F;
            value = std::round((held_t * 2.0F - 1.0F) * levels) / levels;
            break;
        }
        case cd::EffectKind::wavefolder: {
            const float input = (t * 2.0F - 1.0F) * (1.0F + effect.amount * (2.0F + effect.tone * 5.0F));
            const float wrapped = std::fmod(input + 1.0F, 4.0F);
            const float positive = wrapped < 0.0F ? wrapped + 4.0F : wrapped;
            value = 1.0F - std::abs(positive - 2.0F);
            break;
        }
        case cd::EffectKind::ringmod:
            value = std::sin(t * 6.2831853F * (1.0F + effect.tone * 9.0F)) * effect.amount;
            break;
        case cd::EffectKind::comb: {
            const float teeth = 2.0F + (1.0F - effect.tone) * 12.0F;
            const float resonance = std::pow(std::abs(std::sin(t * 3.14159265F * teeth)),
                3.0F + effect.feedback * 9.0F);
            value = ((1.0F - effect.amount) + resonance * effect.amount) * 2.0F - 1.0F;
            break;
        }
        case cd::EffectKind::chorus:
        case cd::EffectKind::flanger:
        case cd::EffectKind::phaser:
            value = std::sin(t * 6.2831853F * (1.0F + effect.tone * 7.0F) +
                std::sin(t * 12.5663706F) * effect.feedback * 2.2F) * effect.amount;
            break;
        case cd::EffectKind::diffuser:
            value = (std::sin(t * 18.8495559F) + std::sin(t * 43.9822972F) * 0.52F +
                std::sin(t * 69.1150384F) * 0.25F) * effect.amount * 0.48F;
            break;
        case cd::EffectKind::ahdr: {
            const float phase = std::fmod(t * (1.0F + effect.tone * 3.0F), 1.0F);
            float envelope = 0.0F;
            if (phase < 0.13F) envelope = phase / 0.13F;
            else if (phase < 0.30F) envelope = 1.0F;
            else if (phase < 0.72F) envelope = 1.0F - (phase - 0.30F) * 1.55F;
            else envelope = std::max(0.0F, 0.35F * (1.0F - (phase - 0.72F) / 0.28F));
            value = envelope * effect.amount * 1.8F - 0.82F;
            break;
        }
        case cd::EffectKind::tape_void:
        case cd::EffectKind::black_hole:
        case cd::EffectKind::ritual_gate:
        case cd::EffectKind::rust_cloud:
        case cd::EffectKind::deep_sea:
        case cd::EffectKind::granular_reverse:
            value = std::tanh((std::sin(t * 6.2831853F * (1.0F + effect.tone * 5.0F)) +
                std::sin(t * 18.8495559F * (1.0F + effect.feedback * 2.0F)) * 0.62F +
                std::sin(t * 50.2654825F) * effect.feedback * 0.24F) *
                (0.55F + effect.amount * 2.4F));
            break;
        case cd::EffectKind::bypass:
        case cd::EffectKind::delay:
            break;
        }
        const int current_y = center - static_cast<int>(std::lround(value * static_cast<float>(height - 14) * 0.5F));
        if (column > 0) SDL_RenderDrawLine(renderer, left + column - 1, previous_y, left + column, current_y);
        previous_y = current_y;
    }
}

void draw_effects(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state) {
    constexpr int panel_width = 117;
    const int active_effect = parameter(state);
    const auto& slot = session.slots[static_cast<std::size_t>(state.slot)];
    for (int track = 0; track < 4; ++track) {
        const int x = 10 + track * 125;
        const bool selected = track == state.slot;
        const float meter = std::clamp(
            telemetry.slot_rms[static_cast<std::size_t>(track)] * 4.2F, 0.0F, 1.0F);
        fill(renderer, {x, 48, panel_width, 32}, selected ? SDL_Color{73, 46, 104, 255} : kPanel);
        if (selected) outline(renderer, {x, 48, panel_width, 32}, kInk);
        const auto& track_slot = session.slots[static_cast<std::size_t>(track)];
        const std::string source = std::to_string(track + 1) + " " +
            std::string{engine_name(track_slot.engine, ru(session))};
        cd::ui::draw_text(renderer, x + 6, 54, source, selected ? kInk : kDim);
        bar(renderer, x + 6, 68, panel_width - 12, 6, meter,
            react(kFxColors[static_cast<std::size_t>(track)], meter));
    }
    for (int index = 0; index < 4; ++index) {
        const int x = 10 + index * 125;
        fill(renderer, {x, 84, panel_width, 270}, index == active_effect ? kPurple : kPanel);
        if (index == active_effect) {
            outline(renderer, {x + 1, 85, panel_width - 2, 268}, kInk);
        }
        const std::string title = "FX " + std::to_string(index + 1);
        const auto& effect = slot.effects[static_cast<std::size_t>(index)];
        cd::ui::draw_text(renderer, x + 7, 93, title, kInk);
        cd::ui::draw_text(renderer, x + 7, 107, effect_name(effect.kind, ru(session)),
            kFxColors[static_cast<std::size_t>(index)]);
        effect_visual(renderer, x + 8, 125, panel_width - 16, 66, effect,
            kFxColors[static_cast<std::size_t>(index)]);
        const int fields = effect_field_count(effect.kind);
        if (fields == 0) {
            cd::ui::draw_text(renderer, x + 7, 214,
                ru(session) ? "СИГНАЛ БЕЗ" : "SIGNAL PASSES", kDim);
            cd::ui::draw_text(renderer, x + 7, 228,
                ru(session) ? "ОБРАБОТКИ" : "UNCHANGED", kDim);
        }
        for (int field = 0; field < fields; ++field) {
            const int y = 205 + field * 47;
            const bool selected = index == active_effect && field == state.effect_field;
            cd::ui::draw_text(renderer, x + 7, y, effect_field(effect.kind, field, ru(session)),
                selected ? kInk : kDim);
            bar(renderer, x + 7, y + 14, panel_width - 14, 10, effect_value(effect, field),
                selected ? kInk : kFxColors[static_cast<std::size_t>(index)]);
            char number[12]{};
            std::snprintf(number, sizeof(number), "%d%%", static_cast<int>(std::lround(
                effect_value(effect, field) * 100.0F)));
            cd::ui::draw_text(renderer, x + 7, y + 28, number, selected ? kInk : kDim);
        }
    }
}

void draw_master(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state) {
    fill(renderer, {10, 48, 492, 306}, kPanel);
    cd::ui::draw_text(renderer, 24, 58, ru(session) ? "ИТОГОВЫЙ СИГНАЛ" : "MASTER SIGNAL", kDim);
    scope(renderer, 24, 74, 464, 72, telemetry.master_scope, telemetry.master_rms, telemetry.master_peak);
    const std::array<std::string, 2> labels{
        ru(session) ? "ОБЩАЯ ГРОМКОСТЬ" : "MASTER LEVEL",
        ru(session) ? "ТЕМП ПРОЦЕССОВ" : "PROCESS TEMPO",
    };
    const std::array<float, 2> values{
        session.master_level, (session.tempo_bpm - 10.0F) / 290.0F};
    for (int index = 0; index < 2; ++index) {
        const int y = 164 + index * 57;
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
    std::string fade_status;
    if (state.auto_fade) {
        fade_status = state.fade_target > session.performance.fade
            ? (ru(session) ? "ВЫХОД: ОТКРЫВАЕТСЯ" : "OUTPUT: OPENING")
            : (ru(session) ? "ВЫХОД: ЗАКРЫВАЕТСЯ" : "OUTPUT: CLOSING");
    } else if (session.performance.fade >= 0.999F) {
        fade_status = ru(session) ? "ВЫХОД: ОТКРЫТ" : "OUTPUT: OPEN";
    } else if (session.performance.fade <= 0.001F) {
        fade_status = ru(session) ? "ВЫХОД: ЗАКРЫТ" : "OUTPUT: CLOSED";
    } else {
        fade_status = ru(session) ? "ВЫХОД: ПАУЗА" : "OUTPUT: PAUSED";
    }
    char fade_value[16]{};
    std::snprintf(fade_value, sizeof(fade_value), "%d%%",
        static_cast<int>(std::lround(session.performance.fade * 100.0F)));
    cd::ui::draw_text(renderer, 24, 278, fade_status, state.auto_fade ? SDL_Color{91, 218, 179, 255} : kDim);
    cd::ui::draw_text(renderer, 454 - cd::ui::text_width(fade_value), 278, fade_value, kInk);
    bar(renderer, 24, 293, 464, 11, session.performance.fade,
        state.auto_fade ? SDL_Color{91, 218, 179, 255} : kPurple);
    char next_fade[64]{};
    const bool will_open = session.performance.fade <= 0.5F;
    const char* fade_button = handheld() ? "SELECT" : "F";
    std::snprintf(next_fade, sizeof(next_fade), "%s %s: %s %.1fS",
        ru(session) ? "КНОПКА" : "BUTTON", fade_button,
        will_open ? (ru(session) ? "ОТКРЫТЬ ЗА" : "OPEN IN")
                  : (ru(session) ? "ЗАКРЫТЬ ЗА" : "CLOSE IN"),
        static_cast<double>(will_open ? session.fade_in_seconds : session.fade_out_seconds));
    cd::ui::draw_text(renderer, 24, 309, next_fade, kDim);
    char cpu[24]{};
    std::snprintf(cpu, sizeof(cpu), "DSP %d%%", state.displayed_cpu_percent);
    cd::ui::draw_text(renderer, 24, 329, cpu, state.displayed_cpu_percent < 75 ? kDim : kFxColors[0]);
    for (int slot = 0; slot < 4; ++slot) {
        const float value = telemetry.slot_rms[static_cast<std::size_t>(slot)] * 4.0F;
        bar(renderer, 112 + slot * 94, 330, 84, 8, value, react(kFxColors[static_cast<std::size_t>(slot)], value));
    }
}

void draw_setup(SDL_Renderer* renderer, const cd::Session& session, const UiState& state) {
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
            std::snprintf(seconds_text, sizeof(seconds_text), "%.2f S", static_cast<double>(
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
    SDL_SetRenderDrawColor(renderer, 75, 67, 86, 255);
    SDL_RenderDrawLine(renderer, 32, 267, 468, 267);
    constexpr std::string_view version{"EXPERIMENT 0.10"};
    cd::ui::draw_text(renderer, 32, 278, version, kDim);
    char cpu[48]{};
    std::snprintf(cpu, sizeof(cpu), "DSP %d%%", state.displayed_cpu_percent);
    cd::ui::draw_text(renderer, 468 - cd::ui::text_width(cpu), 278, cpu,
        state.displayed_cpu_percent < 75 ? kDim : kFxColors[0]);
    cd::ui::draw_text(renderer, 32, 328,
        handheld()
            ? (ru(session) ? "L/R: ИЗМЕНИТЬ / АВТОСОХРАНЕНИЕ" : "L/R: CHANGE / AUTOSAVE")
            : (ru(session) ? "A/D: ИЗМЕНИТЬ / НАСТРОЙКИ СОХРАНЯЮТСЯ АВТОМАТИЧЕСКИ"
                           : "A/D: CHANGE / SETTINGS ARE SAVED AUTOMATICALLY"),
        kDim);
}

void draw_picker(SDL_Renderer* renderer, const cd::Session& session, const UiState& state) {
    fill(renderer, {76, 52, 360, 302}, {8, 7, 12, 244});
    outline(renderer, {76, 52, 360, 302}, kInk);
    if (state.picker == Picker::scene) {
        cd::ui::draw_text(renderer, 92, 66,
            ru(session) ? "ЦЕЛЕВОЙ ЛАНДШАФТ" : "TARGET LANDSCAPE", kInk, 2);
        for (int item = 0; item < static_cast<int>(kScenes.size()); ++item) {
            const int column = item / 5;
            const int row = item % 5;
            const int x = 92 + column * 164;
            const int y = 110 + row * 42;
            const bool selected = item == state.picker_item;
            if (selected) fill(renderer, {x, y - 5, 154, 24}, {73, 46, 104, 255});
            const std::string label = std::to_string(item + 1) + "  " +
                std::string{scene_name(kScenes[static_cast<std::size_t>(item)], ru(session))};
            cd::ui::draw_text(renderer, x + 8, y, label, selected ? kInk : kDim);
        }
        cd::ui::draw_text(renderer, 92, 334,
            handheld()
                ? (ru(session) ? "UP/DN ВЫБОР   A ПРИНЯТЬ   B НАЗАД"
                               : "UP/DN SELECT   A APPLY   B BACK")
                : (ru(session) ? "UP/DN ВЫБОР   E ПРИНЯТЬ   ESC НАЗАД"
                               : "UP/DN SELECT   E APPLY   ESC BACK"), kDim);
    } else if (state.picker == Picker::engine) {
        cd::ui::draw_text(renderer, 92, 78,
            ru(session) ? "ВЫБОР ДВИЖКА" : "CHOOSE ENGINE", kInk, 2);
        for (int group = 0; group < static_cast<int>(kEngineGroups.size()); ++group) {
            const int x = 92 + (group % 3) * 108;
            const int y = 108 + (group / 3) * 21;
            const bool selected = group == state.picker_group;
            if (selected) fill(renderer, {x - 4, y - 4, 104, 17}, kPurple);
            cd::ui::draw_text(renderer, x, y, engine_group_name(group, ru(session)), selected ? kInk : kDim);
        }
        const auto& group = kEngineGroups[static_cast<std::size_t>(state.picker_group)];
        for (int item = 0; item < 4; ++item) {
            const int y = 180 + item * 32;
            const bool selected = item == state.picker_item;
            if (selected) fill(renderer, {92, y - 5, 328, 24}, {73, 46, 104, 255});
            const std::string label = std::to_string(item + 1) + "  " +
                std::string{engine_name(group[static_cast<std::size_t>(item)], ru(session))};
            cd::ui::draw_text(renderer, 102, y, label, selected ? kInk : kDim);
        }
        cd::ui::draw_text(renderer, 92, 334,
            handheld()
                ? (ru(session) ? "LT/RT ГРУППА  UP/DN ДВИЖОК  A OK  B НАЗАД"
                               : "LT/RT GROUP  UP/DN ENGINE  A OK  B BACK")
                : (ru(session) ? "LT/RT ГРУППА  UP/DN ДВИЖОК  E OK  ESC НАЗАД"
                               : "LT/RT GROUP  UP/DN ENGINE  E OK  ESC BACK"), kDim);
    } else if (state.picker == Picker::effect) {
        cd::ui::draw_text(renderer, 92, 66,
            ru(session) ? "ВЫБОР ЭФФЕКТА" : "CHOOSE EFFECT", kInk, 2);
        for (int group = 0; group < 2; ++group) {
            const int x = 92 + group * 164;
            const bool selected_group = group == state.picker_group;
            if (selected_group) fill(renderer, {x - 4, 92, 154, 20}, kPurple);
            cd::ui::draw_text(renderer, x + 4, 97,
                effect_group_name(group, ru(session)), selected_group ? kInk : kDim);
        }
        const int count = effect_group_size(state.picker_group);
        const int rows = state.picker_group == 0 ? 8 : 5;
        for (int item = 0; item < count; ++item) {
            const int column = item / rows;
            const int row = item % rows;
            const int x = 92 + column * 164;
            const int y = 124 + row * 25;
            const bool selected = item == state.picker_item;
            if (selected) fill(renderer, {x, y - 4, 154, 20}, {73, 46, 104, 255});
            cd::ui::draw_text(renderer, x + 7, y,
                effect_name(effect_at(state.picker_group, item), ru(session)),
                selected ? kInk : kDim);
        }
        cd::ui::draw_text(renderer, 92, 334,
            handheld()
                ? (ru(session) ? "LT/RT ГРУППА  UP/DN ЭФФЕКТ  A OK  B НАЗАД"
                               : "LT/RT GROUP  UP/DN EFFECT  A OK  B BACK")
                : (ru(session) ? "LT/RT ГРУППА  UP/DN ЭФФЕКТ  E OK  ESC НАЗАД"
                               : "LT/RT GROUP  UP/DN EFFECT  E OK  ESC BACK"), kDim);
    }
}


std::string_view plaits_model_name(cd::PlaitsModel model) noexcept {
    constexpr std::array<std::string_view, 16> names{
        "VA FILTER", "PHASE DIST", "WAVE TERRAIN", "STRING MACHINE",
        "CHIPTUNE", "VIRTUAL ANALOG", "WAVESHAPER", "FM",
        "GRAIN", "ADDITIVE", "WAVETABLE", "CHORD",
        "SWARM", "NOISE", "STRING", "MODAL"};
    return names[static_cast<std::size_t>(model)];
}

std::string_view output_mode_name(cd::PlaitsOutputMode mode) noexcept {
    constexpr std::array<std::string_view, 4> names{"MAIN", "AUX", "MIX", "STEREO"};
    return names[static_cast<std::size_t>(mode)];
}

std::string_view mod_wave_name(cd::ModWave wave) noexcept {
    constexpr std::array<std::string_view, 4> names{"SINE", "TRIANGLE", "S&H", "RANDOM WALK"};
    return names[static_cast<std::size_t>(wave)];
}

std::string_view mod_destination_name(cd::ModDestination destination) noexcept {
    constexpr std::array<std::string_view, 11> names{
        "PITCH", "TIMBRE", "COLOR", "MOTION", "TEXTURE", "LEVEL", "PAN",
        "FX1", "FX2", "FX3", "FX4"};
    return names[static_cast<std::size_t>(destination)];
}

int lab_field_count(int tab) noexcept {
    constexpr std::array<int, 5> counts{5, 7, 6, 3, 1};
    return counts[static_cast<std::size_t>(std::clamp(tab, 0, 4))];
}

void navigate_lab(UiState& state, int horizontal, int vertical) noexcept {
    if (horizontal != 0) {
        state.lab_tab = (state.lab_tab + horizontal + 5) % 5;
        state.lab_field = std::min(state.lab_field, lab_field_count(state.lab_tab) - 1);
    }
    if (vertical != 0) {
        const int count = lab_field_count(state.lab_tab);
        state.lab_field = (state.lab_field + vertical + count) % count;
    }
}

int current_scale_index(const cd::ScalaTuning& tuning) noexcept {
    for (std::size_t index = 0; index < g_scales.size(); ++index) {
        if (g_scales[index].name == tuning.name.data()) return static_cast<int>(index);
    }
    return 0;
}

void adjust_lab(cd::Session& session, UiState& state, int direction) {
    auto& slot = session.slots[static_cast<std::size_t>(state.slot)];
    if (state.lab_tab == 0) {
        if (state.lab_field == 0) {
            slot.engine = cd::EngineKind::plaits;
        } else if (state.lab_field == 1) {
            const int count = 16;
            slot.plaits_model = static_cast<cd::PlaitsModel>(
                (static_cast<int>(slot.plaits_model) + direction + count) % count);
        } else if (state.lab_field == 2) {
            const int count = 4;
            slot.plaits_output = static_cast<cd::PlaitsOutputMode>(
                (static_cast<int>(slot.plaits_output) + direction + count) % count);
        } else if (state.lab_field == 3 && !g_scales.empty()) {
            int index = current_scale_index(slot.tuning);
            index = (index + direction + static_cast<int>(g_scales.size())) % static_cast<int>(g_scales.size());
            cd::apply_scale(slot.tuning, g_scales[static_cast<std::size_t>(index)]);
        } else if (state.lab_field == 4) {
            slot.tuning.root_midi = std::clamp(slot.tuning.root_midi + direction, 0, 127);
        }
    } else if (state.lab_tab == 1) {
        auto& mod = slot.modulators[static_cast<std::size_t>(state.lab_modulator)];
        if (state.lab_field == 0) mod.enabled = direction > 0;
        else if (state.lab_field == 1) mod.wave = static_cast<cd::ModWave>(
            (static_cast<int>(mod.wave) + direction + 4) % 4);
        else if (state.lab_field == 2) mod.destination = static_cast<cd::ModDestination>(
            (static_cast<int>(mod.destination) + direction + 11) % 11);
        else if (state.lab_field == 3) mod.rate_hz = std::clamp(
            mod.rate_hz * std::pow(2.0F, static_cast<float>(direction) / 12.0F), 0.001F, 40.0F);
        else if (state.lab_field == 4) mod.depth = std::clamp(mod.depth + direction * 0.02F, -1.0F, 1.0F);
        else if (state.lab_field == 5) {
            const int max_source = state.lab_modulator - 1;
            if (max_source < 0) mod.rate_mod_source = -1;
            else {
                const int count = max_source + 2;
                mod.rate_mod_source = ((mod.rate_mod_source + 1 + direction + count) % count) - 1;
            }
        } else if (state.lab_field == 6) {
            mod.rate_mod_amount = std::clamp(mod.rate_mod_amount + direction * 0.02F, -1.0F, 1.0F);
        }
    } else if (state.lab_tab == 2) {
        auto& sequence = slot.euclidean;
        if (state.lab_field == 0) sequence.enabled = direction > 0;
        else if (state.lab_field == 1) {
            sequence.steps = std::clamp(sequence.steps + direction, 1, 32);
            sequence.pulses = std::min(sequence.pulses, sequence.steps);
            sequence.rotation = std::min(sequence.rotation, sequence.steps - 1);
        } else if (state.lab_field == 2) sequence.pulses = std::clamp(sequence.pulses + direction, 0, sequence.steps);
        else if (state.lab_field == 3) sequence.rotation =
            (sequence.rotation + direction + sequence.steps) % sequence.steps;
        else if (state.lab_field == 4) sequence.probability =
            std::clamp(sequence.probability + direction * 0.02F, 0.0F, 1.0F);
        else if (state.lab_field == 5) {
            slot.effects[3].kind = direction > 0 ? cd::EffectKind::granular_reverse : cd::EffectKind::bypass;
            if (direction > 0 && slot.effects[3].amount < 0.01F) {
                slot.effects[3] = {cd::EffectKind::granular_reverse, 0.42F, 0.52F, 0.38F};
            }
        }
    } else if (state.lab_tab == 3) {
        if (state.lab_field == 0) {
            constexpr int count = 10;
            session.performance.morph_target = static_cast<cd::SceneKind>(
                (static_cast<int>(session.performance.morph_target) + direction + count) % count);
        } else if (state.lab_field == 1) {
            session.performance.morph = std::clamp(session.performance.morph + direction * 0.02F, 0.0F, 1.0F);
        }
    }
    session.scene_modified = true;
}

void activate_lab(cd::Session& session, UiState& state) {
    auto& slot = session.slots[static_cast<std::size_t>(state.slot)];
    if (state.lab_tab == 0 && state.lab_field == 0) {
        slot.engine = cd::EngineKind::plaits;
    } else if (state.lab_tab == 1 && state.lab_field == 0) {
        auto& mod = slot.modulators[static_cast<std::size_t>(state.lab_modulator)];
        mod.enabled = !mod.enabled;
    } else if (state.lab_tab == 2 && state.lab_field == 0) {
        slot.euclidean.enabled = !slot.euclidean.enabled;
    } else if (state.lab_tab == 2 && state.lab_field == 5) {
        const bool active = slot.effects[3].kind == cd::EffectKind::granular_reverse;
        slot.effects[3] = active
            ? cd::EffectSettings{}
            : cd::EffectSettings{cd::EffectKind::granular_reverse, 0.42F, 0.52F, 0.38F};
    } else if (state.lab_tab == 3 && state.lab_field == 2) {
        cd::apply_scene_recipe(session, session.performance.morph_target);
        session.performance.morph = 0.0F;
    } else if (state.lab_tab == 4) {
        state.lab_open = false;
        state.stage_mode = true;
    }
    session.scene_modified = true;
}

std::string lab_value(const cd::Session& session, const UiState& state, int field) {
    const auto& slot = session.slots[static_cast<std::size_t>(state.slot)];
    char value[48]{};
    if (state.lab_tab == 0) {
        if (field == 0) return slot.engine == cd::EngineKind::plaits ? "PLAITs ACTIVE" : "A: ACTIVATE";
        if (field == 1) return std::string{plaits_model_name(slot.plaits_model)};
        if (field == 2) return std::string{output_mode_name(slot.plaits_output)};
        if (field == 3) return slot.tuning.name.data();
        std::snprintf(value, sizeof(value), "MIDI %d", slot.tuning.root_midi);
    } else if (state.lab_tab == 1) {
        const auto& mod = slot.modulators[static_cast<std::size_t>(state.lab_modulator)];
        if (field == 0) return mod.enabled ? "ON" : "OFF";
        if (field == 1) return std::string{mod_wave_name(mod.wave)};
        if (field == 2) return std::string{mod_destination_name(mod.destination)};
        if (field == 3) std::snprintf(value, sizeof(value), "%.3f HZ", static_cast<double>(mod.rate_hz));
        else if (field == 4) std::snprintf(value, sizeof(value), "%+.0f%%", static_cast<double>(mod.depth * 100.0F));
        else if (field == 5) {
            if (mod.rate_mod_source < 0) return "NONE";
            return "MOD " + std::to_string(mod.rate_mod_source + 1);
        } else std::snprintf(value, sizeof(value), "%+.0f%%", static_cast<double>(mod.rate_mod_amount * 100.0F));
    } else if (state.lab_tab == 2) {
        const auto& sequence = slot.euclidean;
        if (field == 0) return sequence.enabled ? "ON" : "OFF";
        if (field == 1) std::snprintf(value, sizeof(value), "%d", sequence.steps);
        else if (field == 2) std::snprintf(value, sizeof(value), "%d", sequence.pulses);
        else if (field == 3) std::snprintf(value, sizeof(value), "%d", sequence.rotation);
        else if (field == 4) std::snprintf(value, sizeof(value), "%.0f%%", static_cast<double>(sequence.probability * 100.0F));
        else return slot.effects[3].kind == cd::EffectKind::granular_reverse ? "FX4 ACTIVE" : "A: INSTALL FX4";
    } else if (state.lab_tab == 3) {
        if (field == 0) return std::string{scene_name(session.performance.morph_target, ru(session))};
        if (field == 1) std::snprintf(value, sizeof(value), "%.0f%%", static_cast<double>(session.performance.morph * 100.0F));
        else return "A: COMMIT TARGET";
    } else {
        return "A: ENTER STAGE MODE";
    }
    return value;
}

void draw_lab(SDL_Renderer* renderer, const cd::Session& session, const UiState& state) {
    fill(renderer, {8, 46, 496, 324}, {8, 7, 12, 250});
    outline(renderer, {8, 46, 496, 324}, kInk);
    constexpr std::array<std::string_view, 5> tabs{"ACTOR", "MOD", "EVENT", "MORPH", "STAGE"};
    for (int tab = 0; tab < 5; ++tab) {
        const int x = 18 + tab * 96;
        if (tab == state.lab_tab) fill(renderer, {x, 58, 88, 20}, kPurple);
        cd::ui::draw_text(renderer, x + 5, 64, tabs[static_cast<std::size_t>(tab)],
            tab == state.lab_tab ? kInk : kDim);
    }
    cd::ui::draw_text(renderer, 20, 88,
        std::string{"ACTOR "} + std::to_string(state.slot + 1), kFxColors[static_cast<std::size_t>(state.slot)]);
    if (state.lab_tab == 1) {
        cd::ui::draw_text(renderer, 340, 88,
            std::string{"MOD "} + std::to_string(state.lab_modulator + 1) + "  X: NEXT", kInk);
    }
    constexpr std::array<std::array<std::string_view, 7>, 5> labels{{
        {"ENGINE", "MODEL", "OUTPUT", "SCALA", "ROOT", "", ""},
        {"ENABLED", "WAVE", "DEST", "RATE", "AMOUNT", "RATE SOURCE", "RATE AMOUNT"},
        {"ENABLED", "STEPS", "PULSES", "ROTATION", "PROBABILITY", "REVERSE GRAINS", ""},
        {"TARGET", "MORPH", "COMMIT", "", "", "", ""},
        {"PERFORMANCE VIEW", "", "", "", "", "", ""},
    }};
    const int count = lab_field_count(state.lab_tab);
    for (int field = 0; field < count; ++field) {
        const int y = 116 + field * 31;
        const bool selected = field == state.lab_field;
        if (selected) fill(renderer, {18, y - 5, 476, 25}, {73, 46, 104, 255});
        cd::ui::draw_text(renderer, 28, y,
            labels[static_cast<std::size_t>(state.lab_tab)][static_cast<std::size_t>(field)],
            selected ? kInk : kDim);
        const std::string value = lab_value(session, state, field);
        cd::ui::draw_text(renderer, 486 - cd::ui::text_width(value), y, value,
            selected ? kInk : kDim);
    }
    cd::ui::draw_text(renderer, 18, 346,
        "DPAD TAB/FIELD  L/R VALUE  A ACTION  Y ACTOR  B CLOSE", kDim);
}

void draw_stage(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state) {
    SDL_SetRenderDrawColor(renderer, 8, 7, 12, 255);
    SDL_RenderClear(renderer);
    cd::ui::draw_text(renderer, 16, 12, "CURSED DRONE / STAGE", kInk, 2);
    const std::string scene = std::string{scene_name(session.scene, ru(session))} + "  >  " +
        std::string{scene_name(session.performance.morph_target, ru(session))};
    cd::ui::draw_text(renderer, 18, 44, scene, kDim);
    for (int field = 0; field < 6; ++field) {
        const int y = 74 + field * 34;
        const bool selected = field == state.stage_field;
        if (selected) fill(renderer, {14, y - 5, 484, 28}, {73, 46, 104, 255});
        const std::string label = field < 5
            ? std::string{macro_name(field, ru(session))}
            : std::string{"MORPH"};
        const float value = field < 5
            ? macro_value(session.performance, field)
            : session.performance.morph;
        cd::ui::draw_text(renderer, 22, y, label, selected ? kInk : kDim);
        bar(renderer, 142, y, 344, 11, value,
            selected ? kInk : kFxColors[static_cast<std::size_t>(field % 4)]);
    }
    for (int actor = 0; actor < 4; ++actor) {
        const int x = 14 + actor * 122;
        const bool selected = actor == state.slot;
        if (selected) outline(renderer, {x, 282, 114, 54}, kInk);
        const auto& slot = session.slots[static_cast<std::size_t>(actor)];
        cd::ui::draw_text(renderer, x + 6, 290,
            std::to_string(actor + 1) + " " + std::string{engine_name(slot.engine, ru(session))},
            selected ? kInk : kDim);
        bar(renderer, x + 6, 311, 102, 10,
            std::clamp(telemetry.slot_rms[static_cast<std::size_t>(actor)] * 4.2F, 0.0F, 1.0F),
            kFxColors[static_cast<std::size_t>(actor)]);
        if (!slot.enabled) cd::ui::draw_text(renderer, x + 6, 326, "MUTE", kFxColors[0]);
    }
    cd::ui::draw_text(renderer, 14, 356,
        "UP/DN SELECT  L/R MOVE  Y ACTOR  A MUTE  SELECT FADE  B EXIT", kDim);
    SDL_RenderPresent(renderer);
}

void adjust_stage(cd::Session& session, UiState& state, int direction) {
    if (state.stage_field < 5) {
        float* value = macro_value(session.performance, state.stage_field);
        *value = std::clamp(*value + direction * 0.02F, 0.0F, 1.0F);
    } else {
        session.performance.morph = std::clamp(session.performance.morph + direction * 0.02F, 0.0F, 1.0F);
    }
}

void draw(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state,
    Uint32 now) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (state.stage_mode) {
        draw_stage(renderer, session, telemetry, state);
        return;
    }
    SDL_SetRenderDrawColor(renderer, 8, 7, 12, 255);
    SDL_RenderClear(renderer);
    draw_header(renderer, session, telemetry, state);
    if (state.page == Page::perform) {
        draw_scene(renderer, session, telemetry, state);
    } else if (state.page == Page::slot) {
        draw_tracks(renderer, session, telemetry, state);
    } else if (state.page == Page::effects) {
        draw_effects(renderer, session, telemetry, state);
    } else if (state.page == Page::master) {
        draw_master(renderer, session, telemetry, state);
    } else {
        draw_setup(renderer, session, state);
    }
    if (state.picker != Picker::none) {
        draw_picker(renderer, session, state);
    }
    if (state.lab_open) {
        draw_lab(renderer, session, state);
    }
    std::string help;
    if (state.lab_open) {
        help.clear();
    } else if (handheld()) {
        if (state.picker != Picker::none) {
            help = ru(session) ? "A ВЫБРАТЬ  B НАЗАД" : "A SELECT  B BACK";
        } else if (state.page == Page::effects) {
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
        }
    } else if (state.picker != Picker::none) {
        help = ru(session) ? "E ВЫБРАТЬ  ESC НАЗАД" : "E SELECT  ESC BACK";
    } else if (state.page == Page::effects) {
        help = ru(session) ? "LT/RT FX  UP/DN ПАРАМ.  S ДОРОЖКА  A/D ЗНАЧ.  E ТИП"
                           : "LT/RT FX  UP/DN PARAM  S TRACK  A/D VALUE  E TYPE";
    } else if (state.page == Page::perform) {
        help = ru(session) ? "UP/DN РУЧКА/УРОВ.  LT/RT ДОРОЖКА  SPACE MUTE  E ЛАНДШАФТ"
                           : "UP/DN MACRO/LEVEL  LT/RT TRACK  SPACE MUTE  E LANDSCAPE";
    } else if (state.page == Page::master || state.page == Page::setup) {
        help = ru(session) ? "UP/DN ПАРАМ.  A/D ЗНАЧ.  TAB ЭКРАН  F ФЕЙД  K KILL"
                           : "UP/DN PARAM  A/D VALUE  TAB PAGE  F FADE  K KILL";
    } else {
        help = ru(session) ? "LT/RT СЛОТ  UP/DN ПАРАМ.  A/D ЗНАЧ.  E ВЫБОР  SPACE MUTE"
                           : "LT/RT SLOT  UP/DN PARAM  A/D VALUE  E CHOOSE  SPACE MUTE";
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

void activate_current(cd::Session& session, UiState& state) {
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
    if (state.lab_open) {
        state.lab_open = false;
    } else if (state.stage_mode) {
        state.stage_mode = false;
    } else if (state.picker != Picker::none) {
        state.picker = Picker::none;
    } else if (state.page == Page::perform && state.scene_track_focus) {
        state.scene_track_focus = false;
    } else if (state.page != Page::perform) {
        state.page = Page::perform;
    }
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

void navigate_horizontal(UiState& state, const cd::Session& session, int direction) noexcept {
    if (state.page == Page::effects) {
        parameter(state) = (parameter(state) + direction + 4) % 4;
        const auto kind = session.slots[static_cast<std::size_t>(state.slot)]
            .effects[static_cast<std::size_t>(parameter(state))].kind;
        state.effect_field = std::min(state.effect_field, std::max(0, effect_field_count(kind) - 1));
    } else if (state.page == Page::perform || state.page == Page::slot) {
        state.slot = (state.slot + direction + 4) % 4;
    }
}

void navigate_vertical(UiState& state, const cd::Session& session, int direction) noexcept {
    if (state.page == Page::effects) {
        const auto kind = session.slots[static_cast<std::size_t>(state.slot)]
            .effects[static_cast<std::size_t>(parameter(state))].kind;
        const int count = effect_field_count(kind);
        if (count > 0) state.effect_field = (state.effect_field + direction + count) % count;
    } else if (state.page == Page::perform) {
        if (state.scene_track_focus) {
            if (direction < 0) {
                state.scene_track_focus = false;
                parameter(state) = 4;
            }
        } else if (direction > 0 && parameter(state) == 4) {
            state.scene_track_focus = true;
        } else {
            parameter(state) = std::clamp(parameter(state) + direction, 0, 4);
        }
    } else {
        const int count = parameter_count(state.page);
        parameter(state) = (parameter(state) + direction + count) % count;
    }
}

void open_context_picker(UiState& state, const cd::Session& session) noexcept {
    if (state.page == Page::perform) {
        open_scene_picker(state, session);
    } else if (state.page == Page::slot) {
        open_engine_picker(state, session);
    } else if (state.page == Page::effects) {
        open_effect_picker(state, session);
    }
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
    if (!show_startup_splash(renderer)) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    cd::Session session = cd::make_default_session();
    std::filesystem::path autosave_path;
    std::filesystem::path legacy_autosave_path;
    if (const char* data_directory = std::getenv("CURSED_DRONE_DATA_DIR");
        data_directory != nullptr && data_directory[0] != '\0') {
        const std::filesystem::path root{data_directory};
        autosave_path = root / "autosave.cdrone";
        legacy_autosave_path = root / "myldy20" / "cursed-drone" / "autosave.cdrone";
        g_scales = cd::load_scala_scales({root / "scales", std::filesystem::path{"assets/scales"}});
    } else if (char* preference_path = SDL_GetPrefPath("myldy20", "cursed-drone")) {
        autosave_path = std::filesystem::path{preference_path} / "autosave.cdrone";
        g_scales = cd::load_scala_scales({std::filesystem::path{preference_path} / "scales",
            std::filesystem::path{"assets/scales"}});
        SDL_free(preference_path);
    }
    if (g_scales.empty()) g_scales.push_back(cd::equal_temperament_scale());

    const std::filesystem::path load_path = std::filesystem::exists(autosave_path)
        ? autosave_path : legacy_autosave_path;
    if (!load_path.empty() && std::filesystem::exists(load_path)) {
        std::string load_error;
        if (!cd::load_session(load_path, session, load_error)) {
            std::fprintf(stderr, "autosave load: %s\n", load_error.c_str());
        } else if (load_path == legacy_autosave_path) {
            std::fprintf(stderr, "autosave migration: loaded legacy path %s\n",
                load_path.string().c_str());
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
                if (state.picker != Picker::none) {
                    switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: state.picker = Picker::none; break;
                    case SDLK_LEFT: move_picker(state, -1, 0); break;
                    case SDLK_RIGHT: move_picker(state, 1, 0); break;
                    case SDLK_UP: move_picker(state, 0, -1); break;
                    case SDLK_DOWN: move_picker(state, 0, 1); break;
                    case SDLK_RETURN:
                    case SDLK_SPACE: confirm_picker(state, session); changed = true; break;
                    case SDLK_e: confirm_picker(state, session); changed = true; break;
                    default: break;
                    }
                } else {
                    switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_LEFT: navigate_horizontal(state, session, -1); break;
                    case SDLK_RIGHT: navigate_horizontal(state, session, 1); break;
                    case SDLK_UP: navigate_vertical(state, session, -1); break;
                    case SDLK_DOWN: navigate_vertical(state, session, 1); break;
                    case SDLK_a:
                        if (state.page == Page::slot && parameter(state) == 0) open_engine_picker(state, session);
                        else { start_adjust(session, state, -1, now); changed = true; }
                        break;
                    case SDLK_d:
                        if (state.page == Page::slot && parameter(state) == 0) open_engine_picker(state, session);
                        else { start_adjust(session, state, 1, now); changed = true; }
                        break;
                    case SDLK_TAB:
                        state.page = static_cast<Page>((page_index(state.page) + 1) % 5);
                        break;
                    case SDLK_1: state.page = Page::perform; break;
                    case SDLK_2: state.page = Page::slot; break;
                    case SDLK_3: state.page = Page::effects; break;
                    case SDLK_4: state.page = Page::master; break;
                    case SDLK_5: state.page = Page::setup; break;
                    case SDLK_s:
                        if (state.page == Page::effects) state.slot = (state.slot + 1) % 4;
                        break;
                    case SDLK_e:
                        open_context_picker(state, session);
                        break;
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
                }
            } else if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_a) stop_adjust(state, -1);
                if (event.key.keysym.sym == SDLK_d) stop_adjust(state, 1);
            } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
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
                } else if (state.lab_open) {
                    switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: navigate_lab(state, -1, 0); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: navigate_lab(state, 1, 0); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP: navigate_lab(state, 0, -1); break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: navigate_lab(state, 0, 1); break;
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: adjust_lab(session, state, -1); changed = true; break;
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: adjust_lab(session, state, 1); changed = true; break;
                    case SDL_CONTROLLER_BUTTON_B: activate_lab(session, state); changed = true; break;
                    case SDL_CONTROLLER_BUTTON_A: state.lab_open = false; break;
                    case SDL_CONTROLLER_BUTTON_X:
                        if (state.lab_tab == 1) state.lab_modulator = (state.lab_modulator + 1) % 4;
                        break;
                    case SDL_CONTROLLER_BUTTON_Y: state.slot = (state.slot + 1) % 4; break;
                    default: break;
                    }
                } else if (state.stage_mode) {
                    switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_DPAD_UP: state.stage_field = (state.stage_field + 5) % 6; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: state.stage_field = (state.stage_field + 1) % 6; break;
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: adjust_stage(session, state, -1); changed = true; break;
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: adjust_stage(session, state, 1); changed = true; break;
                    case SDL_CONTROLLER_BUTTON_B:
                        session.slots[static_cast<std::size_t>(state.slot)].enabled =
                            !session.slots[static_cast<std::size_t>(state.slot)].enabled;
                        changed = true;
                        break;
                    case SDL_CONTROLLER_BUTTON_A: state.stage_mode = false; break;
                    case SDL_CONTROLLER_BUTTON_Y: state.slot = (state.slot + 1) % 4; break;
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
                        state.lab_open = true;
                        state.lab_field = 0;
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
            }
        }
        const Uint32 now = SDL_GetTicks();
        if (state.back_held && !state.back_long_action && now - state.back_held_since >= 1'100U) {
            audio.graph.panic();
            state.kill_flash_until = now + 700U;
            state.back_long_action = true;
        }
        changed = repeat_adjust(session, state, now) || changed;
        changed = update_fade(session, state, static_cast<float>(now - previous_frame) * 0.001F) || changed;
        previous_frame = now;
        if (now - state.cpu_display_updated_at >= 500U) {
            state.displayed_cpu_percent = static_cast<int>(std::lround(
                audio.cpu_load.load(std::memory_order_relaxed) * 100.0F));
            state.cpu_display_updated_at = now;
        }
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
        update_title(window, session, state);
        draw(renderer, session, audio.graph.telemetry(), state, now);
        SDL_Delay(16);
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

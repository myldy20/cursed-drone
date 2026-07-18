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
constexpr float kMinimumAudibleFrequency = 20.0F;
constexpr SDL_Color kInk{238, 226, 197, 255};
constexpr SDL_Color kDim{151, 143, 158, 255};
constexpr SDL_Color kPurple{117, 67, 171, 255};
constexpr SDL_Color kPanel{35, 30, 46, 255};
constexpr std::array<SDL_Color, 4> kFxColors{{
    {216, 88, 88, 255}, {224, 154, 63, 255}, {80, 169, 154, 255}, {91, 122, 187, 255},
}};

enum class Page { perform, slot, effects, master, memory };
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
    int focus_zone{1};
    int effect_field{0};
    int actor_advanced_field{0};
    int actor_modulator{0};
    int master_effect{0};
    int master_effect_field{0};
    int memory_slot{0};
    int memory_action{0};
    int memory_setting{0};
    Picker picker{Picker::none};
    bool picker_master{false};
    int picker_group{0};
    int picker_item{0};
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
    bool help_open{false};
    bool menu_open{false};
    int menu_item{0};
    bool request_exit{false};
    std::string toast{};
    Uint32 toast_until{0};
};

std::vector<cd::ParsedScale> g_scales{};
std::filesystem::path g_data_root{};
std::array<std::filesystem::path, cd::kMemorySlots> g_memory_paths{};

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
    case Page::perform: return russian ? "МЕСТО" : "PLACE";
    case Page::slot: return russian ? "АКТЕР" : "ACTOR";
    case Page::effects: return "FX";
    case Page::master: return russian ? "МАСТЕР" : "MASTER";
    case Page::memory: return russian ? "ПАМЯТЬ" : "MEMORY";
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
    case cd::EngineKind::plaits: return russian ? "МУЗЫКАЛЬНЫЙ" : "MUSICAL";
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

int effect_catalog_index(cd::EffectKind kind) noexcept {
    for (int item = 0; item < static_cast<int>(kBasicEffects.size()); ++item) {
        if (kBasicEffects[static_cast<std::size_t>(item)] == kind) return item;
    }
    for (int item = 0; item < static_cast<int>(kCompoundEffects.size()); ++item) {
        if (kCompoundEffects[static_cast<std::size_t>(item)] == kind)
            return static_cast<int>(kBasicEffects.size()) + item;
    }
    return 0;
}

cd::EffectKind effect_from_catalog_index(int index) noexcept {
    const int total = static_cast<int>(kBasicEffects.size() + kCompoundEffects.size());
    index = (index % total + total) % total;
    if (index < static_cast<int>(kBasicEffects.size()))
        return kBasicEffects[static_cast<std::size_t>(index)];
    return kCompoundEffects[static_cast<std::size_t>(index - static_cast<int>(kBasicEffects.size()))];
}

void cycle_effect_kind(cd::EffectSettings& effect, int direction) noexcept {
    const int index = effect_catalog_index(effect.kind) + direction;
    effect.kind = effect_from_catalog_index(index);
    if (effect.kind == cd::EffectKind::bypass) {
        effect.amount = 0.0F;
        effect.tone = 0.50F;
        effect.feedback = 0.0F;
    } else if (effect.amount <= 0.001F) {
        effect.amount = 0.20F;
        effect.tone = 0.50F;
        effect.feedback = 0.20F;
    }
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

void open_effect_picker(UiState& state, const cd::Session& session, bool master = false) noexcept {
    state.picker_master = master;
    const auto current = master
        ? session.master_effects[static_cast<std::size_t>(state.master_effect)].kind
        : session.slots[static_cast<std::size_t>(state.slot)].effects[static_cast<std::size_t>(parameter(state))].kind;
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
        constexpr int rows = 5;
        constexpr int columns = 2;
        int row = state.picker_item % rows;
        int column = state.picker_item / rows;
        row = (row + vertical + rows) % rows;
        column = (column + horizontal + columns) % columns;
        state.picker_item = column * rows + row;
        return;
    }
    if (state.picker == Picker::engine) {
        const int groups = static_cast<int>(kEngineGroups.size());
        state.picker_group = (state.picker_group + horizontal + groups) % groups;
        state.picker_item = (state.picker_item + vertical + 4) % 4;
        return;
    }
    if (state.picker == Picker::effect) {
        int count = effect_group_size(state.picker_group);
        const int rows = state.picker_group == 0 ? 8 : 5;
        int row = state.picker_item % rows;
        int column = state.picker_item / rows;
        if (vertical != 0) {
            for (int attempt = 0; attempt < rows; ++attempt) {
                row = (row + vertical + rows) % rows;
                const int candidate = column * rows + row;
                if (candidate < count) { state.picker_item = candidate; break; }
            }
        }
        if (horizontal != 0) {
            const int candidate = (column + horizontal) * rows + row;
            if (candidate >= 0 && candidate < count) {
                state.picker_item = candidate;
            } else {
                state.picker_group = (state.picker_group + horizontal + 2) % 2;
                count = effect_group_size(state.picker_group);
                const int target_rows = state.picker_group == 0 ? 8 : 5;
                state.picker_item = std::min(row, count - 1);
                if (state.picker_item >= target_rows) state.picker_item = target_rows - 1;
            }
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
        if (state.picker_master) {
            session.master_effects[static_cast<std::size_t>(state.master_effect)].kind =
                effect_at(state.picker_group, state.picker_item);
            state.master_effect_field = 0;
        } else {
            session.slots[static_cast<std::size_t>(state.slot)]
                .effects[static_cast<std::size_t>(parameter(state))].kind =
                effect_at(state.picker_group, state.picker_item);
            state.effect_field = 0;
            session.scene_modified = true;
        }
    }
    state.picker = Picker::none;
    state.picker_master = false;
}

int page_index(Page page) noexcept { return static_cast<int>(page); }

int parameter_count(Page page) noexcept {
    switch (page) {
    case Page::perform: return 5;
    case Page::slot: return 10;
    case Page::effects: return 4;
    case Page::master: return 2;
    case Page::memory: return 3;
    }
    return 1;
}

int focus_zone_count(Page page) noexcept {
    switch (page) {
    case Page::perform: return 3; // landscape, macros, actors
    case Page::slot: return 3;    // actor selector, basic, advanced
    case Page::effects: return 5; // actor selector, FX1..FX4
    case Page::master: return 5;  // signal, master FX1..FX4
    case Page::memory: return 3;  // slots, actions, settings
    }
    return 1;
}

int& parameter(UiState& state) noexcept {
    if (state.page == Page::slot) return state.slot_selected[static_cast<std::size_t>(state.slot)];
    return state.selected[static_cast<std::size_t>(page_index(state.page))];
}

int parameter(const UiState& state) noexcept {
    if (state.page == Page::slot) return state.slot_selected[static_cast<std::size_t>(state.slot)];
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

float effect_value(const cd::EffectSettings& effect, int field) noexcept {
    return field == 0 ? effect.amount : (field == 1 ? effect.tone : effect.feedback);
}

float* effect_value(cd::EffectSettings& effect, int field) noexcept {
    return field == 0 ? &effect.amount : (field == 1 ? &effect.tone : &effect.feedback);
}

std::string note_name(int midi, bool russian) {
    constexpr std::array<std::string_view, 12> notes{
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    midi = std::clamp(midi, 0, 127);
    const int octave = midi / 12 - 1;
    std::string result{notes[static_cast<std::size_t>(midi % 12)]};
    result += std::to_string(octave);
    if (russian) result += "  (НОТА " + std::to_string(midi) + ")";
    return result;
}

std::string friendly_scale_name(const cd::ScalaTuning& tuning, bool russian) {
    const std::string name{tuning.name.data()};
    if (name.find("12") != std::string::npos) return russian ? "СТАНДАРТНЫЙ 12" : "STANDARD 12";
    if (name.find("19") != std::string::npos) return "19 EDO";
    if (name.find("minor") != std::string::npos || name.find("MINOR") != std::string::npos)
        return russian ? "ЧИСТЫЙ МИНОР" : "JUST MINOR";
    return name.empty() ? (russian ? "СТАНДАРТНЫЙ" : "STANDARD") : name;
}

std::string_view musical_model_name(cd::PlaitsModel model, bool russian) noexcept {
    constexpr std::array<std::string_view, 16> en{
        "FILTER TONE", "PHASE TONE", "WAVE TERRAIN", "STRING MACHINE",
        "CHIP TONE", "ANALOG", "WAVESHAPER", "FM",
        "GRAIN", "ADDITIVE", "WAVETABLE", "CHORD",
        "SWARM", "NOISE", "STRING", "MODAL BODY"};
    constexpr std::array<std::string_view, 16> ru_names{
        "ФИЛЬТР-ТОН", "ФАЗОВЫЙ ТОН", "ВОЛНОВОЙ РЕЛЬЕФ", "СТРУННАЯ МАШИНА",
        "ЧИП-ТОН", "АНАЛОГ", "ФОРМИРОВАТЕЛЬ", "FM",
        "ЗЕРНО", "АДДИТИВНЫЙ", "ВОЛНОВАЯ ТАБЛИЦА", "АККОРД",
        "РОЙ", "ШУМ", "СТРУНА", "МОДАЛЬНЫЙ КОРПУС"};
    return (russian ? ru_names : en)[static_cast<std::size_t>(model)];
}

std::string_view output_mode_name(cd::PlaitsOutputMode mode, bool russian) noexcept {
    constexpr std::array<std::string_view, 4> en{"MAIN", "AUX", "MIX", "STEREO"};
    constexpr std::array<std::string_view, 4> ru_names{"ОСНОВНОЙ", "ДОП.", "СМЕСЬ", "СТЕРЕО"};
    return (russian ? ru_names : en)[static_cast<std::size_t>(mode)];
}

std::string_view mod_wave_name(cd::ModWave wave, bool russian) noexcept {
    constexpr std::array<std::string_view, 4> en{"SINE", "TRIANGLE", "S&H", "RANDOM WALK"};
    constexpr std::array<std::string_view, 4> ru_names{"СИНУС", "ТРЕУГОЛЬНИК", "СЛУЧ. ШАГ", "БЛУЖДАНИЕ"};
    return (russian ? ru_names : en)[static_cast<std::size_t>(wave)];
}

std::string_view mod_destination_name(cd::ModDestination destination, bool russian) noexcept {
    constexpr std::array<std::string_view, 11> en{
        "PITCH", "TIMBRE", "BODY", "MOTION", "TEXTURE", "LEVEL", "PAN",
        "FX1", "FX2", "FX3", "FX4"};
    constexpr std::array<std::string_view, 11> ru_names{
        "ВЫСОТА", "ТЕМБР", "ТЕЛО", "ДВИЖЕНИЕ", "ТЕКСТУРА", "УРОВЕНЬ", "ПАНОРАМА",
        "FX1", "FX2", "FX3", "FX4"};
    return (russian ? ru_names : en)[static_cast<std::size_t>(destination)];
}

std::string_view actor_basic_label(int field, bool russian) noexcept {
    constexpr std::array<std::string_view, 10> en{
        "ACTIVE", "SOURCE", "ENGINE", "PITCH", "CHARACTER", "BODY", "MOTION", "TEXTURE", "LEVEL", "PAN"};
    constexpr std::array<std::string_view, 10> ru_names{
        "АКТИВЕН", "ИСТОЧНИК", "ДВИЖОК", "ВЫСОТА", "ХАРАКТЕР", "ТЕЛО", "ДВИЖЕНИЕ", "ТЕКСТУРА", "УРОВЕНЬ", "ПАНОРАМА"};
    return (russian ? ru_names : en)[static_cast<std::size_t>(field)];
}

std::string_view actor_advanced_label(int field, bool russian) noexcept {
    constexpr std::array<std::string_view, 17> en{
        "MODEL", "OUTPUT", "TUNING", "ROOT", "EVENTS", "STEPS", "PULSES", "ROTATE", "PROBABILITY",
        "MOD ROW", "MOD ACTIVE", "MOD SHAPE", "MOD TARGET", "MOD RATE", "MOD DEPTH", "RATE SOURCE", "CROSS AMOUNT"};
    constexpr std::array<std::string_view, 17> ru_names{
        "МОДЕЛЬ", "ВЫХОД", "СТРОЙ", "ОСНОВА", "СОБЫТИЯ", "ШАГИ", "ИМПУЛЬСЫ", "СДВИГ", "ВЕРОЯТНОСТЬ",
        "СТРОКА MOD", "MOD АКТИВЕН", "ФОРМА MOD", "ЦЕЛЬ MOD", "СКОРОСТЬ MOD", "ГЛУБИНА MOD", "ИСТОЧНИК СКОР.", "ГЛУБИНА СВЯЗИ"};
    return (russian ? ru_names : en)[static_cast<std::size_t>(field)];
}

std::string actor_basic_value(const cd::Session& session, const UiState& state, int field) {
    const auto& slot = session.slots[static_cast<std::size_t>(state.slot)];
    char value[40]{};
    if (field == 0) return slot.enabled ? (ru(session) ? "ДА" : "YES") : (ru(session) ? "НЕТ" : "NO");
    if (field == 1) return slot.engine == cd::EngineKind::plaits
        ? (ru(session) ? "МУЗЫКАЛЬНЫЙ" : "MUSICAL")
        : (ru(session) ? "ЛАНДШАФТ" : "LANDSCAPE");
    if (field == 2) return std::string{engine_name(slot.engine, ru(session))};
    if (field == 3) std::snprintf(value, sizeof(value), "%.1f HZ", static_cast<double>(slot.frequency_hz));
    else if (field == 9) std::snprintf(value, sizeof(value), "%+.0f%%", static_cast<double>(slot.pan * 100.0F));
    else {
        const float values[]{slot.timbre, slot.color, slot.motion, slot.texture, slot.level};
        std::snprintf(value, sizeof(value), "%.0f%%", static_cast<double>(values[field - 4] * 100.0F));
    }
    return value;
}

std::string actor_advanced_value(const cd::Session& session, const UiState& state, int field) {
    const auto& slot = session.slots[static_cast<std::size_t>(state.slot)];
    const auto& mod = slot.modulators[static_cast<std::size_t>(state.actor_modulator)];
    char value[48]{};
    if (field == 0) return std::string{musical_model_name(slot.plaits_model, ru(session))};
    if (field == 1) return std::string{output_mode_name(slot.plaits_output, ru(session))};
    if (field == 2) return friendly_scale_name(slot.tuning, ru(session));
    if (field == 3) return note_name(slot.tuning.root_midi, ru(session));
    if (field == 4) return slot.euclidean.enabled ? (ru(session) ? "ВКЛ" : "ON") : (ru(session) ? "ВЫКЛ" : "OFF");
    if (field == 5) return std::to_string(slot.euclidean.steps);
    if (field == 6) return std::to_string(slot.euclidean.pulses);
    if (field == 7) return std::to_string(slot.euclidean.rotation);
    if (field == 8) { std::snprintf(value, sizeof(value), "%.0f%%", static_cast<double>(slot.euclidean.probability * 100.0F)); return value; }
    if (field == 9) return std::to_string(state.actor_modulator + 1) + " / 4";
    if (field == 10) return mod.enabled ? (ru(session) ? "ВКЛ" : "ON") : (ru(session) ? "ВЫКЛ" : "OFF");
    if (field == 11) return std::string{mod_wave_name(mod.wave, ru(session))};
    if (field == 12) return std::string{mod_destination_name(mod.destination, ru(session))};
    if (field == 13) { std::snprintf(value, sizeof(value), "%.3f HZ", static_cast<double>(mod.rate_hz)); return value; }
    if (field == 14) { std::snprintf(value, sizeof(value), "%+.0f%%", static_cast<double>(mod.depth * 100.0F)); return value; }
    if (field == 15) return mod.rate_mod_source < 0 ? (ru(session) ? "НЕТ" : "NONE") : "MOD " + std::to_string(mod.rate_mod_source + 1);
    std::snprintf(value, sizeof(value), "%+.0f%%", static_cast<double>(mod.rate_mod_amount * 100.0F));
    return value;
}

void restore_landscape_actor(cd::Session& session, int actor) {
    cd::Session recipe = session;
    cd::apply_scene_recipe(recipe, session.scene);
    session.slots[static_cast<std::size_t>(actor)] = recipe.slots[static_cast<std::size_t>(actor)];
    session.scene_modified = true;
}

void toggle_actor_source(cd::Session& session, int actor) {
    auto& slot = session.slots[static_cast<std::size_t>(actor)];
    if (slot.engine == cd::EngineKind::plaits) {
        restore_landscape_actor(session, actor);
    } else {
        slot.engine = cd::EngineKind::plaits;
        slot.enabled = true;
        slot.frequency_hz = std::clamp(slot.frequency_hz, 24.0F, 880.0F);
        slot.plaits_output = cd::PlaitsOutputMode::stereo;
        session.scene_modified = true;
    }
}

std::string value_text(const cd::Session& session, const UiState& state, int = -1) {
    if (state.page == Page::perform) {
        if (state.focus_zone == 0) return std::string{scene_name(session.scene, ru(session))};
        if (state.focus_zone == 1) {
            char value[16]{};
            std::snprintf(value, sizeof(value), "%.0f%%", static_cast<double>(macro_value(session.performance, parameter(state)) * 100.0F));
            return value;
        }
        return session.slots[static_cast<std::size_t>(state.slot)].enabled
            ? (ru(session) ? "ЗВУЧИТ" : "ACTIVE") : (ru(session) ? "ЗАГЛУШЕН" : "MUTED");
    }
    if (state.page == Page::slot) {
        if (state.focus_zone == 0) return std::to_string(state.slot + 1);
        return state.focus_zone == 1
            ? actor_basic_value(session, state, parameter(state))
            : actor_advanced_value(session, state, state.actor_advanced_field);
    }
    if (state.page == Page::effects) {
        if (state.focus_zone == 0) return std::to_string(state.slot + 1);
        const int effect_index = std::clamp(state.focus_zone - 1, 0, 3);
        const auto& effect = session.slots[static_cast<std::size_t>(state.slot)].effects[static_cast<std::size_t>(effect_index)];
        if (state.effect_field == 0) return std::string{effect_name(effect.kind, ru(session))};
        return std::to_string(static_cast<int>(std::lround(effect_value(effect, state.effect_field - 1) * 100.0F))) + "%";
    }
    if (state.page == Page::master) {
        if (state.focus_zone == 0) {
            if (parameter(state) == 0) return std::to_string(static_cast<int>(std::lround(session.master_level * 100.0F))) + "%";
            return std::to_string(static_cast<int>(std::lround(session.tempo_bpm))) + " BPM";
        }
        const int effect_index = std::clamp(state.focus_zone - 1, 0, 3);
        const auto& effect = session.master_effects[static_cast<std::size_t>(effect_index)];
        if (state.master_effect_field == 0) return std::string{effect_name(effect.kind, ru(session))};
        return std::to_string(static_cast<int>(std::lround(effect_value(effect, state.master_effect_field - 1) * 100.0F))) + "%";
    }
    return std::to_string(state.memory_slot + 1);
}

std::string current_label(const cd::Session& session, const UiState& state) {
    if (state.page == Page::perform) {
        if (state.focus_zone == 0) return ru(session) ? "ЛАНДШАФТ" : "LANDSCAPE";
        if (state.focus_zone == 1) return std::string{macro_name(parameter(state), ru(session))};
        return std::string{ru(session) ? "АКТЕР " : "ACTOR "} + std::to_string(state.slot + 1);
    }
    if (state.page == Page::slot) {
        if (state.focus_zone == 0) return ru(session) ? "ВЫБОР АКТЕРА" : "ACTOR SELECT";
        return state.focus_zone == 1
            ? std::string{actor_basic_label(parameter(state), ru(session))}
            : std::string{actor_advanced_label(state.actor_advanced_field, ru(session))};
    }
    if (state.page == Page::effects) {
        if (state.focus_zone == 0) return ru(session) ? "ВЫБОР АКТЕРА" : "ACTOR SELECT";
        const int effect_index = std::clamp(state.focus_zone - 1, 0, 3);
        const auto kind = session.slots[static_cast<std::size_t>(state.slot)].effects[static_cast<std::size_t>(effect_index)].kind;
        return state.effect_field == 0
            ? "FX " + std::to_string(effect_index + 1) + " " + std::string{ru(session) ? "ТИП" : "TYPE"}
            : std::string{effect_field(kind, state.effect_field - 1, ru(session))};
    }
    if (state.page == Page::master) {
        if (state.focus_zone == 0)
            return parameter(state) == 0 ? (ru(session) ? "ГРОМКОСТЬ" : "LEVEL") : (ru(session) ? "ТЕМП" : "TEMPO");
        const int effect_index = std::clamp(state.focus_zone - 1, 0, 3);
        const auto kind = session.master_effects[static_cast<std::size_t>(effect_index)].kind;
        return state.master_effect_field == 0
            ? "MASTER FX " + std::to_string(effect_index + 1) + " " + std::string{ru(session) ? "ТИП" : "TYPE"}
            : std::string{effect_field(kind, state.master_effect_field - 1, ru(session))};
    }
    return ru(session) ? "СЛОТ ПАМЯТИ" : "MEMORY SLOT";
}

bool current_adjustable(const cd::Session& session, const UiState& state) {
    if (state.page == Page::perform) return state.focus_zone == 1 || state.focus_zone == 2;
    if (state.page == Page::slot) {
        if (state.focus_zone == 1) return parameter(state) >= 3;
        if (state.focus_zone == 2) {
            switch (state.actor_advanced_field) {
            case 3: case 5: case 6: case 7: case 8: case 13: case 14: case 16: return true;
            default: return false;
            }
        }
        return false;
    }
    if (state.page == Page::effects && state.focus_zone > 0) {
        const int effect_index = state.focus_zone - 1;
        return state.effect_field > 0 && state.effect_field <=
            effect_field_count(session.slots[static_cast<std::size_t>(state.slot)].effects[static_cast<std::size_t>(effect_index)].kind);
    }
    if (state.page == Page::master) {
        if (state.focus_zone == 0) return true;
        const int effect_index = state.focus_zone - 1;
        return state.master_effect_field > 0 && state.master_effect_field <=
            effect_field_count(session.master_effects[static_cast<std::size_t>(effect_index)].kind);
    }
    return state.page == Page::memory && state.focus_zone == 2 && state.memory_setting > 0;
}

void adjust(cd::Session& session, UiState& state, float steps) {
    const int direction = steps < 0.0F ? -1 : 1;
    if (state.page == Page::perform) {
        if (state.focus_zone == 1) {
            float* value = macro_value(session.performance, parameter(state));
            *value = std::clamp(*value + steps * 0.01F, 0.0F, 1.0F);
        } else if (state.focus_zone == 2) {
            auto& level = session.slots[static_cast<std::size_t>(state.slot)].level;
            level = std::clamp(level + steps * 0.01F, 0.0F, 1.0F);
            session.scene_modified = true;
        }
        return;
    }
    if (state.page == Page::slot) {
        auto& slot = session.slots[static_cast<std::size_t>(state.slot)];
        if (state.focus_zone == 1) {
            switch (parameter(state)) {
            case 3: slot.frequency_hz = std::clamp(slot.frequency_hz * std::pow(2.0F, steps / 12.0F), kMinimumAudibleFrequency, 2'000.0F); break;
            case 4: slot.timbre = std::clamp(slot.timbre + steps * 0.01F, 0.0F, 1.0F); break;
            case 5: slot.color = std::clamp(slot.color + steps * 0.01F, 0.0F, 1.0F); break;
            case 6: slot.motion = std::clamp(slot.motion + steps * 0.01F, 0.0F, 1.0F); break;
            case 7: slot.texture = std::clamp(slot.texture + steps * 0.01F, 0.0F, 1.0F); break;
            case 8: slot.level = std::clamp(slot.level + steps * 0.01F, 0.0F, 1.0F); break;
            case 9: slot.pan = std::clamp(slot.pan + steps * 0.02F, -1.0F, 1.0F); break;
            default: break;
            }
        } else if (state.focus_zone == 2) {
            auto& mod = slot.modulators[static_cast<std::size_t>(state.actor_modulator)];
            switch (state.actor_advanced_field) {
            case 0: slot.plaits_model = static_cast<cd::PlaitsModel>((static_cast<int>(slot.plaits_model) + direction + 16) % 16); break;
            case 1: slot.plaits_output = static_cast<cd::PlaitsOutputMode>((static_cast<int>(slot.plaits_output) + direction + 4) % 4); break;
            case 2: if (!g_scales.empty()) { int i = 0; for (std::size_t n=0;n<g_scales.size();++n) if (g_scales[n].name==slot.tuning.name.data()) i=static_cast<int>(n); i=(i+direction+static_cast<int>(g_scales.size()))%static_cast<int>(g_scales.size()); cd::apply_scale(slot.tuning,g_scales[static_cast<std::size_t>(i)]); } break;
            case 3: slot.tuning.root_midi = std::clamp(slot.tuning.root_midi + static_cast<int>(steps), 0, 127); break;
            case 5: slot.euclidean.steps = std::clamp(slot.euclidean.steps + static_cast<int>(steps), 1, 32); slot.euclidean.pulses=std::min(slot.euclidean.pulses,slot.euclidean.steps); break;
            case 6: slot.euclidean.pulses = std::clamp(slot.euclidean.pulses + static_cast<int>(steps), 0, slot.euclidean.steps); break;
            case 7: slot.euclidean.rotation = (slot.euclidean.rotation + direction + slot.euclidean.steps) % slot.euclidean.steps; break;
            case 8: slot.euclidean.probability = std::clamp(slot.euclidean.probability + steps * 0.01F, 0.0F, 1.0F); break;
            case 9: state.actor_modulator = (state.actor_modulator + direction + 4) % 4; break;
            case 11: mod.wave = static_cast<cd::ModWave>((static_cast<int>(mod.wave) + direction + 4) % 4); break;
            case 12: mod.destination = static_cast<cd::ModDestination>((static_cast<int>(mod.destination) + direction + 11) % 11); break;
            case 13: mod.rate_hz = std::clamp(mod.rate_hz * std::pow(2.0F, steps / 12.0F), 0.001F, 40.0F); break;
            case 14: mod.depth = std::clamp(mod.depth + steps * 0.01F, -1.0F, 1.0F); break;
            case 15: { const int max_source=state.actor_modulator-1; if(max_source<0) mod.rate_mod_source=-1; else {const int count=max_source+2; mod.rate_mod_source=((mod.rate_mod_source+1+direction+count)%count)-1;} break; }
            case 16: mod.rate_mod_amount = std::clamp(mod.rate_mod_amount + steps * 0.01F, -1.0F, 1.0F); break;
            default: break;
            }
        }
        session.scene_modified = true;
        return;
    }
    if (state.page == Page::effects && state.focus_zone > 0) {
        const int effect_index = state.focus_zone - 1;
        auto& effect = session.slots[static_cast<std::size_t>(state.slot)].effects[static_cast<std::size_t>(effect_index)];
        if (state.effect_field > 0 && state.effect_field <= effect_field_count(effect.kind))
            *effect_value(effect, state.effect_field - 1) = std::clamp(*effect_value(effect, state.effect_field - 1) + steps * 0.01F, 0.0F, 1.0F);
        session.scene_modified = true;
        return;
    }
    if (state.page == Page::master) {
        if (state.focus_zone == 0) {
            if (parameter(state) == 0) session.master_level = std::clamp(session.master_level + steps * 0.01F, 0.0F, 1.0F);
            else session.tempo_bpm = std::clamp(session.tempo_bpm + steps, 10.0F, 300.0F);
        } else {
            const int effect_index = state.focus_zone - 1;
            auto& effect = session.master_effects[static_cast<std::size_t>(effect_index)];
            if (state.master_effect_field > 0 && state.master_effect_field <= effect_field_count(effect.kind))
                *effect_value(effect, state.master_effect_field - 1) = std::clamp(*effect_value(effect, state.master_effect_field - 1) + steps * 0.01F, 0.0F, 1.0F);
        }
        return;
    }
    if (state.page == Page::memory && state.focus_zone == 2) {
        if (state.memory_setting == 0) session.locale = session.locale == cd::Locale::ru ? cd::Locale::en : cd::Locale::ru;
        else {
            float& seconds = state.memory_setting == 1 ? session.fade_in_seconds : session.fade_out_seconds;
            seconds = std::clamp(seconds + steps * 0.25F, 0.25F, 30.0F);
        }
    }
}

void start_adjust(cd::Session& session, UiState& state, int direction, Uint32 now) {
    state.held_direction = direction;
    state.held_since = now;
    state.last_repeat = now;
    adjust(session, state, static_cast<float>(direction));
    if (!current_adjustable(session, state)) state.held_direction = 0;
}

bool repeat_adjust(cd::Session& session, UiState& state, Uint32 now) {
    if (state.held_direction == 0 || now - state.held_since < 330U) return false;
    const Uint32 elapsed = now - state.held_since;
    const Uint32 interval = elapsed >= 2'200U ? 16U : (elapsed >= 1'050U ? 35U : 70U);
    const float multiplier = elapsed >= 2'200U ? 5.0F : (elapsed >= 1'050U ? 2.0F : 1.0F);
    if (now - state.last_repeat < interval) return false;
    state.last_repeat = now;
    adjust(session, state, static_cast<float>(state.held_direction) * multiplier);
    return true;
}

void stop_adjust(UiState& state, int direction) noexcept {
    if (state.held_direction == direction) state.held_direction = 0;
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
        Page::perform, Page::slot, Page::effects, Page::master, Page::memory};
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
    const bool landscape_focus = state.focus_zone == 0;
    const bool macro_focus = state.focus_zone == 1;
    const bool actor_focus = state.focus_zone == 2;

    fill(renderer, {16, 52, 480, 25}, landscape_focus ? SDL_Color{73, 46, 104, 255} : SDL_Color{27, 23, 36, 255});
    if (landscape_focus) outline(renderer, {16, 52, 480, 25}, kInk);
    const std::string landscape = std::string{ru(session) ? "ЛАНДШАФТ: " : "LANDSCAPE: "} +
        std::string{scene_name(session.scene, ru(session))} + (session.scene_modified ? " *" : "");
    cd::ui::draw_text(renderer, 24, 59, landscape, landscape_focus ? kInk : kDim);
    cd::ui::draw_text(renderer, 462, 59, landscape_focus ? "A" : "", kInk);

    fill(renderer, {16, 80, 480, 157}, macro_focus ? SDL_Color{44, 31, 58, 255} : SDL_Color{27, 23, 36, 255});
    if (macro_focus) outline(renderer, {16, 80, 480, 157}, kInk);
    for (int index = 0; index < 5; ++index) {
        const int y = 88 + index * 28;
        const bool selected = macro_focus && parameter(state) == index;
        if (selected) fill(renderer, {21, y - 4, 470, 23}, {73, 46, 104, 255});
        cd::ui::draw_text(renderer, 28, y, macro_name(index, ru(session)), selected ? kInk : kDim);
        char number[12]{};
        std::snprintf(number, sizeof(number), "%d%%", static_cast<int>(std::lround(
            macro_value(session.performance, index) * 100.0F)));
        cd::ui::draw_text(renderer, 174 - cd::ui::text_width(number), y, number, selected ? kInk : kDim);
        bar(renderer, 194, y, 288, 10, macro_value(session.performance, index),
            selected ? kInk : kFxColors[static_cast<std::size_t>(index % 4)]);
    }

    fill(renderer, {16, 241, 480, 107}, actor_focus ? SDL_Color{44, 31, 58, 255} : SDL_Color{27, 23, 36, 255});
    if (actor_focus) outline(renderer, {16, 241, 480, 107}, kInk);
    cd::ui::draw_text(renderer, 24, 248,
        ru(session) ? "АКТЕРЫ ЛАНДШАФТА" : "LANDSCAPE ACTORS", actor_focus ? kInk : kDim);
    for (int index = 0; index < 4; ++index) {
        const int x = 22 + index * 117;
        const auto& slot = session.slots[static_cast<std::size_t>(index)];
        const bool selected = actor_focus && state.slot == index;
        fill(renderer, {x, 263, 109, 78}, selected ? SDL_Color{73, 46, 104, 255} : SDL_Color{20, 17, 28, 255});
        if (selected) outline(renderer, {x, 263, 109, 78}, kInk);
        cd::ui::draw_text(renderer, x + 6, 270,
            std::to_string(index + 1) + " " + std::string{engine_name(slot.engine, ru(session))},
            selected ? kInk : kDim);
        const float meter = std::clamp(telemetry.slot_rms[static_cast<std::size_t>(index)] * 4.2F, 0.0F, 1.0F);
        bar(renderer, x + 6, 289, 97, 8, meter, react(kFxColors[static_cast<std::size_t>(index)], meter));
        char level[16]{};
        std::snprintf(level, sizeof(level), "%s %d%%", ru(session) ? "УРОВ" : "LEVEL",
            static_cast<int>(std::lround(slot.level * 100.0F)));
        cd::ui::draw_text(renderer, x + 6, 304, level, selected ? kInk : kDim);
        cd::ui::draw_text(renderer, x + 6, 322,
            slot.enabled ? "A: MUTE" : (ru(session) ? "A: ВКЛ" : "A: ON"),
            slot.enabled ? kDim : kFxColors[0]);
    }
}

void draw_tracks(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state) {
    fill(renderer, {10, 48, 492, 306}, kPanel);
    const bool actor_focus = state.focus_zone == 0;
    fill(renderer, {14, 51, 484, 31}, actor_focus ? SDL_Color{44, 31, 58, 255} : SDL_Color{27, 23, 36, 255});
    if (actor_focus) outline(renderer, {14, 51, 484, 31}, kInk);
    for (int actor = 0; actor < 4; ++actor) {
        const int x = 18 + actor * 119;
        const bool selected = actor == state.slot;
        if (selected) fill(renderer, {x, 55, 111, 23}, {73, 46, 104, 255});
        cd::ui::draw_text(renderer, x + 6, 61,
            std::to_string(actor + 1) + " " + std::string{engine_name(session.slots[static_cast<std::size_t>(actor)].engine, ru(session))},
            selected ? kInk : kDim);
        const float meter = std::clamp(telemetry.slot_rms[static_cast<std::size_t>(actor)] * 4.2F, 0.0F, 1.0F);
        bar(renderer, x + 6, 73, 99, 4, meter, react(kFxColors[static_cast<std::size_t>(actor)], meter));
    }

    const bool show_advanced = state.focus_zone == 2;
    const bool list_focus = state.focus_zone == 1 || state.focus_zone == 2;
    const int tab_width = 236;
    fill(renderer, {18, 87, tab_width, 23}, !show_advanced ? SDL_Color{73, 46, 104, 255} : SDL_Color{27, 23, 36, 255});
    fill(renderer, {258, 87, tab_width, 23}, show_advanced ? SDL_Color{73, 46, 104, 255} : SDL_Color{27, 23, 36, 255});
    cd::ui::draw_text(renderer, 26, 94, ru(session) ? "ОСНОВНОЕ" : "BASIC", !show_advanced ? kInk : kDim);
    cd::ui::draw_text(renderer, 266, 94, ru(session) ? "РАСШИРЕННОЕ" : "ADVANCED", show_advanced ? kInk : kDim);

    fill(renderer, {18, 114, 476, 225}, list_focus ? SDL_Color{38, 28, 50, 255} : SDL_Color{27, 23, 36, 255});
    if (list_focus) outline(renderer, {18, 114, 476, 225}, kInk);
    if (!show_advanced) {
        for (int field = 0; field < 10; ++field) {
            const int y = 121 + field * 21;
            const bool active = state.focus_zone == 1 && parameter(state) == field;
            if (active) fill(renderer, {23, y - 3, 466, 18}, {73, 46, 104, 255});
            const std::string label{actor_basic_label(field, ru(session))};
            const std::string value = actor_basic_value(session, state, field);
            cd::ui::draw_text(renderer, 30, y, label, active ? kInk : kDim);
            cd::ui::draw_text(renderer, 482 - cd::ui::text_width(value), y, value,
                active ? kInk : kFxColors[static_cast<std::size_t>(state.slot)]);
        }
    } else {
        for (int field = 0; field < 17; ++field) {
            const bool right = field >= 9;
            const int row = right ? field - 9 : field;
            const int x = right ? 258 : 23;
            const int width = right ? 231 : 231;
            const int y = 121 + row * 24;
            const bool active = state.focus_zone == 2 && state.actor_advanced_field == field;
            if (active) fill(renderer, {x, y - 3, width, 20}, {73, 46, 104, 255});
            const std::string label{actor_advanced_label(field, ru(session))};
            const std::string value = actor_advanced_value(session, state, field);
            cd::ui::draw_text(renderer, x + 7, y, label, active ? kInk : kDim);
            cd::ui::draw_text(renderer, x + width - 7 - cd::ui::text_width(value), y, value,
                active ? kInk : kFxColors[static_cast<std::size_t>(state.slot)]);
        }
    }
    cd::ui::draw_text(renderer, 22, 344,
        actor_focus
            ? (ru(session) ? "D-PAD: АКТЕР  A: MUTE  X: ОСНОВНОЕ" : "D-PAD: ACTOR  A: MUTE  X: BASIC")
            : (ru(session) ? "UP/DN: ПАРАМЕТР  LT/RT: ЗНАЧЕНИЕ  A: ДЕЙСТВИЕ" : "UP/DN: PARAMETER  LT/RT: VALUE  A: ACTION"),
        kDim);
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
    fill(renderer, {10, 48, 492, 306}, kPanel);
    const bool actor_focus = state.focus_zone == 0;
    fill(renderer, {14, 51, 484, 31}, actor_focus ? SDL_Color{44, 31, 58, 255} : SDL_Color{27, 23, 36, 255});
    if (actor_focus) outline(renderer, {14, 51, 484, 31}, kInk);
    for (int actor = 0; actor < 4; ++actor) {
        const int x = 18 + actor * 119;
        const bool selected = actor == state.slot;
        if (selected) fill(renderer, {x, 55, 111, 23}, {73, 46, 104, 255});
        cd::ui::draw_text(renderer, x + 6, 61,
            std::to_string(actor + 1) + " " + std::string{engine_name(session.slots[static_cast<std::size_t>(actor)].engine, ru(session))},
            selected ? kInk : kDim);
        const float meter = std::clamp(telemetry.slot_rms[static_cast<std::size_t>(actor)] * 4.2F, 0.0F, 1.0F);
        bar(renderer, x + 6, 73, 99, 4, meter, react(kFxColors[static_cast<std::size_t>(actor)], meter));
    }

    const auto& slot = session.slots[static_cast<std::size_t>(state.slot)];
    const int active_effect = state.focus_zone > 0 ? state.focus_zone - 1 : parameter(state);
    for (int index = 0; index < 4; ++index) {
        const int x = 18 + index * 119;
        const bool selected = index == active_effect;
        const bool focused = state.focus_zone == index + 1;
        fill(renderer, {x, 87, 111, 38}, focused ? SDL_Color{73, 46, 104, 255} : SDL_Color{27, 23, 36, 255});
        if (focused) outline(renderer, {x, 87, 111, 38}, kInk);
        cd::ui::draw_text(renderer, x + 6, 93, "FX " + std::to_string(index + 1), selected ? kInk : kDim);
        cd::ui::draw_text(renderer, x + 6, 108, effect_name(slot.effects[static_cast<std::size_t>(index)].kind, ru(session)),
            selected ? kFxColors[static_cast<std::size_t>(index)] : kDim);
    }

    const auto& effect = slot.effects[static_cast<std::size_t>(active_effect)];
    const bool editor_focus = state.focus_zone > 0;
    fill(renderer, {18, 131, 476, 210}, editor_focus ? SDL_Color{38, 28, 50, 255} : SDL_Color{27, 23, 36, 255});
    if (editor_focus) outline(renderer, {18, 131, 476, 210}, kInk);
    cd::ui::draw_text(renderer, 28, 140,
        "FX " + std::to_string(active_effect + 1) + "  " + std::string{effect_name(effect.kind, ru(session))},
        kFxColors[static_cast<std::size_t>(active_effect)], 2);
    effect_visual(renderer, 28, 174, 190, 104, effect, kFxColors[static_cast<std::size_t>(active_effect)]);

    const int fields = effect_field_count(effect.kind);
    const int rows = 1 + fields;
    for (int row = 0; row < rows; ++row) {
        const int y = 174 + row * 38;
        const bool active = editor_focus && state.effect_field == row;
        if (active) fill(renderer, {234, y - 5, 248, 29}, {73, 46, 104, 255});
        const std::string label = row == 0
            ? (ru(session) ? "ТИП ЭФФЕКТА" : "EFFECT TYPE")
            : std::string{effect_field(effect.kind, row - 1, ru(session))};
        const std::string value = row == 0
            ? std::string{effect_name(effect.kind, ru(session))}
            : std::to_string(static_cast<int>(std::lround(effect_value(effect, row - 1) * 100.0F))) + "%";
        cd::ui::draw_text(renderer, 242, y, label, active ? kInk : kDim);
        cd::ui::draw_text(renderer, 474 - cd::ui::text_width(value), y, value,
            active ? kInk : kFxColors[static_cast<std::size_t>(active_effect)]);
        if (row > 0) bar(renderer, 242, y + 16, 230, 7, effect_value(effect, row - 1),
            active ? kInk : kFxColors[static_cast<std::size_t>(active_effect)]);
    }
    cd::ui::draw_text(renderer, 22, 344,
        actor_focus
            ? (ru(session) ? "D-PAD: АКТЕР  A: MUTE  X: FX1" : "D-PAD: ACTOR  A: MUTE  X: FX1")
            : (ru(session) ? "X: СЛЕДУЮЩИЙ FX  UP/DN: СТРОКА  LT/RT: ЗНАЧЕНИЕ  A: СПИСОК" : "X: NEXT FX  UP/DN: ROW  LT/RT: VALUE  A: LIST"),
        kDim);
}

void draw_master(
    SDL_Renderer* renderer,
    const cd::Session& session,
    const cd::AudioTelemetry& telemetry,
    const UiState& state) {
    fill(renderer, {10, 48, 492, 306}, kPanel);
    const bool signal_focus = state.focus_zone == 0;
    fill(renderer, {16, 51, 480, 105}, signal_focus ? SDL_Color{44, 31, 58, 255} : SDL_Color{27, 23, 36, 255});
    if (signal_focus) outline(renderer, {16, 51, 480, 105}, kInk);
    cd::ui::draw_text(renderer, 24, 58, ru(session) ? "ИТОГОВЫЙ СИГНАЛ" : "MASTER SIGNAL", signal_focus ? kInk : kDim);
    scope(renderer, 24, 74, 460, 43, telemetry.master_scope, telemetry.master_rms, telemetry.master_peak);
    const std::array<std::string_view, 2> labels{ru(session) ? "ГРОМКОСТЬ" : "LEVEL", ru(session) ? "ТЕМП" : "TEMPO"};
    for (int field = 0; field < 2; ++field) {
        const int y = 124 + field * 22;
        const bool selected = signal_focus && parameter(state) == field;
        if (selected) fill(renderer, {22, y - 3, 466, 18}, {73, 46, 104, 255});
        cd::ui::draw_text(renderer, 30, y, labels[static_cast<std::size_t>(field)], selected ? kInk : kDim);
        const float normalized = field == 0 ? session.master_level : (session.tempo_bpm - 10.0F) / 290.0F;
        bar(renderer, 136, y + 1, 270, 8, normalized, selected ? kInk : kPurple);
        const std::string value = field == 0
            ? std::to_string(static_cast<int>(std::lround(session.master_level * 100.0F))) + "%"
            : std::to_string(static_cast<int>(std::lround(session.tempo_bpm))) + " BPM";
        cd::ui::draw_text(renderer, 480 - cd::ui::text_width(value), y, value, selected ? kInk : kDim);
    }

    const int active_effect = state.focus_zone > 0 ? state.focus_zone - 1 : state.master_effect;
    for (int index = 0; index < 4; ++index) {
        const int x = 18 + index * 119;
        const bool focused = state.focus_zone == index + 1;
        fill(renderer, {x, 162, 111, 37}, focused ? SDL_Color{73, 46, 104, 255} : SDL_Color{27, 23, 36, 255});
        if (focused) outline(renderer, {x, 162, 111, 37}, kInk);
        cd::ui::draw_text(renderer, x + 6, 168, "FX " + std::to_string(index + 1), focused ? kInk : kDim);
        cd::ui::draw_text(renderer, x + 6, 183, effect_name(session.master_effects[static_cast<std::size_t>(index)].kind, ru(session)),
            focused ? kFxColors[static_cast<std::size_t>(index)] : kDim);
    }

    const auto& effect = session.master_effects[static_cast<std::size_t>(active_effect)];
    const bool editor_focus = state.focus_zone > 0;
    fill(renderer, {18, 204, 476, 137}, editor_focus ? SDL_Color{38, 28, 50, 255} : SDL_Color{27, 23, 36, 255});
    if (editor_focus) outline(renderer, {18, 204, 476, 137}, kInk);
    effect_visual(renderer, 28, 216, 174, 105, effect, kFxColors[static_cast<std::size_t>(active_effect)]);
    const int fields = effect_field_count(effect.kind);
    for (int row = 0; row < 1 + fields; ++row) {
        const int y = 216 + row * 28;
        const bool active = editor_focus && state.master_effect_field == row;
        if (active) fill(renderer, {216, y - 4, 268, 22}, {73, 46, 104, 255});
        const std::string label = row == 0
            ? (ru(session) ? "ТИП ЭФФЕКТА" : "EFFECT TYPE")
            : std::string{effect_field(effect.kind, row - 1, ru(session))};
        const std::string value = row == 0
            ? std::string{effect_name(effect.kind, ru(session))}
            : std::to_string(static_cast<int>(std::lround(effect_value(effect, row - 1) * 100.0F))) + "%";
        cd::ui::draw_text(renderer, 224, y, label, active ? kInk : kDim);
        cd::ui::draw_text(renderer, 476 - cd::ui::text_width(value), y, value,
            active ? kInk : kFxColors[static_cast<std::size_t>(active_effect)]);
    }
    cd::ui::draw_text(renderer, 22, 344,
        signal_focus
            ? (ru(session) ? "UP/DN: ПАРАМЕТР  LT/RT: ЗНАЧЕНИЕ  X: MASTER FX1" : "UP/DN: PARAMETER  LT/RT: VALUE  X: MASTER FX1")
            : (ru(session) ? "X: СЛЕДУЮЩИЙ FX  UP/DN: СТРОКА  LT/RT: ЗНАЧЕНИЕ  A: СПИСОК" : "X: NEXT FX  UP/DN: ROW  LT/RT: VALUE  A: LIST"),
        kDim);
}

bool memory_slot_exists(int index) {
    if (index < 0 || index >= static_cast<int>(cd::kMemorySlots)) return false;
    std::error_code error;
    return std::filesystem::is_regular_file(g_memory_paths[static_cast<std::size_t>(index)], error);
}

void set_toast(UiState& state, std::string text) {
    state.toast = std::move(text);
    state.toast_until = SDL_GetTicks() + 1'800U;
}

bool save_memory_slot(const cd::Session& session, UiState& state) {
    if (g_memory_paths[static_cast<std::size_t>(state.memory_slot)].empty()) return false;
    std::string error;
    const bool ok = cd::save_session(session, g_memory_paths[static_cast<std::size_t>(state.memory_slot)], error);
    set_toast(state, ok ? (ru(session) ? "СОХРАНЕНО В СЛОТ " : "SAVED TO SLOT ") + std::to_string(state.memory_slot + 1)
                        : (ru(session) ? "ОШИБКА СОХРАНЕНИЯ" : "SAVE FAILED"));
    return ok;
}

bool load_memory_slot(cd::Session& session, UiState& state) {
    const auto path = g_memory_paths[static_cast<std::size_t>(state.memory_slot)];
    if (path.empty() || !memory_slot_exists(state.memory_slot)) {
        set_toast(state, ru(session) ? "СЛОТ ПУСТ" : "SLOT IS EMPTY");
        return false;
    }
    cd::Session loaded{};
    std::string error;
    if (!cd::load_session(path, loaded, error)) {
        set_toast(state, ru(session) ? "ОШИБКА ЗАГРУЗКИ" : "LOAD FAILED");
        return false;
    }
    const auto locale = session.locale;
    const float fade_in = session.fade_in_seconds;
    const float fade_out = session.fade_out_seconds;
    session = loaded;
    session.locale = locale;
    session.fade_in_seconds = fade_in;
    session.fade_out_seconds = fade_out;
    session.performance.morph = 0.0F;
    session.performance.morph_target = session.scene;
    set_toast(state, (ru(session) ? "ЗАГРУЖЕН СЛОТ " : "LOADED SLOT ") + std::to_string(state.memory_slot + 1));
    return true;
}

void reset_landscape(cd::Session& session, UiState& state) {
    const auto master_effects = session.master_effects;
    cd::apply_scene_recipe(session, session.scene);
    session.master_effects = master_effects;
    session.performance.morph = 0.0F;
    session.performance.morph_target = session.scene;
    set_toast(state, ru(session) ? "ЛАНДШАФТ ВОССТАНОВЛЕН" : "LANDSCAPE RESTORED");
}

void draw_setup(SDL_Renderer* renderer, const cd::Session& session, const UiState& state) {
    fill(renderer, {10, 48, 492, 306}, kPanel);
    const bool slots_focus = state.focus_zone == 0;
    const bool actions_focus = state.focus_zone == 1;
    const bool settings_focus = state.focus_zone == 2;
    fill(renderer, {16, 51, 480, 137}, slots_focus ? SDL_Color{44, 31, 58, 255} : SDL_Color{27, 23, 36, 255});
    if (slots_focus) outline(renderer, {16, 51, 480, 137}, kInk);
    cd::ui::draw_text(renderer, 24, 58,
        ru(session) ? "8 СЛОТОВ ДЛЯ СВОИХ СОСТОЯНИЙ" : "8 SLOTS FOR YOUR STATES", slots_focus ? kInk : kDim);
    for (int index = 0; index < 8; ++index) {
        const int column = index % 4;
        const int row = index / 4;
        const int x = 22 + column * 118;
        const int y = 82 + row * 49;
        const bool selected = slots_focus && state.memory_slot == index;
        fill(renderer, {x, y, 106, 39}, selected ? SDL_Color{73, 46, 104, 255} : SDL_Color{20, 17, 28, 255});
        if (selected) outline(renderer, {x, y, 106, 39}, kInk);
        cd::ui::draw_text(renderer, x + 8, y + 7,
            (ru(session) ? "СЛОТ " : "SLOT ") + std::to_string(index + 1), selected ? kInk : kDim);
        cd::ui::draw_text(renderer, x + 8, y + 23,
            memory_slot_exists(index) ? (ru(session) ? "СОХРАНЕН" : "SAVED") : (ru(session) ? "ПУСТ" : "EMPTY"),
            memory_slot_exists(index) ? kFxColors[2] : kDim);
    }

    fill(renderer, {16, 193, 274, 103}, actions_focus ? SDL_Color{44, 31, 58, 255} : SDL_Color{27, 23, 36, 255});
    if (actions_focus) outline(renderer, {16, 193, 274, 103}, kInk);
    const std::array<std::string, 3> actions{
        ru(session) ? "ЗАГРУЗИТЬ ВЫБРАННЫЙ СЛОТ" : "LOAD SELECTED SLOT",
        ru(session) ? "СОХРАНИТЬ В ВЫБРАННЫЙ СЛОТ" : "SAVE TO SELECTED SLOT",
        ru(session) ? "ВОССТАНОВИТЬ ЛАНДШАФТ" : "RESTORE LANDSCAPE"};
    for (int index = 0; index < 3; ++index) {
        const int y = 204 + index * 28;
        const bool selected = actions_focus && state.memory_action == index;
        if (selected) fill(renderer, {22, y - 4, 260, 23}, {73, 46, 104, 255});
        cd::ui::draw_text(renderer, 28, y, actions[static_cast<std::size_t>(index)], selected ? kInk : kDim);
    }

    fill(renderer, {296, 193, 200, 103}, settings_focus ? SDL_Color{44, 31, 58, 255} : SDL_Color{27, 23, 36, 255});
    if (settings_focus) outline(renderer, {296, 193, 200, 103}, kInk);
    const std::array<std::string, 3> settings{
        ru(session) ? "ЯЗЫК" : "LANGUAGE",
        ru(session) ? "ФЕЙД ВХОДА" : "FADE IN",
        ru(session) ? "ФЕЙД ВЫХОДА" : "FADE OUT"};
    for (int index = 0; index < 3; ++index) {
        const int y = 204 + index * 28;
        const bool selected = settings_focus && state.memory_setting == index;
        if (selected) fill(renderer, {302, y - 4, 188, 23}, {73, 46, 104, 255});
        cd::ui::draw_text(renderer, 308, y, settings[static_cast<std::size_t>(index)], selected ? kInk : kDim);
        std::string value;
        if (index == 0) value = session.locale == cd::Locale::ru ? "РУССКИЙ" : "ENGLISH";
        else {
            char number[16]{};
            std::snprintf(number, sizeof(number), "%.2f S", static_cast<double>(index == 1 ? session.fade_in_seconds : session.fade_out_seconds));
            value = number;
        }
        cd::ui::draw_text(renderer, 484 - cd::ui::text_width(value), y, value, selected ? kInk : kDim);
    }
    cd::ui::draw_text(renderer, 22, 306,
        ru(session) ? "АВТОСОХРАНЕНИЕ: ПОСЛЕДНЕЕ СОСТОЯНИЕ" : "AUTOSAVE: LAST STATE IS ALWAYS RESUMED", kDim);
    draw_myldy_mark(renderer, 22, 326, 1, kDim);
    cd::ui::draw_text(renderer, 58, 330, "DEVELOPED BY MYLDY DESIGN  @MYLDY20", kDim);
    cd::ui::draw_text(renderer, 484 - cd::ui::text_width("V0.11.1"), 330, "V0.11.1", kDim);
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
                ? (ru(session) ? "D-PAD ВЫБОР   A ПРИНЯТЬ   B НАЗАД"
                               : "D-PAD SELECT   A APPLY   B BACK")
                : (ru(session) ? "D-PAD ВЫБОР   E ПРИНЯТЬ   ESC НАЗАД"
                               : "D-PAD SELECT   E APPLY   ESC BACK"), kDim);
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
                ? (ru(session) ? "D-PAD ВЫБОР  A OK  B НАЗАД"
                               : "D-PAD SELECT  A OK  B BACK")
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
                ? (ru(session) ? "D-PAD ВЫБОР  A OK  B НАЗАД"
                               : "D-PAD SELECT  A OK  B BACK")
                : (ru(session) ? "LT/RT ГРУППА  UP/DN ЭФФЕКТ  E OK  ESC НАЗАД"
                               : "LT/RT GROUP  UP/DN EFFECT  E OK  ESC BACK"), kDim);
    }
}


int draw_wrapped_text(SDL_Renderer* renderer, int x, int y, int max_width,
    std::string text, SDL_Color color, int line_height = 18) {
    std::string line;
    std::string word;
    auto flush_word = [&]() mutable {
        if (word.empty()) return;
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && cd::ui::text_width(candidate) > max_width) {
            cd::ui::draw_text(renderer, x, y, line, color);
            y += line_height;
            line = word;
        } else {
            line = candidate;
        }
        word.clear();
    };
    for (char character : text) {
        if (character == ' ') flush_word();
        else word.push_back(character);
    }
    flush_word();
    if (!line.empty()) {
        cd::ui::draw_text(renderer, x, y, line, color);
        y += line_height;
    }
    return y;
}

std::string page_purpose(const cd::Session& session, Page page) {
    switch (page) {
    case Page::perform: return ru(session) ? "ВЫБЕРИТЕ МЕСТО, ЗАТЕМ УПРАВЛЯЙТЕ ЕГО СОСТОЯНИЕМ И АКТЕРАМИ."
                                           : "CHOOSE A PLACE, THEN SHAPE ITS STATE AND ACTORS.";
    case Page::slot: return ru(session) ? "НАСТРОЙТЕ ОДНОГО АКТЕРА. X ПЕРЕКЛЮЧАЕТ ОСНОВНОЕ И РАСШИРЕННОЕ."
                                        : "SHAPE ONE ACTOR. X SWITCHES BASIC AND ADVANCED.";
    case Page::effects: return ru(session) ? "ЧЕТЫРЕ ЭФФЕКТА В ЦЕПОЧКЕ ВЫБРАННОГО АКТЕРА."
                                           : "FOUR SERIAL EFFECTS FOR THE SELECTED ACTOR.";
    case Page::master: return ru(session) ? "ОБЩИЙ СИГНАЛ И ЧЕТЫРЕ ЭФФЕКТА ПОСЛЕ СМЕШИВАНИЯ АКТЕРОВ."
                                          : "FINAL SIGNAL AND FOUR EFFECTS AFTER THE ACTOR MIX.";
    case Page::memory: return ru(session) ? "СОХРАНЯЙТЕ СОСТОЯНИЯ, ВОЗВРАЩАЙТЕСЬ К НИМ И ВОССТАНАВЛИВАЙТЕ МЕСТО."
                                          : "SAVE STATES, RETURN TO THEM, OR RESTORE THE PLACE.";
    }
    return {};
}

void draw_help_overlay(SDL_Renderer* renderer, const cd::Session& session, const UiState& state) {
    fill(renderer, {24, 52, 464, 292}, {8, 7, 12, 248});
    outline(renderer, {24, 52, 464, 292}, kInk);
    cd::ui::draw_text(renderer, 38, 68,
        std::string{ru(session) ? "ПОМОЩЬ: " : "HELP: "} + std::string{page_name(state.page, ru(session))}, kInk, 2);
    int y = draw_wrapped_text(renderer, 38, 108, 420, page_purpose(session, state.page), kDim, 18) + 10;
    const std::array<std::pair<std::string_view, std::string_view>, 7> controls{{
        {"D-PAD", ru(session) ? "ВЫБОР И ИЗМЕНЕНИЕ" : "SELECT AND CHANGE"},
        {"A", ru(session) ? "ОТКРЫТЬ / ПРИМЕНИТЬ" : "OPEN / APPLY"},
        {"B", ru(session) ? "НАЗАД / ОТМЕНА" : "BACK / CANCEL"},
        {"X", ru(session) ? "СЛЕДУЮЩИЙ РАЗДЕЛ ЭКРАНА" : "NEXT SECTION ON PAGE"},
        {"L / R", ru(session) ? "ПРЕДЫДУЩАЯ / СЛЕДУЮЩАЯ СТРАНИЦА" : "PREVIOUS / NEXT PAGE"},
        {"START", ru(session) ? "БЫСТРОЕ МЕНЮ" : "QUICK MENU"},
        {"SELECT", ru(session) ? "ПЛАВНО ОТКРЫТЬ / ЗАКРЫТЬ ВЫХОД" : "FADE OUTPUT IN / OUT"},
    }};
    for (const auto& control : controls) {
        cd::ui::draw_text(renderer, 42, y, control.first, kInk);
        cd::ui::draw_text(renderer, 142, y, control.second, kDim);
        y += 22;
    }
    cd::ui::draw_text(renderer, 38, 322, ru(session) ? "Y: ЗАКРЫТЬ ПОМОЩЬ" : "Y: CLOSE HELP", kInk);
}

void draw_menu_overlay(SDL_Renderer* renderer, const cd::Session& session, const UiState& state) {
    fill(renderer, {92, 62, 328, 272}, {8, 7, 12, 250});
    outline(renderer, {92, 62, 328, 272}, kInk);
    cd::ui::draw_text(renderer, 112, 78, ru(session) ? "БЫСТРОЕ МЕНЮ" : "QUICK MENU", kInk, 2);
    const std::array<std::string, 6> items{
        ru(session) ? "ПРОДОЛЖИТЬ" : "CONTINUE",
        (ru(session) ? "СОХРАНИТЬ В СЛОТ " : "SAVE TO SLOT ") + std::to_string(state.memory_slot + 1),
        (ru(session) ? "ЗАГРУЗИТЬ СЛОТ " : "LOAD SLOT ") + std::to_string(state.memory_slot + 1),
        ru(session) ? "ВОССТАНОВИТЬ ЛАНДШАФТ" : "RESTORE LANDSCAPE",
        ru(session) ? "ОТКРЫТЬ ПАМЯТЬ" : "OPEN MEMORY",
        ru(session) ? "СОХРАНИТЬ И ВЫЙТИ" : "SAVE AND EXIT"};
    for (int item = 0; item < static_cast<int>(items.size()); ++item) {
        const int y = 126 + item * 31;
        const bool selected = item == state.menu_item;
        if (selected) fill(renderer, {106, y - 5, 300, 25}, {73, 46, 104, 255});
        cd::ui::draw_text(renderer, 116, y, items[static_cast<std::size_t>(item)], selected ? kInk : kDim);
    }
    cd::ui::draw_text(renderer, 112, 316, ru(session) ? "LT/RT: СЛОТ  A: OK  B: НАЗАД" : "LT/RT: SLOT  A: OK  B: BACK", kDim);
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
    draw_header(renderer, session, telemetry, state);
    if (state.page == Page::perform) draw_scene(renderer, session, telemetry, state);
    else if (state.page == Page::slot) draw_tracks(renderer, session, telemetry, state);
    else if (state.page == Page::effects) draw_effects(renderer, session, telemetry, state);
    else if (state.page == Page::master) draw_master(renderer, session, telemetry, state);
    else draw_setup(renderer, session, state);

    if (state.picker != Picker::none) draw_picker(renderer, session, state);
    if (state.menu_open) draw_menu_overlay(renderer, session, state);
    if (state.help_open) draw_help_overlay(renderer, session, state);

    if (!state.help_open && !state.menu_open && state.picker == Picker::none) {
        cd::ui::draw_text(renderer, 10, 370,
            ru(session) ? "L/R СТРАНИЦА  X РАЗДЕЛ  A ДЕЙСТВИЕ  Y ПОМОЩЬ  START МЕНЮ"
                        : "L/R PAGE  X SECTION  A ACTION  Y HELP  START MENU", kDim);
    }
    if (state.held_direction != 0 && now - state.held_since >= 1'050U) {
        cd::ui::draw_text(renderer, 466, 357, now - state.held_since >= 2'200U ? ">>>" : ">>", {239, 169, 80, 255});
    }
    if (!state.toast.empty() && now < state.toast_until) {
        const int width = std::min(470, cd::ui::text_width(state.toast) + 20);
        fill(renderer, {(kWidth - width) / 2, 341, width, 25}, {15, 13, 20, 238});
        outline(renderer, {(kWidth - width) / 2, 341, width, 25}, kFxColors[2]);
        cd::ui::draw_text(renderer, (kWidth - cd::ui::text_width(state.toast)) / 2, 348, state.toast, kInk);
    }
    if (now < state.kill_flash_until) {
        outline(renderer, {2, 2, kWidth - 4, kHeight - 4}, {242, 70, 82, 255});
        cd::ui::draw_text(renderer, 206, 180, ru(session) ? "KILL: ТИШИНА" : "KILL: SILENCE", {242, 70, 82, 255});
    }
    SDL_RenderPresent(renderer);
}

void cycle_focus(UiState& state) noexcept {
    state.focus_zone = (state.focus_zone + 1) % focus_zone_count(state.page);
    if (state.page == Page::effects && state.focus_zone > 0) {
        parameter(state) = state.focus_zone - 1;
        state.effect_field = 0;
    }
    if (state.page == Page::master && state.focus_zone > 0) {
        state.master_effect = state.focus_zone - 1;
        state.master_effect_field = 0;
    }
    state.held_direction = 0;
}

void change_page(UiState& state, int direction) noexcept {
    constexpr int count = 5;
    state.page = static_cast<Page>((page_index(state.page) + direction + count) % count);
    switch (state.page) {
    case Page::perform: state.focus_zone = 1; break;
    case Page::slot: state.focus_zone = 1; break;
    case Page::effects: state.focus_zone = 1; parameter(state) = 0; state.effect_field = 0; break;
    case Page::master: state.focus_zone = 0; break;
    case Page::memory: state.focus_zone = 0; break;
    }
    state.held_direction = 0;
}

bool execute_memory_action(cd::Session& session, UiState& state) {
    if (state.memory_action == 0) return load_memory_slot(session, state);
    if (state.memory_action == 1) return save_memory_slot(session, state);
    reset_landscape(session, state);
    return true;
}

void activate_current(cd::Session& session, UiState& state) {
    if (state.page == Page::perform) {
        if (state.focus_zone == 0) open_scene_picker(state, session);
        else if (state.focus_zone == 2) {
            auto& actor = session.slots[static_cast<std::size_t>(state.slot)];
            actor.enabled = !actor.enabled;
            session.scene_modified = true;
            set_toast(state, actor.enabled ? (ru(session) ? "АКТЕР ВКЛЮЧЕН" : "ACTOR ON")
                                           : (ru(session) ? "АКТЕР ЗАГЛУШЕН" : "ACTOR MUTED"));
        }
        return;
    }
    if (state.page == Page::slot) {
        auto& actor = session.slots[static_cast<std::size_t>(state.slot)];
        if (state.focus_zone == 0) {
            actor.enabled = !actor.enabled;
            set_toast(state, actor.enabled ? (ru(session) ? "АКТЕР ВКЛЮЧЕН" : "ACTOR ON")
                                           : (ru(session) ? "АКТЕР ЗАГЛУШЕН" : "ACTOR MUTED"));
        } else if (state.focus_zone == 1) {
            if (parameter(state) == 0) actor.enabled = !actor.enabled;
            else if (parameter(state) == 1) toggle_actor_source(session, state.slot);
            else if (parameter(state) == 2) {
                if (actor.engine == cd::EngineKind::plaits) {
                    state.focus_zone = 2;
                    state.actor_advanced_field = 0;
                } else open_engine_picker(state, session);
            }
        } else {
            auto& mod = actor.modulators[static_cast<std::size_t>(state.actor_modulator)];
            if (state.actor_advanced_field == 4) actor.euclidean.enabled = !actor.euclidean.enabled;
            else if (state.actor_advanced_field == 9) state.actor_modulator = (state.actor_modulator + 1) % 4;
            else if (state.actor_advanced_field == 10) mod.enabled = !mod.enabled;
        }
        session.scene_modified = true;
        return;
    }
    if (state.page == Page::effects) {
        if (state.focus_zone == 0) {
            auto& actor = session.slots[static_cast<std::size_t>(state.slot)];
            actor.enabled = !actor.enabled;
            session.scene_modified = true;
        } else if (state.effect_field == 0) {
            parameter(state) = state.focus_zone - 1;
            open_effect_picker(state, session, false);
        }
        return;
    }
    if (state.page == Page::master) {
        if (state.focus_zone > 0 && state.master_effect_field == 0) {
            state.master_effect = state.focus_zone - 1;
            open_effect_picker(state, session, true);
        }
        return;
    }
    if (state.page == Page::memory) {
        if (state.focus_zone == 0) load_memory_slot(session, state);
        else if (state.focus_zone == 1) execute_memory_action(session, state);
    }
}

void execute_quick_menu(cd::Session& session, UiState& state) {
    switch (state.menu_item) {
    case 0: state.menu_open = false; break;
    case 1: save_memory_slot(session, state); state.menu_open = false; break;
    case 2: load_memory_slot(session, state); state.menu_open = false; break;
    case 3: reset_landscape(session, state); state.menu_open = false; break;
    case 4: state.page = Page::memory; state.focus_zone = 0; state.menu_open = false; break;
    case 5: state.request_exit = true; break;
    }
}

void go_back(UiState& state) noexcept {
    if (state.help_open) state.help_open = false;
    else if (state.menu_open) state.menu_open = false;
    else if (state.picker != Picker::none) { state.picker = Picker::none; state.picker_master = false; }
    else if (state.page != Page::perform) { state.page = Page::perform; state.focus_zone = 1; }
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

bool handle_dpad(cd::Session& session, UiState& state, int horizontal, int vertical, Uint32 now) {
    if (state.help_open) return false;
    if (state.menu_open) {
        if (vertical != 0) state.menu_item = (state.menu_item + vertical + 6) % 6;
        if (horizontal != 0 && (state.menu_item == 1 || state.menu_item == 2))
            state.memory_slot = (state.memory_slot + horizontal + static_cast<int>(cd::kMemorySlots)) % static_cast<int>(cd::kMemorySlots);
        return false;
    }
    if (state.picker != Picker::none) { move_picker(state, horizontal, vertical); return false; }

    if (state.page == Page::perform) {
        if (state.focus_zone == 0) return false;
        if (state.focus_zone == 1) {
            if (vertical != 0) parameter(state) = (parameter(state) + vertical + 5) % 5;
            if (horizontal != 0) { start_adjust(session, state, horizontal, now); return true; }
        } else {
            if (horizontal != 0) state.slot = (state.slot + horizontal + 4) % 4;
            if (vertical != 0) { start_adjust(session, state, -vertical, now); return true; }
        }
        return false;
    }
    if (state.page == Page::slot) {
        auto& actor = session.slots[static_cast<std::size_t>(state.slot)];
        if (state.focus_zone == 0) {
            if (horizontal != 0) state.slot = (state.slot + horizontal + 4) % 4;
            return false;
        }
        if (state.focus_zone == 1) {
            if (vertical != 0) parameter(state) = (parameter(state) + vertical + 10) % 10;
            if (horizontal != 0 && parameter(state) >= 3) { start_adjust(session, state, horizontal, now); return true; }
            if (horizontal != 0 && parameter(state) == 0) { actor.enabled = horizontal > 0; session.scene_modified = true; return true; }
            if (horizontal != 0 && parameter(state) == 1) {
                if (horizontal > 0 && actor.engine != cd::EngineKind::plaits) toggle_actor_source(session, state.slot);
                else if (horizontal < 0 && actor.engine == cd::EngineKind::plaits) toggle_actor_source(session, state.slot);
                return true;
            }
        } else {
            if (vertical != 0) state.actor_advanced_field = (state.actor_advanced_field + vertical + 17) % 17;
            if (horizontal != 0) { start_adjust(session, state, horizontal, now); return true; }
        }
        return false;
    }
    if (state.page == Page::effects) {
        if (state.focus_zone == 0) {
            if (horizontal != 0) state.slot = (state.slot + horizontal + 4) % 4;
            return false;
        }
        const int effect_index = state.focus_zone - 1;
        parameter(state) = effect_index;
        auto& effect = session.slots[static_cast<std::size_t>(state.slot)].effects[static_cast<std::size_t>(effect_index)];
        const int rows = 1 + effect_field_count(effect.kind);
        if (vertical != 0) state.effect_field = (state.effect_field + vertical + rows) % rows;
        if (horizontal != 0) {
            if (state.effect_field == 0) {
                cycle_effect_kind(effect, horizontal);
                state.effect_field = 0;
                session.scene_modified = true;
                return true;
            }
            start_adjust(session, state, horizontal, now);
            return true;
        }
        return false;
    }
    if (state.page == Page::master) {
        if (state.focus_zone == 0) {
            if (vertical != 0) parameter(state) = (parameter(state) + vertical + 2) % 2;
            if (horizontal != 0) { start_adjust(session, state, horizontal, now); return true; }
            return false;
        }
        const int effect_index = state.focus_zone - 1;
        state.master_effect = effect_index;
        auto& effect = session.master_effects[static_cast<std::size_t>(effect_index)];
        const int rows = 1 + effect_field_count(effect.kind);
        if (vertical != 0) state.master_effect_field = (state.master_effect_field + vertical + rows) % rows;
        if (horizontal != 0) {
            if (state.master_effect_field == 0) {
                cycle_effect_kind(effect, horizontal);
                state.master_effect_field = 0;
                return true;
            }
            start_adjust(session, state, horizontal, now);
            return true;
        }
        return false;
    }
    if (state.focus_zone == 0) {
        if (horizontal != 0) state.memory_slot = (state.memory_slot + horizontal + 8) % 8;
        if (vertical != 0) state.memory_slot = (state.memory_slot + vertical * 4 + 8) % 8;
    } else if (state.focus_zone == 1 && vertical != 0) {
        state.memory_action = (state.memory_action + vertical + 3) % 3;
    } else if (state.focus_zone == 2) {
        if (vertical != 0) state.memory_setting = (state.memory_setting + vertical + 3) % 3;
        if (horizontal != 0) { start_adjust(session, state, horizontal, now); return true; }
    }
    return false;
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
        g_data_root = root;
        autosave_path = root / "autosave.cdrone";
        legacy_autosave_path = root / "myldy20" / "cursed-drone" / "autosave.cdrone";
        g_scales = cd::load_scala_scales({root / "scales", std::filesystem::path{"assets/scales"}});
    } else if (char* preference_path = SDL_GetPrefPath("myldy20", "cursed-drone")) {
        g_data_root = std::filesystem::path{preference_path};
        autosave_path = g_data_root / "autosave.cdrone";
        g_scales = cd::load_scala_scales({g_data_root / "scales",
            std::filesystem::path{"assets/scales"}});
        SDL_free(preference_path);
    }
    if (g_scales.empty()) g_scales.push_back(cd::equal_temperament_scale());
    if (!g_data_root.empty()) {
        std::error_code directory_error;
        std::filesystem::create_directories(g_data_root, directory_error);
        for (std::size_t index = 0; index < cd::kMemorySlots; ++index) {
            g_memory_paths[index] = g_data_root / ("memory-" + std::to_string(index + 1U) + ".cdrone");
        }
    }

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
    // The guided workflow keeps landscape changes explicit. Legacy half-morph states
    // are collapsed back to the currently selected landscape on startup.
    session.performance.morph = 0.0F;
    session.performance.morph_target = session.scene;
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
                    if (event.key.keysym.sym == SDLK_ESCAPE) state.picker = Picker::none;
                    else if (event.key.keysym.sym == SDLK_LEFT) move_picker(state, -1, 0);
                    else if (event.key.keysym.sym == SDLK_RIGHT) move_picker(state, 1, 0);
                    else if (event.key.keysym.sym == SDLK_UP) move_picker(state, 0, -1);
                    else if (event.key.keysym.sym == SDLK_DOWN) move_picker(state, 0, 1);
                    else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                        confirm_picker(state, session); changed = true;
                    }
                } else if (state.menu_open) {
                    if (event.key.keysym.sym == SDLK_ESCAPE) state.menu_open = false;
                    else if (event.key.keysym.sym == SDLK_UP) state.menu_item = (state.menu_item + 5) % 6;
                    else if (event.key.keysym.sym == SDLK_DOWN) state.menu_item = (state.menu_item + 1) % 6;
                    else if (event.key.keysym.sym == SDLK_LEFT && (state.menu_item == 1 || state.menu_item == 2)) state.memory_slot = (state.memory_slot + 7) % 8;
                    else if (event.key.keysym.sym == SDLK_RIGHT && (state.menu_item == 1 || state.menu_item == 2)) state.memory_slot = (state.memory_slot + 1) % 8;
                    else if (event.key.keysym.sym == SDLK_RETURN) { execute_quick_menu(session, state); changed = true; }
                } else if (state.help_open) {
                    if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_h) state.help_open = false;
                } else {
                    switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: go_back(state); break;
                    case SDLK_LEFT: changed = handle_dpad(session, state, -1, 0, now) || changed; break;
                    case SDLK_RIGHT: changed = handle_dpad(session, state, 1, 0, now) || changed; break;
                    case SDLK_UP: changed = handle_dpad(session, state, 0, -1, now) || changed; break;
                    case SDLK_DOWN: changed = handle_dpad(session, state, 0, 1, now) || changed; break;
                    case SDLK_RETURN: activate_current(session, state); changed = true; break;
                    case SDLK_TAB: change_page(state, 1); break;
                    case SDLK_x: cycle_focus(state); break;
                    case SDLK_h: state.help_open = true; break;
                    case SDLK_m: state.menu_open = true; break;
                    case SDLK_f: toggle_fade(session, state); changed = true; break;
                    case SDLK_k: audio.graph.panic(); state.kill_flash_until = now + 700U; break;
                    default: break;
                    }
                }
            } else if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT ||
                    event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_DOWN) state.held_direction = 0;
            } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START) state.start_held = true;
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) state.select_held = true;
                if (state.start_held && state.select_held) { running = false; continue; }

                if (state.picker != Picker::none) {
                    if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) move_picker(state, -1, 0);
                    else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) move_picker(state, 1, 0);
                    else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) move_picker(state, 0, -1);
                    else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) move_picker(state, 0, 1);
                    else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_B) { confirm_picker(state, session); changed = true; }
                    else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_A) { state.picker = Picker::none; state.picker_master = false; }
                } else if (state.menu_open) {
                    if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) state.menu_item = (state.menu_item + 5) % 6;
                    else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) state.menu_item = (state.menu_item + 1) % 6;
                    else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT && (state.menu_item == 1 || state.menu_item == 2)) state.memory_slot = (state.memory_slot + 7) % 8;
                    else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT && (state.menu_item == 1 || state.menu_item == 2)) state.memory_slot = (state.memory_slot + 1) % 8;
                    else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_B) { execute_quick_menu(session, state); changed = true; }
                    else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_A) state.menu_open = false;
                } else if (state.help_open) {
                    if (event.cbutton.button == SDL_CONTROLLER_BUTTON_X || event.cbutton.button == SDL_CONTROLLER_BUTTON_A) state.help_open = false;
                } else {
                    switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: changed = handle_dpad(session, state, -1, 0, now) || changed; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: changed = handle_dpad(session, state, 1, 0, now) || changed; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP: changed = handle_dpad(session, state, 0, -1, now) || changed; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: changed = handle_dpad(session, state, 0, 1, now) || changed; break;
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: change_page(state, -1); break;
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: change_page(state, 1); break;
                    // TrimUI physical A is SDL B; physical B is SDL A.
                    case SDL_CONTROLLER_BUTTON_B: activate_current(session, state); changed = true; break;
                    case SDL_CONTROLLER_BUTTON_A:
                        state.back_held = true; state.back_long_action = false; state.back_held_since = now; break;
                    case SDL_CONTROLLER_BUTTON_Y: cycle_focus(state); break;
                    case SDL_CONTROLLER_BUTTON_X: state.help_open = true; break;
                    case SDL_CONTROLLER_BUTTON_BACK: toggle_fade(session, state); changed = true; break;
                    case SDL_CONTROLLER_BUTTON_START: state.menu_open = true; state.menu_item = 0; break;
                    default: break;
                    }
                }
            } else if (event.type == SDL_CONTROLLERBUTTONUP) {
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START) state.start_held = false;
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) state.select_held = false;
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT ||
                    event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT ||
                    event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP ||
                    event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) state.held_direction = 0;
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
        if (state.request_exit) running = false;
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
            if (cd::save_session(session, autosave_path, save_error)) save_pending = false;
            else { std::fprintf(stderr, "autosave: %s\n", save_error.c_str()); changed_at = now; }
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

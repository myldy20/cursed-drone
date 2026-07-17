#!/usr/bin/env python3
from pathlib import Path


def replace(path: str, old: str, new: str, expected: int = 1) -> None:
    file = Path(path)
    text = file.read_text(encoding="utf-8")
    count = text.count(old)
    if count != expected:
        raise RuntimeError(f"{path}: expected {expected} matches, found {count}: {old[:100]!r}")
    file.write_text(text.replace(old, new), encoding="utf-8")


replace(
    "src/sdl_main.cpp",
    "#include <string>\n#include <string_view>\n",
    "#include <string>\n#include <string_view>\n#include <system_error>\n",
)

handheld = '''bool handheld() noexcept {
    const char* value = std::getenv("CURSED_DRONE_HANDHELD");
    return value != nullptr && value[0] != '\\0' && value[0] != '0';
}
'''

branding_helpers = r'''bool handheld() noexcept {
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

    const Uint32 deadline = SDL_GetTicks() + (handheld() ? 1'350U : 1'000U);
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
    }
    if (texture != nullptr) SDL_DestroyTexture(texture);
    return keep_running;
}
'''
replace("src/sdl_main.cpp", handheld, branding_helpers)

outline = '''void outline(SDL_Renderer* renderer, SDL_Rect rectangle, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(renderer, &rectangle);
}
'''

mark = r'''void outline(SDL_Renderer* renderer, SDL_Rect rectangle, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(renderer, &rectangle);
}

void draw_myldy_mark(SDL_Renderer* renderer, int x, int y, int scale, SDL_Color color) {
    const int s = std::max(1, scale);
    fill(renderer, {x, y, 3 * s, 14 * s}, color);
    fill(renderer, {x + 5 * s, y, 2 * s, 14 * s}, color);
    fill(renderer, {x + 13 * s, y, 2 * s, 14 * s}, color);
    fill(renderer, {x + 17 * s, y, 3 * s, 14 * s}, color);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int offset = 0; offset < 2 * s; ++offset) {
        SDL_RenderDrawLine(renderer,
            x + 7 * s + offset, y,
            x + 10 * s + offset, y + 7 * s);
        SDL_RenderDrawLine(renderer,
            x + 10 * s + offset, y + 7 * s,
            x + 13 * s + offset, y);
    }
}
'''
replace("src/sdl_main.cpp", outline, mark)

old_setup_footer = '''    char cpu[48]{};
    std::snprintf(cpu, sizeof(cpu), "%s: %d%%", ru(session) ? "ЗАГРУЗКА DSP" : "DSP LOAD",
        state.displayed_cpu_percent);
    cd::ui::draw_text(renderer, 32, 292, cpu, state.displayed_cpu_percent < 75 ? kDim : kFxColors[0]);
    cd::ui::draw_text(renderer, 32, 316,
        handheld()
            ? (ru(session) ? "L/R: ИЗМЕНИТЬ / АВТОСОХРАНЕНИЕ" : "L/R: CHANGE / AUTOSAVE")
            : (ru(session) ? "A/D: ИЗМЕНИТЬ / НАСТРОЙКИ СОХРАНЯЮТСЯ АВТОМАТИЧЕСКИ"
                           : "A/D: CHANGE / SETTINGS ARE SAVED AUTOMATICALLY"),
        kDim);
'''

new_setup_footer = '''    SDL_SetRenderDrawColor(renderer, 75, 67, 86, 255);
    SDL_RenderDrawLine(renderer, 32, 267, 468, 267);
    draw_myldy_mark(renderer, 32, 276, 2, kInk);
    cd::ui::draw_text(renderer, 82, 277, "DEVELOPED BY MYLDY DESIGN", kInk);
    cd::ui::draw_text(renderer, 82, 294, "@MYLDY20", kDim);
    constexpr std::string_view version{"V0.8.0"};
    cd::ui::draw_text(renderer, 468 - cd::ui::text_width(version), 277, version, kDim);
    char cpu[48]{};
    std::snprintf(cpu, sizeof(cpu), "%s: %d%%", ru(session) ? "ЗАГРУЗКА DSP" : "DSP LOAD",
        state.displayed_cpu_percent);
    cd::ui::draw_text(renderer, 468 - cd::ui::text_width(cpu), 294, cpu,
        state.displayed_cpu_percent < 75 ? kDim : kFxColors[0]);
    cd::ui::draw_text(renderer, 32, 328,
        handheld()
            ? (ru(session) ? "L/R: ИЗМЕНИТЬ / АВТОСОХРАНЕНИЕ" : "L/R: CHANGE / AUTOSAVE")
            : (ru(session) ? "A/D: ИЗМЕНИТЬ / НАСТРОЙКИ СОХРАНЯЮТСЯ АВТОМАТИЧЕСКИ"
                           : "A/D: CHANGE / SETTINGS ARE SAVED AUTOMATICALLY"),
        kDim);
'''
replace("src/sdl_main.cpp", old_setup_footer, new_setup_footer)

replace(
    "src/sdl_main.cpp",
    '''    SDL_RenderSetLogicalSize(renderer, kWidth, kHeight);

    cd::Session session = cd::make_default_session();
''',
    '''    SDL_RenderSetLogicalSize(renderer, kWidth, kHeight);
    if (!show_startup_splash(renderer)) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    cd::Session session = cd::make_default_session();
''',
)

old_autosave = '''    std::filesystem::path autosave_path;
    if (char* preference_path = SDL_GetPrefPath("myldy20", "cursed-drone")) {
        autosave_path = std::filesystem::path{preference_path} / "autosave.cdrone";
        SDL_free(preference_path);
        if (std::filesystem::exists(autosave_path)) {
            std::string load_error;
            if (!cd::load_session(autosave_path, session, load_error)) {
                std::fprintf(stderr, "autosave load: %s\\n", load_error.c_str());
            }
        }
    }
'''

new_autosave = '''    std::filesystem::path autosave_path;
    std::filesystem::path legacy_autosave_path;
    if (const char* data_directory = std::getenv("CURSED_DRONE_DATA_DIR");
        data_directory != nullptr && data_directory[0] != '\\0') {
        const std::filesystem::path root{data_directory};
        autosave_path = root / "autosave.cdrone";
        legacy_autosave_path = root / "myldy20" / "cursed-drone" / "autosave.cdrone";
    } else if (char* preference_path = SDL_GetPrefPath("myldy20", "cursed-drone")) {
        autosave_path = std::filesystem::path{preference_path} / "autosave.cdrone";
        SDL_free(preference_path);
    }

    const std::filesystem::path load_path = std::filesystem::exists(autosave_path)
        ? autosave_path : legacy_autosave_path;
    if (!load_path.empty() && std::filesystem::exists(load_path)) {
        std::string load_error;
        if (!cd::load_session(load_path, session, load_error)) {
            std::fprintf(stderr, "autosave load: %s\\n", load_error.c_str());
        } else if (load_path == legacy_autosave_path) {
            std::fprintf(stderr, "autosave migration: loaded legacy path %s\\n",
                load_path.string().c_str());
        }
    }
'''
replace("src/sdl_main.cpp", old_autosave, new_autosave)

print("v0.8 branding patch applied")

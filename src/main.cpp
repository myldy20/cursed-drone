// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.
#include "cursed_drone/i18n.hpp"
#include "cursed_drone/session.hpp"
#include "cursed_drone/wav.hpp"

#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

namespace cd = cursed_drone;

namespace {

void print_help(cd::Locale locale) {
    std::cout << cd::tr(locale, cd::TextId::app_name) << " — "
              << cd::tr(locale, cd::TextId::app_subtitle) << "\n\n"
              << "cursed-drone [--lang ru|en] [--scene derelict|factory|wasteland|wet_cave|metro|nursery]\n"
              << "             [--load FILE] [--save-default FILE]\n"
              << "             [--render FILE] [--seconds N]\n\n"
              << cd::tr(locale, cd::TextId::diagnostic_warning) << '\n';
}

bool parse_seconds(const std::string& text, float& seconds) {
    try {
        std::size_t consumed = 0;
        seconds = std::stof(text, &consumed);
        return consumed == text.size() && seconds > 0.0F && seconds <= 600.0F;
    } catch (...) {
        return false;
    }
}

} // namespace

int main(int argc, char** argv) {
    cd::Session session = cd::make_default_session();
    std::filesystem::path load_path;
    std::filesystem::path save_path;
    std::filesystem::path render_path;
    float seconds = 12.0F;
    cd::SceneKind requested_scene{cd::SceneKind::derelict};
    bool has_requested_scene = false;
    bool show_help = argc == 1;
    bool has_action = false;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        auto require_value = [&](const char* option) -> const char* {
            if (index + 1 >= argc) {
                std::cerr << option << " requires a value\n";
                std::exit(2);
            }
            return argv[++index];
        };
        if (argument == "--lang") {
            const std::string value = require_value("--lang");
            if (!cd::parse_locale(value, session.locale)) {
                std::cerr << "Unsupported language: " << value << '\n';
                return 2;
            }
        } else if (argument == "--scene") {
            const std::string value = require_value("--scene");
            if (!cd::parse_scene(value, requested_scene)) {
                std::cerr << "Unsupported scene: " << value << '\n';
                return 2;
            }
            has_requested_scene = true;
            has_action = true;
        } else if (argument == "--load") {
            load_path = require_value("--load");
            has_action = true;
        } else if (argument == "--save-default") {
            save_path = require_value("--save-default");
            has_action = true;
        } else if (argument == "--render") {
            render_path = require_value("--render");
            has_action = true;
        } else if (argument == "--seconds") {
            const std::string value = require_value("--seconds");
            if (!parse_seconds(value, seconds)) {
                std::cerr << "Invalid duration: " << value << '\n';
                return 2;
            }
        } else if (argument == "--help" || argument == "-h") {
            show_help = true;
        } else {
            std::cerr << "Unknown option: " << argument << '\n';
            return 2;
        }
    }
    if (!has_action) {
        show_help = true;
    }

    std::string error;
    if (!load_path.empty() && !cd::load_session(load_path, session, error)) {
        std::cerr << cd::tr(session.locale, cd::TextId::error) << ": " << error << '\n';
        return 1;
    }
    if (has_requested_scene) {
        cd::apply_scene_recipe(session, requested_scene);
    }
    if (!save_path.empty()) {
        if (!cd::save_session(session, save_path, error)) {
            std::cerr << cd::tr(session.locale, cd::TextId::error) << ": " << error << '\n';
            return 1;
        }
        std::cout << cd::tr(session.locale, cd::TextId::saved) << ": " << save_path << '\n';
    }
    if (!render_path.empty()) {
        if (!cd::render_session_to_wav(session, render_path, seconds, error)) {
            std::cerr << cd::tr(session.locale, cd::TextId::error) << ": " << error << '\n';
            return 1;
        }
        std::cout << cd::tr(session.locale, cd::TextId::render_complete) << ": " << render_path
                  << " (" << std::fixed << std::setprecision(1) << seconds << " s)\n";
    }
    if (show_help) {
        print_help(session.locale);
    }
    return 0;
}

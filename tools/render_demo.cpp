// SPDX-License-Identifier: GPL-3.0-or-later
#include "cursed_drone/i18n.hpp"
#include "cursed_drone/session.hpp"
#include "cursed_drone/wav.hpp"

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    const std::filesystem::path path = argc > 1 ? argv[1] : "cursed-drone-demo.wav";
    float seconds = 12.0F;
    if (argc > 2) {
        try {
            seconds = std::stof(argv[2]);
        } catch (...) {
            std::cerr << "Invalid duration\n";
            return 2;
        }
    }
    auto session = cursed_drone::make_default_session();
    std::string error;
    if (!cursed_drone::render_session_to_wav(session, path, seconds, error)) {
        std::cerr << error << '\n';
        return 1;
    }
    std::cout << cursed_drone::tr(session.locale, cursed_drone::TextId::render_complete)
              << ": " << path << '\n';
    return 0;
}

// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
#include "cursed_drone/scala.hpp"
#include "cursed_drone/session.hpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace cd = cursed_drone;

namespace {

bool require(bool condition, const char* message) {
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}

std::filesystem::path test_root() {
    const auto stamp = std::chrono::steady_clock::now()
        .time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
        ("cursed-drone-hardening-" + std::to_string(stamp));
}

bool test_sanitize_and_save(const std::filesystem::path& root) {
    cd::Session session = cd::make_default_session();
    session.tempo_bpm = std::numeric_limits<float>::quiet_NaN();
    session.master_level = std::numeric_limits<float>::infinity();
    session.slots[0].timbre = -40.0F;
    session.slots[0].level = 12.0F;
    session.slots[0].pan = std::numeric_limits<float>::quiet_NaN();
    session.slots[0].effects[0].feedback = 99.0F;
    session.slots[0].modulators[0].rate_hz =
        std::numeric_limits<float>::infinity();

    const auto path = root / "sanitized.cdrone";
    std::string error;
    if (!require(cd::save_session(session, path, error),
            "sanitized session should save")) {
        std::cerr << error << '\n';
        return false;
    }

    std::ifstream input(path);
    std::ostringstream text;
    text << input.rdbuf();
    const std::string serialized = text.str();
    bool ok = true;
    ok &= require(serialized.find("nan") == std::string::npos,
        "saved session must not contain nan");
    ok &= require(serialized.find("inf") == std::string::npos,
        "saved session must not contain inf");

    cd::Session loaded{};
    error.clear();
    ok &= require(cd::load_session(path, loaded, error),
        "sanitized session should load");
    ok &= require(std::isfinite(loaded.tempo_bpm),
        "tempo must be finite after load");
    ok &= require(loaded.master_level >= 0.0F && loaded.master_level <= 1.0F,
        "master level must be clamped");
    ok &= require(loaded.slots[0].timbre == 0.0F,
        "actor timbre must be clamped");
    ok &= require(loaded.slots[0].level == 1.5F,
        "actor level must be clamped");
    ok &= require(std::isfinite(loaded.slots[0].pan),
        "actor pan must be finite");
    ok &= require(loaded.slots[0].effects[0].feedback == 1.0F,
        "actor effect feedback must be clamped");
    ok &= require(loaded.slots[0].modulators[0].rate_hz <= 40.0F,
        "modulator rate must be clamped");
    return ok;
}

bool test_untrusted_text_sanitization(const std::filesystem::path& root) {
    const auto path = root / "untrusted.cdrone";
    std::ofstream output(path);
    output << "cursed_drone_session=12\n"
           << "tempo_bpm=nan\n"
           << "master_level=inf\n"
           << "slot.0.timbre=-100\n"
           << "slot.0.color=100\n"
           << "slot.0.pan=nan\n"
           << "slot.0.effect.0.feedback=inf\n"
           << "slot.0.mod.0.rate_hz=inf\n";
    output.close();

    cd::Session loaded{};
    std::string error;
    bool ok = require(cd::load_session(path, loaded, error),
        "legacy parser output should be sanitized");
    ok &= require(std::isfinite(loaded.tempo_bpm),
        "text nan tempo must be replaced");
    ok &= require(std::isfinite(loaded.master_level),
        "text inf master level must be replaced");
    ok &= require(loaded.slots[0].timbre == 0.0F,
        "text timbre must be clamped");
    ok &= require(loaded.slots[0].color == 1.0F,
        "text color must be clamped");
    ok &= require(std::isfinite(loaded.slots[0].pan),
        "text nan pan must be replaced");
    ok &= require(loaded.slots[0].effects[0].feedback <= 1.0F,
        "text effect feedback must be clamped");
    ok &= require(loaded.slots[0].modulators[0].rate_hz <= 40.0F,
        "text modulator rate must be clamped");
    return ok;
}

bool test_backup_recovery(const std::filesystem::path& root) {
    const auto path = root / "recover.cdrone";
    std::string error;

    cd::Session first = cd::make_default_session();
    first.master_level = 0.25F;
    if (!require(cd::save_session(first, path, error),
            "first recovery session should save")) return false;

    cd::Session second = first;
    second.master_level = 0.75F;
    if (!require(cd::save_session(second, path, error),
            "second recovery session should save")) return false;

    std::ofstream corrupt(path, std::ios::trunc);
    corrupt << "not a session\n";
    corrupt.close();

    cd::Session recovered{};
    error.clear();
    bool ok = require(cd::load_session(path, recovered, error),
        "backup should recover a corrupt primary session");
    ok &= require(std::abs(recovered.master_level - 0.25F) < 0.0001F,
        "backup should contain the previous complete session");
    ok &= require(error.find("recovered from backup") != std::string::npos,
        "backup recovery should remain diagnosable");
    return ok;
}

bool test_control_rate_quantizer() {
    cd::ScalaTuning tuning{};
    cd::set_equal_temperament(tuning);

    const float initial = cd::quantize_frequency(55.0F, tuning);
    bool ok = require(std::isfinite(initial),
        "initial quantized frequency must be finite");
    for (unsigned index = 0; index < 15U; ++index) {
        ok &= require(cd::quantize_frequency(110.0F, tuning) == initial,
            "quantized pitch should remain stable inside a control block");
    }
    const float updated = cd::quantize_frequency(110.0F, tuning);
    ok &= require(std::abs(updated - initial) > 1.0F,
        "quantized pitch should update at the next control boundary");

    tuning.enabled = false;
    const float bypassed = cd::quantize_frequency(123.45F, tuning);
    ok &= require(std::abs(bypassed - 123.45F) < 0.001F,
        "disabled tuning should bypass the control-rate cache");
    return ok;
}

} // namespace

int main() {
    const auto root = test_root();
    std::error_code filesystem_error;
    std::filesystem::create_directories(root, filesystem_error);
    if (filesystem_error) {
        std::cerr << "cannot create test directory: "
                  << filesystem_error.message() << '\n';
        return 1;
    }

    bool ok = true;
    ok &= test_sanitize_and_save(root);
    ok &= test_untrusted_text_sanitization(root);
    ok &= test_backup_recovery(root);
    ok &= test_control_rate_quantizer();

    std::filesystem::remove_all(root, filesystem_error);
    return ok ? 0 : 1;
}

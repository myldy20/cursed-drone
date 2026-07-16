// SPDX-License-Identifier: GPL-3.0-or-later
#include "cursed_drone/audio.hpp"
#include "cursed_drone/i18n.hpp"
#include "cursed_drone/session.hpp"

#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace cd = cursed_drone;

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void test_i18n() {
    constexpr int count = static_cast<int>(cd::TextId::error) + 1;
    for (int index = 0; index < count; ++index) {
        const auto id = static_cast<cd::TextId>(index);
        expect(!cd::tr(cd::Locale::ru, id).empty(), "Russian string is missing");
        expect(!cd::tr(cd::Locale::en, id).empty(), "English string is missing");
    }
    expect(cd::tr(cd::Locale::ru, cd::TextId::app_name) != cd::tr(cd::Locale::en, cd::TextId::app_name),
        "localized app names should differ");
}

void test_queue() {
    cd::SpscQueue<int, 4> queue;
    expect(queue.push(1), "queue push 1");
    expect(queue.push(2), "queue push 2");
    expect(queue.push(3), "queue push 3");
    expect(!queue.push(4), "bounded queue must report full");
    int value = 0;
    expect(queue.pop(value) && value == 1, "queue preserves order 1");
    expect(queue.pop(value) && value == 2, "queue preserves order 2");
    expect(queue.pop(value) && value == 3, "queue preserves order 3");
    expect(!queue.pop(value), "queue must report empty");
}

void test_audio() {
    cd::AudioGraph graph;
    graph.prepare({48'000.0F, 256U}, cd::make_default_session());
    std::array<cd::StereoFrame, 256> block{};
    float peak = 0.0F;
    double energy = 0.0;
    for (int iteration = 0; iteration < 200; ++iteration) {
        graph.process(block);
        for (const auto frame : block) {
            expect(std::isfinite(frame.left) && std::isfinite(frame.right), "audio must remain finite");
            peak = std::max(peak, std::max(std::abs(frame.left), std::abs(frame.right)));
            energy += static_cast<double>(frame.left * frame.left + frame.right * frame.right);
        }
    }
    expect(energy > 1.0, "default patch should produce sound");
    expect(peak <= 1.0001F, "master limiter should contain output");
    const auto telemetry = graph.telemetry();
    expect(telemetry.master_rms > 0.0F, "master telemetry should react to audio");
    expect(telemetry.slot_rms[0] > 0.0F, "slot telemetry should react to audio");

    graph.panic();
    graph.process(block);
    float panic_peak = 0.0F;
    for (const auto frame : block) {
        panic_peak = std::max(panic_peak, std::max(std::abs(frame.left), std::abs(frame.right)));
    }
    expect(panic_peak == 0.0F, "panic should clear output and effect tails");
    expect(graph.telemetry().master_peak == 0.0F, "panic should clear telemetry");
}

void test_session_roundtrip() {
    const auto path = std::filesystem::temp_directory_path() / "cursed-drone-test.cdrone";
    auto original = cd::make_default_session();
    original.locale = cd::Locale::en;
    original.slots[2].effects[1].amount = 0.731F;
    original.performance.texture = 0.812F;
    original.performance.pulse = 0.643F;
    original.performance.chaos = 0.522F;
    original.performance.space = 0.407F;
    original.performance.fade = 0.381F;
    std::string error;
    expect(cd::save_session(original, path, error), "session should save");
    cd::Session loaded{};
    expect(cd::load_session(path, loaded, error), "session should load");
    expect(loaded.locale == cd::Locale::en, "locale should roundtrip");
    expect(std::abs(loaded.slots[2].effects[1].amount - 0.731F) < 0.0001F, "effect should roundtrip");
    expect(std::abs(loaded.performance.texture - 0.812F) < 0.0001F, "texture macro should roundtrip");
    expect(std::abs(loaded.performance.pulse - 0.643F) < 0.0001F, "pulse macro should roundtrip");
    expect(std::abs(loaded.performance.chaos - 0.522F) < 0.0001F, "chaos macro should roundtrip");
    expect(std::abs(loaded.performance.space - 0.407F) < 0.0001F, "space macro should roundtrip");
    expect(std::abs(loaded.performance.fade - 0.381F) < 0.0001F, "fade macro should roundtrip");
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
}

} // namespace

int main() {
    test_i18n();
    test_queue();
    test_audio();
    test_session_roundtrip();
    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "All Cursed Drone tests passed\n";
    return 0;
}

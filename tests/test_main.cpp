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
    cd::SceneKind scene{};
    expect(cd::parse_scene("factory", scene) && scene == cd::SceneKind::factory,
        "scene names should parse");
    expect(!cd::parse_scene("buzz", scene), "unknown scene names should fail");
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
    auto default_session = cd::make_default_session();
    expect(default_session.locale == cd::Locale::en, "English should be the default language");
    expect(default_session.scene == cd::SceneKind::derelict, "the derelict landscape should be the default");
    expect(default_session.slots[0].engine == cd::EngineKind::derelict_bed, "slot 1 should be the room bed");
    expect(default_session.slots[1].engine == cd::EngineKind::footsteps, "slot 2 should be footsteps");
    expect(default_session.slots[2].engine == cd::EngineKind::door, "slot 3 should be the door gesture");
    expect(default_session.slots[3].engine == cd::EngineKind::pipe, "slot 4 should be the pipe layer");
    constexpr std::array scenes{
        cd::SceneKind::derelict, cd::SceneKind::factory, cd::SceneKind::wasteland};
    for (const auto scene : scenes) {
        auto landscape = default_session;
        cd::apply_scene_recipe(landscape, scene);
        for (std::size_t active = 0; active < cd::kSlotCount; ++active) {
            auto solo = landscape;
            for (std::size_t slot = 0; slot < cd::kSlotCount; ++slot) {
                solo.slots[slot].enabled = slot == active;
                for (auto& effect : solo.slots[slot].effects) {
                    effect.kind = cd::EffectKind::bypass;
                }
            }
            cd::AudioGraph solo_graph;
            solo_graph.prepare({48'000.0F, 256U}, solo);
            std::array<cd::StereoFrame, 256> solo_block{};
            double solo_energy = 0.0;
            for (int block_index = 0; block_index < 750; ++block_index) {
                solo_graph.process({solo_block.data(), solo_block.size()});
                for (const auto frame : solo_block) {
                    solo_energy += static_cast<double>(frame.left * frame.left + frame.right * frame.right);
                }
            }
            if (solo_energy <= 0.01) {
                std::cerr << "scene " << static_cast<int>(scene) << " actor " << active
                          << " solo energy: " << solo_energy << '\n';
            }
            expect(solo_energy > 0.01, "every landscape actor should produce a non-silent solo output");
        }
    }
    cd::AudioGraph graph;
    graph.prepare({48'000.0F, 256U}, default_session);
    std::array<cd::StereoFrame, 256> block{};
    float peak = 0.0F;
    double energy = 0.0;
    for (int iteration = 0; iteration < 200; ++iteration) {
        graph.process({block.data(), block.size()});
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
    float scope_peak = 0.0F;
    for (const float sample : telemetry.master_scope) {
        scope_peak = std::max(scope_peak, std::abs(sample));
    }
    expect(scope_peak > 0.0F, "master scope should contain real audio samples");

    graph.panic();
    graph.process({block.data(), block.size()});
    float panic_peak = 0.0F;
    for (const auto frame : block) {
        panic_peak = std::max(panic_peak, std::max(std::abs(frame.left), std::abs(frame.right)));
    }
    expect(panic_peak == 0.0F, "panic should clear output and effect tails");
    expect(graph.telemetry().master_peak == 0.0F, "panic should clear telemetry");

    auto restrained = default_session;
    cd::apply_scene_recipe(restrained, cd::SceneKind::factory);
    restrained.performance.texture = 0.05F;
    restrained.performance.pulse = 0.05F;
    restrained.performance.chaos = 0.05F;
    restrained.performance.space = 0.05F;
    restrained.performance.events = 0.05F;
    auto extreme = restrained;
    extreme.performance.texture = 0.92F;
    extreme.performance.pulse = 0.88F;
    extreme.performance.chaos = 0.84F;
    extreme.performance.space = 0.90F;
    extreme.performance.events = 0.86F;
    cd::AudioGraph restrained_graph;
    cd::AudioGraph extreme_graph;
    restrained_graph.prepare({48'000.0F, 256U}, restrained);
    extreme_graph.prepare({48'000.0F, 256U}, extreme);
    std::array<cd::StereoFrame, 256> restrained_block{};
    std::array<cd::StereoFrame, 256> extreme_block{};
    double macro_difference = 0.0;
    for (int iteration = 0; iteration < 500; ++iteration) {
        restrained_graph.process({restrained_block.data(), restrained_block.size()});
        extreme_graph.process({extreme_block.data(), extreme_block.size()});
        for (std::size_t index = 0; index < restrained_block.size(); ++index) {
            macro_difference += std::abs(static_cast<double>(
                restrained_block[index].left - extreme_block[index].left));
            macro_difference += std::abs(static_cast<double>(
                restrained_block[index].right - extreme_block[index].right));
        }
    }
    expect(macro_difference > 100.0, "scene macros should produce a materially different waveform");
}

void test_session_roundtrip() {
    const auto path = std::filesystem::temp_directory_path() / "cursed-drone-test.cdrone";
    auto original = cd::make_default_session();
    original.locale = cd::Locale::en;
    cd::apply_scene_recipe(original, cd::SceneKind::factory);
    original.slots[2].effects[1].amount = 0.731F;
    original.performance.texture = 0.812F;
    original.performance.pulse = 0.643F;
    original.performance.chaos = 0.522F;
    original.performance.space = 0.407F;
    original.performance.events = 0.467F;
    original.performance.fade = 0.381F;
    original.fade_in_seconds = 2.75F;
    original.fade_out_seconds = 8.25F;
    original.scene_modified = true;
    std::string error;
    expect(cd::save_session(original, path, error), "session should save");
    cd::Session loaded{};
    expect(cd::load_session(path, loaded, error), "session should load");
    expect(loaded.locale == cd::Locale::en, "locale should roundtrip");
    expect(loaded.scene == cd::SceneKind::factory, "scene should roundtrip");
    expect(loaded.schema_version == 5, "session should upgrade to schema 5");
    expect(loaded.scene_modified, "scene modification state should roundtrip");
    expect(std::abs(loaded.slots[2].effects[1].amount - 0.731F) < 0.0001F, "effect should roundtrip");
    expect(std::abs(loaded.performance.texture - 0.812F) < 0.0001F, "texture macro should roundtrip");
    expect(std::abs(loaded.performance.pulse - 0.643F) < 0.0001F, "pulse macro should roundtrip");
    expect(std::abs(loaded.performance.chaos - 0.522F) < 0.0001F, "chaos macro should roundtrip");
    expect(std::abs(loaded.performance.space - 0.407F) < 0.0001F, "space macro should roundtrip");
    expect(std::abs(loaded.performance.events - 0.467F) < 0.0001F, "events macro should roundtrip");
    expect(std::abs(loaded.performance.fade - 0.381F) < 0.0001F, "fade macro should roundtrip");
    expect(std::abs(loaded.fade_in_seconds - 2.75F) < 0.0001F, "fade-in time should roundtrip");
    expect(std::abs(loaded.fade_out_seconds - 8.25F) < 0.0001F, "fade-out time should roundtrip");
    cd::apply_scene_recipe(loaded, cd::SceneKind::wasteland);
    expect(!loaded.scene_modified, "loading a landscape recipe should clear the modified marker");
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

// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.
#include "cursed_drone/audio.hpp"
#include "cursed_drone/i18n.hpp"
#include "cursed_drone/scala.hpp"
#include "cursed_drone/session.hpp"

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
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
        cd::SceneKind::derelict, cd::SceneKind::factory, cd::SceneKind::wasteland,
        cd::SceneKind::wet_cave, cd::SceneKind::metro, cd::SceneKind::nursery,
        cd::SceneKind::bunker, cd::SceneKind::power_grid, cd::SceneKind::deep_water,
        cd::SceneKind::ash_field};
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

    const float density = cd::effective_event_density(0.50F, 0.44F);
    expect(cd::event_rate_hz(45.0F, density, 0.25F) > 0.0F,
        "event rate should be positive");
    expect(cd::event_max_wait_seconds(45.0F, density, 0.25F) > 0.0F,
        "event maximum wait should be finite and positive");
    expect(cd::supports_manual_trigger(cd::EngineKind::door),
        "door should support manual triggering");
    expect(!cd::supports_manual_trigger(cd::EngineKind::wind),
        "continuous wind should not pretend to support manual triggering");

    auto trigger_session = default_session;
    cd::apply_scene_recipe(trigger_session, cd::SceneKind::derelict);
    for (std::size_t slot = 1; slot < cd::kSlotCount; ++slot) trigger_session.slots[slot].enabled = false;
    trigger_session.slots[0] = trigger_session.slots[2];
    trigger_session.slots[0].enabled = true;
    for (auto& effect : trigger_session.slots[0].effects) effect.kind = cd::EffectKind::bypass;
    cd::AudioGraph trigger_graph;
    trigger_graph.prepare({48'000.0F, 256U}, trigger_session);
    trigger_graph.trigger_slot(0U);
    std::array<cd::StereoFrame, 256> trigger_block{};
    trigger_graph.process({trigger_block.data(), trigger_block.size()});
    expect(trigger_graph.telemetry().slot_event[0] > 0.5F,
        "manual actor trigger should publish visible event telemetry");

    graph.panic();
    graph.process({block.data(), block.size()});
    float panic_peak = 0.0F;
    for (const auto frame : block) {
        panic_peak = std::max(panic_peak, std::max(std::abs(frame.left), std::abs(frame.right)));
    }
    expect(panic_peak == 0.0F, "panic should clear output and effect tails");
    expect(graph.telemetry().master_peak == 0.0F, "panic should clear telemetry");

    auto mute_session = default_session;
    for (std::size_t slot = 1; slot < cd::kSlotCount; ++slot) {
        mute_session.slots[slot].enabled = false;
    }
    mute_session.slots[0].effects[0] = {
        cd::EffectKind::delay, 0.72F, 0.45F, 0.82F};
    cd::AudioGraph mute_graph;
    mute_graph.prepare({48'000.0F, 256U}, mute_session);
    std::array<cd::StereoFrame, 256> mute_block{};
    double active_energy = 0.0;
    for (int iteration = 0; iteration < 120; ++iteration) {
        mute_graph.process({mute_block.data(), mute_block.size()});
        for (const auto frame : mute_block) {
            active_energy += static_cast<double>(frame.left * frame.left + frame.right * frame.right);
        }
    }
    expect(active_energy > 0.01, "the actor used by the mute test should initially produce sound");
    mute_session.slots[0].enabled = false;
    expect(mute_graph.submit_session(mute_session), "mute session should enter the audio queue");
    float muted_peak = 0.0F;
    for (int iteration = 0; iteration < 24; ++iteration) {
        mute_graph.process({mute_block.data(), mute_block.size()});
        if (iteration < 12) continue;
        for (const auto frame : mute_block) {
            muted_peak = std::max(muted_peak, std::max(std::abs(frame.left), std::abs(frame.right)));
        }
    }
    expect(muted_peak < 0.0001F,
        "muting the last actor should silence its oscillator and existing actor-FX tail");

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

    const auto render_master_rms = [](float master_level) {
        cd::Session controlled{};
        controlled.master_level = master_level;
        controlled.performance = {};
        controlled.performance.fade = 1.0F;
        controlled.slots[0].engine = cd::EngineKind::macro;
        controlled.slots[0].frequency_hz = 82.41F;
        controlled.slots[0].level = 1.0F;
        controlled.slots[0].timbre = 0.0F;
        controlled.slots[0].texture = 0.0F;
        for (std::size_t index = 1; index < cd::kSlotCount; ++index) controlled.slots[index].enabled = false;
        cd::AudioGraph controlled_graph;
        controlled_graph.prepare({48'000.0F, 256U}, controlled);
        std::array<cd::StereoFrame, 256> controlled_block{};
        double controlled_energy = 0.0;
        std::size_t samples = 0U;
        for (int iteration = 0; iteration < 240; ++iteration) {
            controlled_graph.process({controlled_block.data(), controlled_block.size()});
            if (iteration < 40) continue;
            for (const auto frame : controlled_block) {
                controlled_energy += static_cast<double>(
                    0.5F * (frame.left * frame.left + frame.right * frame.right));
                ++samples;
            }
        }
        return std::sqrt(controlled_energy / static_cast<double>(samples));
    };
    const double rms_10 = render_master_rms(0.10F);
    const double rms_50 = render_master_rms(0.50F);
    const double rms_100 = render_master_rms(1.0F);
    expect(rms_10 < rms_50 * 0.24, "10% master should be much quieter than 50%");
    expect(rms_50 < rms_100 * 0.56, "50% master should remain quieter than 100% after limiting");
    expect(rms_50 > rms_100 * 0.44, "master control should be approximately linear after limiting");

    constexpr std::array effects{
        cd::EffectKind::bypass, cd::EffectKind::drive, cd::EffectKind::lowpass,
        cd::EffectKind::highpass, cd::EffectKind::tremolo, cd::EffectKind::delay,
        cd::EffectKind::crusher, cd::EffectKind::wavefolder, cd::EffectKind::ringmod,
        cd::EffectKind::comb, cd::EffectKind::chorus, cd::EffectKind::flanger,
        cd::EffectKind::phaser, cd::EffectKind::diffuser, cd::EffectKind::ahdr,
        cd::EffectKind::tape_void, cd::EffectKind::black_hole,
        cd::EffectKind::ritual_gate, cd::EffectKind::rust_cloud, cd::EffectKind::deep_sea,
        cd::EffectKind::granular_reverse};
    for (const auto effect : effects) {
        auto effected = default_session;
        for (std::size_t slot = 1; slot < cd::kSlotCount; ++slot) effected.slots[slot].enabled = false;
        for (auto& slot_effect : effected.slots[0].effects) slot_effect.kind = cd::EffectKind::bypass;
        effected.slots[0].effects[0] = {effect, 0.72F, 0.57F, 0.61F};
        cd::AudioGraph effect_graph;
        effect_graph.prepare({48'000.0F, 256U}, effected);
        std::array<cd::StereoFrame, 256> effect_block{};
        double effect_energy = 0.0;
        for (int iteration = 0; iteration < 300; ++iteration) {
            effect_graph.process({effect_block.data(), effect_block.size()});
            for (const auto frame : effect_block) {
                expect(std::isfinite(frame.left) && std::isfinite(frame.right),
                    "every effect should keep the audio finite");
                effect_energy += static_cast<double>(frame.left * frame.left + frame.right * frame.right);
            }
        }
        expect(effect_energy > 0.001, "every effect should preserve or produce audible output");
    }

    auto musical = default_session;
    for (std::size_t slot = 1; slot < cd::kSlotCount; ++slot) musical.slots[slot].enabled = false;
    musical.slots[0].engine = cd::EngineKind::plaits;
    musical.slots[0].plaits_model = cd::PlaitsModel::swarm;
    musical.slots[0].plaits_output = cd::PlaitsOutputMode::stereo;
    musical.slots[0].frequency_hz = 55.0F;
    musical.slots[0].level = 0.45F;
    musical.slots[0].euclidean = {true, 16, 5, 2, 1.0F};
    cd::AudioGraph musical_graph;
    musical_graph.prepare({48'000.0F, 256U}, musical);
    std::array<cd::StereoFrame, 256> musical_block{};
    double musical_energy = 0.0;
    double stereo_difference = 0.0;
    for (int iteration = 0; iteration < 400; ++iteration) {
        musical_graph.process({musical_block.data(), musical_block.size()});
        for (const auto frame : musical_block) {
            expect(std::isfinite(frame.left) && std::isfinite(frame.right),
                "macro actor output must remain finite");
            musical_energy += static_cast<double>(frame.left * frame.left + frame.right * frame.right);
            stereo_difference += std::abs(static_cast<double>(frame.left - frame.right));
        }
    }
    expect(musical_energy > 0.01, "macro actor should produce audible output");
    expect(stereo_difference > 0.01, "stereo macro output should preserve distinct MAIN and AUX signals");

    auto morph_a = default_session;
    morph_a.performance.morph_target = cd::SceneKind::deep_water;
    morph_a.performance.morph = 0.0F;
    auto morph_b = morph_a;
    morph_b.performance.morph = 1.0F;
    cd::AudioGraph morph_a_graph;
    cd::AudioGraph morph_b_graph;
    morph_a_graph.prepare({48'000.0F, 256U}, morph_a);
    morph_b_graph.prepare({48'000.0F, 256U}, morph_b);
    std::array<cd::StereoFrame, 256> morph_a_block{};
    std::array<cd::StereoFrame, 256> morph_b_block{};
    double morph_difference = 0.0;
    for (int iteration = 0; iteration < 240; ++iteration) {
        morph_a_graph.process({morph_a_block.data(), morph_a_block.size()});
        morph_b_graph.process({morph_b_block.data(), morph_b_block.size()});
        for (std::size_t sample = 0; sample < morph_a_block.size(); ++sample) {
            morph_difference += std::abs(static_cast<double>(morph_a_block[sample].left - morph_b_block[sample].left));
            morph_difference += std::abs(static_cast<double>(morph_a_block[sample].right - morph_b_block[sample].right));
        }
    }
    expect(morph_difference > 10.0, "landscape morph should materially change the rendered sound");
}

void test_scala() {
    const auto path = std::filesystem::temp_directory_path() / "cursed-drone-test.scl";
    {
        std::ofstream output(path);
        output << "! test scale\nJust minor test\n7\n16/15\n6/5\n4/3\n3/2\n8/5\n9/5\n2/1\n";
    }
    cd::ParsedScale scale{};
    std::string error;
    expect(cd::parse_scala_file(path, scale, error), "Scala file should parse");
    expect(scale.cents.size() == 7U, "Scala parser should keep declared degrees");
    cd::ScalaTuning tuning{};
    cd::apply_scale(tuning, scale);
    const float quantized = cd::quantize_frequency(61.0F, tuning);
    expect(std::isfinite(quantized) && quantized > 0.0F, "Scala quantisation should produce a valid frequency");
    expect(std::abs(quantized - 61.0F) > 0.001F, "Scala quantisation should move an off-scale frequency");
    std::filesystem::remove(path);
}

void test_session_roundtrip() {
    const auto path = std::filesystem::temp_directory_path() / "cursed-drone-test.cdrone";
    auto original = cd::make_default_session();
    original.locale = cd::Locale::en;
    cd::apply_scene_recipe(original, cd::SceneKind::nursery);
    original.slots[2].effects[1].kind = cd::EffectKind::ringmod;
    original.slots[2].effects[1].amount = 0.731F;
    original.performance.texture = 0.812F;
    original.performance.pulse = 0.643F;
    original.performance.chaos = 0.522F;
    original.performance.space = 0.407F;
    original.performance.events = 0.467F;
    original.performance.fade = 0.381F;
    original.performance.morph_target = cd::SceneKind::ash_field;
    original.performance.morph = 0.44F;
    original.master_effects[0] = {cd::EffectKind::delay, 0.42F, 0.77F, 0.68F};
    original.master_effects[1] = {cd::EffectKind::diffuser, 0.31F, 0.54F, 0.59F};
    original.slots[0].frequency_hz = 8.0F;
    original.slots[1].engine = cd::EngineKind::plaits;
    original.slots[1].plaits_model = cd::PlaitsModel::wavetable;
    original.slots[1].plaits_output = cd::PlaitsOutputMode::aux;
    original.slots[1].euclidean = {true, 13, 5, 3, 0.73F};
    original.slots[1].event_density = 0.731F;
    original.slots[1].modulators[2].depth = -0.42F;
    original.slots[1].modulators[2].rate_mod_source = 0;
    original.slots[1].modulators[2].rate_mod_amount = -0.31F;
    original.fade_in_seconds = 2.75F;
    original.fade_out_seconds = 8.25F;
    original.scene_modified = true;
    std::string error;
    expect(cd::save_session(original, path, error), "session should save");
    cd::Session loaded{};
    expect(cd::load_session(path, loaded, error), "session should load");
    expect(loaded.locale == cd::Locale::en, "locale should roundtrip");
    expect(loaded.scene == cd::SceneKind::nursery, "scene should roundtrip");
    expect(loaded.schema_version == 11, "session should upgrade to schema 11");
    expect(loaded.scene_modified, "scene modification state should roundtrip");
    expect(std::abs(loaded.slots[2].effects[1].amount - 0.731F) < 0.0001F, "effect should roundtrip");
    expect(loaded.slots[2].effects[1].kind == cd::EffectKind::ringmod,
        "new effect kinds should roundtrip");
    expect(std::abs(loaded.performance.texture - 0.812F) < 0.0001F, "texture macro should roundtrip");
    expect(std::abs(loaded.performance.pulse - 0.643F) < 0.0001F, "pulse macro should roundtrip");
    expect(std::abs(loaded.performance.chaos - 0.522F) < 0.0001F, "chaos macro should roundtrip");
    expect(std::abs(loaded.performance.space - 0.407F) < 0.0001F, "space macro should roundtrip");
    expect(std::abs(loaded.performance.events - 0.467F) < 0.0001F, "events macro should roundtrip");
    expect(std::abs(loaded.performance.fade - 0.381F) < 0.0001F, "performance fade should roundtrip");
    expect(loaded.performance.morph_target == cd::SceneKind::ash_field, "morph target should roundtrip");
    expect(std::abs(loaded.performance.morph - 0.44F) < 0.0001F, "morph amount should roundtrip");
    expect(loaded.master_effects[0].kind == cd::EffectKind::delay, "master delay should roundtrip");
    expect(std::abs(loaded.master_effects[0].feedback - 0.68F) < 0.0001F, "master delay feedback should roundtrip");
    expect(loaded.master_effects[1].kind == cd::EffectKind::diffuser, "master diffuser should roundtrip");
    expect(std::abs(loaded.slots[0].frequency_hz - 20.0F) < 0.0001F,
        "legacy sub-audible actor frequency should clamp to 20 Hz");
    expect(loaded.slots[1].engine == cd::EngineKind::plaits, "macro actor should roundtrip");
    expect(loaded.slots[1].plaits_model == cd::PlaitsModel::wavetable, "macro model should roundtrip");
    expect(loaded.slots[1].plaits_output == cd::PlaitsOutputMode::aux, "macro output should roundtrip");
    expect(loaded.slots[1].euclidean.steps == 13 && loaded.slots[1].euclidean.pulses == 5,
        "Euclidean settings should roundtrip");
    expect(std::abs(loaded.slots[1].event_density - 0.731F) < 0.0001F,
        "per-actor event density should roundtrip");
    expect(std::abs(loaded.slots[1].modulators[2].depth + 0.42F) < 0.0001F,
        "bipolar modulation depth should roundtrip");
    expect(loaded.slots[1].modulators[2].rate_mod_source == 0,
        "modulator rate source should roundtrip");
    expect(std::abs(loaded.fade_in_seconds - 2.75F) < 0.0001F, "fade-in time should roundtrip");
    expect(std::abs(loaded.fade_out_seconds - 8.25F) < 0.0001F, "fade-out time should roundtrip");
    std::filesystem::remove(path);
}

} // namespace

int main() {
    test_i18n();
    test_queue();
    test_audio();
    test_scala();
    test_session_roundtrip();
    if (failures == 0) {
        std::cout << "All tests passed\n";
        return 0;
    }
    std::cerr << failures << " test(s) failed\n";
    return 1;
}

#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text(encoding='utf-8')
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f'{path}: expected one match, found {count}: {old[:120]!r}')
    p.write_text(text.replace(old, new), encoding='utf-8')

path = 'tests/test_main.cpp'
replace_once(path,
'''#include "cursed_drone/i18n.hpp"
#include "cursed_drone/session.hpp"
''',
'''#include "cursed_drone/i18n.hpp"
#include "cursed_drone/scala.hpp"
#include "cursed_drone/session.hpp"
''')
replace_once(path,
'''        cd::EffectKind::ritual_gate, cd::EffectKind::rust_cloud, cd::EffectKind::deep_sea};''',
'''        cd::EffectKind::ritual_gate, cd::EffectKind::rust_cloud, cd::EffectKind::deep_sea,
        cd::EffectKind::granular_reverse};''')
replace_once(path,
'''        expect(effect_energy > 0.001, "every effect should preserve or produce audible output");
    }
}

void test_session_roundtrip() {''',
'''        expect(effect_energy > 0.001, "every effect should preserve or produce audible output");
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
        output << "! test scale\\nJust minor test\\n7\\n16/15\\n6/5\\n4/3\\n3/2\\n8/5\\n9/5\\n2/1\\n";
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

void test_session_roundtrip() {''')
replace_once(path,
'''    original.performance.fade = 0.381F;
    original.fade_in_seconds = 2.75F;''',
'''    original.performance.fade = 0.381F;
    original.performance.morph_target = cd::SceneKind::ash_field;
    original.performance.morph = 0.44F;
    original.slots[1].engine = cd::EngineKind::plaits;
    original.slots[1].plaits_model = cd::PlaitsModel::wavetable;
    original.slots[1].plaits_output = cd::PlaitsOutputMode::aux;
    original.slots[1].euclidean = {true, 13, 5, 3, 0.73F};
    original.slots[1].modulators[2].depth = -0.42F;
    original.slots[1].modulators[2].rate_mod_source = 0;
    original.slots[1].modulators[2].rate_mod_amount = -0.31F;
    original.fade_in_seconds = 2.75F;''')
replace_once(path,
'''    expect(std::abs(loaded.performance.fade - 0.381F) < 0.0001F, "performance fade should roundtrip");
    expect(std::abs(loaded.fade_in_seconds''',
'''    expect(std::abs(loaded.performance.fade - 0.381F) < 0.0001F, "performance fade should roundtrip");
    expect(loaded.performance.morph_target == cd::SceneKind::ash_field, "morph target should roundtrip");
    expect(std::abs(loaded.performance.morph - 0.44F) < 0.0001F, "morph amount should roundtrip");
    expect(loaded.slots[1].engine == cd::EngineKind::plaits, "macro actor should roundtrip");
    expect(loaded.slots[1].plaits_model == cd::PlaitsModel::wavetable, "macro model should roundtrip");
    expect(loaded.slots[1].plaits_output == cd::PlaitsOutputMode::aux, "macro output should roundtrip");
    expect(loaded.slots[1].euclidean.steps == 13 && loaded.slots[1].euclidean.pulses == 5,
        "Euclidean settings should roundtrip");
    expect(std::abs(loaded.slots[1].modulators[2].depth + 0.42F) < 0.0001F,
        "bipolar modulation depth should roundtrip");
    expect(loaded.slots[1].modulators[2].rate_mod_source == 0,
        "modulator rate source should roundtrip");
    expect(std::abs(loaded.fade_in_seconds''')
replace_once(path,
'''    test_audio();
    test_session_roundtrip();''',
'''    test_audio();
    test_scala();
    test_session_roundtrip();''')

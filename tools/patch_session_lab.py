#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text(encoding='utf-8')
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f'{path}: expected one match, found {count}: {old[:100]!r}')
    p.write_text(text.replace(old, new), encoding='utf-8')

path = 'src/session.cpp'
replace_once(path,
'''#include "cursed_drone/session.hpp"
''',
'''#include "cursed_drone/session.hpp"
#include "cursed_drone/scala.hpp"
''')

replace_once(path,
'''    std::pair{EngineKind::earth_rumble, std::string_view{"earth_rumble"}},
};''',
'''    std::pair{EngineKind::earth_rumble, std::string_view{"earth_rumble"}},
    std::pair{EngineKind::plaits, std::string_view{"plaits"}},
};''')

replace_once(path,
'''    std::pair{EffectKind::deep_sea, std::string_view{"deep_sea"}},
};''',
'''    std::pair{EffectKind::deep_sea, std::string_view{"deep_sea"}},
    std::pair{EffectKind::granular_reverse, std::string_view{"granular_reverse"}},
};''')

replace_once(path,
'''constexpr std::array kDestinations{
    std::pair{ModDestination::pitch, std::string_view{"pitch"}},
    std::pair{ModDestination::timbre, std::string_view{"timbre"}},
    std::pair{ModDestination::color, std::string_view{"color"}},
    std::pair{ModDestination::motion, std::string_view{"motion"}},
    std::pair{ModDestination::texture, std::string_view{"texture"}},
    std::pair{ModDestination::level, std::string_view{"level"}},
    std::pair{ModDestination::pan, std::string_view{"pan"}},
    std::pair{ModDestination::fx1, std::string_view{"fx1"}},
    std::pair{ModDestination::fx2, std::string_view{"fx2"}},
    std::pair{ModDestination::fx3, std::string_view{"fx3"}},
    std::pair{ModDestination::fx4, std::string_view{"fx4"}},
};''',
'''constexpr std::array kDestinations{
    std::pair{ModDestination::pitch, std::string_view{"pitch"}},
    std::pair{ModDestination::timbre, std::string_view{"timbre"}},
    std::pair{ModDestination::color, std::string_view{"color"}},
    std::pair{ModDestination::motion, std::string_view{"motion"}},
    std::pair{ModDestination::texture, std::string_view{"texture"}},
    std::pair{ModDestination::level, std::string_view{"level"}},
    std::pair{ModDestination::pan, std::string_view{"pan"}},
    std::pair{ModDestination::fx1, std::string_view{"fx1"}},
    std::pair{ModDestination::fx2, std::string_view{"fx2"}},
    std::pair{ModDestination::fx3, std::string_view{"fx3"}},
    std::pair{ModDestination::fx4, std::string_view{"fx4"}},
};

constexpr std::array kPlaitsModels{
    std::pair{PlaitsModel::virtual_analog_vcf, std::string_view{"virtual_analog_vcf"}},
    std::pair{PlaitsModel::phase_distortion, std::string_view{"phase_distortion"}},
    std::pair{PlaitsModel::wave_terrain, std::string_view{"wave_terrain"}},
    std::pair{PlaitsModel::string_machine, std::string_view{"string_machine"}},
    std::pair{PlaitsModel::chiptune, std::string_view{"chiptune"}},
    std::pair{PlaitsModel::virtual_analog, std::string_view{"virtual_analog"}},
    std::pair{PlaitsModel::waveshaping, std::string_view{"waveshaping"}},
    std::pair{PlaitsModel::fm, std::string_view{"fm"}},
    std::pair{PlaitsModel::grain, std::string_view{"grain"}},
    std::pair{PlaitsModel::additive, std::string_view{"additive"}},
    std::pair{PlaitsModel::wavetable, std::string_view{"wavetable"}},
    std::pair{PlaitsModel::chord, std::string_view{"chord"}},
    std::pair{PlaitsModel::swarm, std::string_view{"swarm"}},
    std::pair{PlaitsModel::noise, std::string_view{"noise"}},
    std::pair{PlaitsModel::string, std::string_view{"string"}},
    std::pair{PlaitsModel::modal, std::string_view{"modal"}},
};

constexpr std::array kPlaitsOutputs{
    std::pair{PlaitsOutputMode::main, std::string_view{"main"}},
    std::pair{PlaitsOutputMode::aux, std::string_view{"aux"}},
    std::pair{PlaitsOutputMode::mix, std::string_view{"mix"}},
    std::pair{PlaitsOutputMode::stereo, std::string_view{"stereo"}},
};''')

replace_once(path,
'''bool parse_bool(const std::unordered_map<std::string, std::string>& values, const std::string& name, bool& value) {''',
'''bool parse_int(const std::unordered_map<std::string, std::string>& values, const std::string& name, int& value) {
    const auto found = values.find(name);
    if (found == values.end()) return true;
    try {
        std::size_t consumed = 0;
        const int parsed = std::stoi(found->second, &consumed);
        if (consumed != found->second.size()) return false;
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool parse_name(
    const std::unordered_map<std::string, std::string>& values,
    const std::string& name,
    std::array<char, kScaleNameLength>& value) {
    const auto found = values.find(name);
    if (found == values.end()) return true;
    value.fill('\\0');
    const std::size_t count = std::min(value.size() - 1U, found->second.size());
    std::copy_n(found->second.data(), count, value.data());
    return true;
}

bool parse_bool(const std::unordered_map<std::string, std::string>& values, const std::string& name, bool& value) {''')

replace_once(path,
'''        slot.level = levels[index];

        slot.modulators[0]''',
'''        slot.level = levels[index];
        set_equal_temperament(slot.tuning);

        slot.modulators[0]''')

replace_once(path,
'''        slot.level = level;
        slot.pan = pan;
        for (auto& effect : slot.effects) {''',
'''        slot.level = level;
        slot.pan = pan;
        slot.plaits_model = PlaitsModel::virtual_analog;
        slot.plaits_output = PlaitsOutputMode::stereo;
        slot.euclidean = {};
        set_equal_temperament(slot.tuning);
        for (auto& effect : slot.effects) {''')

replace_once(path,
'''    output << "performance.fade=" << session.performance.fade << '\\n';
    for (std::size_t slot_index = 0; slot_index < kSlotCount; ++slot_index) {''',
'''    output << "performance.fade=" << session.performance.fade << '\\n';
    output << "performance.morph_target=" << to_string(session.performance.morph_target) << '\\n';
    output << "performance.morph=" << session.performance.morph << '\\n';
    for (std::size_t slot_index = 0; slot_index < kSlotCount; ++slot_index) {''')

replace_once(path,
'''        output << key(slot_index, "pan") << '=' << slot.pan << '\\n';
        for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {''',
'''        output << key(slot_index, "pan") << '=' << slot.pan << '\\n';
        output << key(slot_index, "plaits_model") << '=' << to_string(slot.plaits_model) << '\\n';
        output << key(slot_index, "plaits_output") << '=' << to_string(slot.plaits_output) << '\\n';
        output << key(slot_index, "tuning_enabled") << '=' << (slot.tuning.enabled ? 1 : 0) << '\\n';
        output << key(slot_index, "tuning_root") << '=' << slot.tuning.root_midi << '\\n';
        output << key(slot_index, "tuning_count") << '=' << slot.tuning.degree_count << '\\n';
        output << key(slot_index, "tuning_period") << '=' << slot.tuning.period_cents << '\\n';
        output << key(slot_index, "tuning_name") << '=' << slot.tuning.name.data() << '\\n';
        for (std::size_t degree = 0; degree < kScaleDegreeCount; ++degree) {
            output << key(slot_index, "tuning_cents_" + std::to_string(degree)) << '='
                   << slot.tuning.cents[degree] << '\\n';
        }
        output << key(slot_index, "euclidean_enabled") << '=' << (slot.euclidean.enabled ? 1 : 0) << '\\n';
        output << key(slot_index, "euclidean_steps") << '=' << slot.euclidean.steps << '\\n';
        output << key(slot_index, "euclidean_pulses") << '=' << slot.euclidean.pulses << '\\n';
        output << key(slot_index, "euclidean_rotation") << '=' << slot.euclidean.rotation << '\\n';
        output << key(slot_index, "euclidean_probability") << '=' << slot.euclidean.probability << '\\n';
        for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {''')

replace_once(path,
'''            output << mod_key(slot_index, mod_index, "offset") << '=' << mod.offset << '\\n';
        }''',
'''            output << mod_key(slot_index, mod_index, "offset") << '=' << mod.offset << '\\n';
            output << mod_key(slot_index, mod_index, "rate_mod_source") << '=' << mod.rate_mod_source << '\\n';
            output << mod_key(slot_index, mod_index, "rate_mod_amount") << '=' << mod.rate_mod_amount << '\\n';
        }''')

replace_once(path,
'''            schema->second != "4" && schema->second != "5" && schema->second != "6" &&
            schema->second != "7" && schema->second != "8")) {''',
'''            schema->second != "4" && schema->second != "5" && schema->second != "6" &&
            schema->second != "7" && schema->second != "8" && schema->second != "9")) {''')

replace_once(path,
'''        !parse_float(values, "performance.events", loaded.performance.events) ||
        !parse_float(values, "performance.fade", loaded.performance.fade)) {''',
'''        !parse_float(values, "performance.events", loaded.performance.events) ||
        !parse_float(values, "performance.fade", loaded.performance.fade) ||
        !parse_enum_value(values, "performance.morph_target", loaded.performance.morph_target, kScenes) ||
        !parse_float(values, "performance.morph", loaded.performance.morph)) {''')

replace_once(path,
'''            !parse_float(values, key(slot_index, "level"), slot.level) ||
            !parse_float(values, key(slot_index, "pan"), slot.pan)) {''',
'''            !parse_float(values, key(slot_index, "level"), slot.level) ||
            !parse_float(values, key(slot_index, "pan"), slot.pan) ||
            !parse_enum_value(values, key(slot_index, "plaits_model"), slot.plaits_model, kPlaitsModels) ||
            !parse_enum_value(values, key(slot_index, "plaits_output"), slot.plaits_output, kPlaitsOutputs) ||
            !parse_bool(values, key(slot_index, "tuning_enabled"), slot.tuning.enabled) ||
            !parse_int(values, key(slot_index, "tuning_root"), slot.tuning.root_midi) ||
            !parse_int(values, key(slot_index, "tuning_count"), slot.tuning.degree_count) ||
            !parse_float(values, key(slot_index, "tuning_period"), slot.tuning.period_cents) ||
            !parse_name(values, key(slot_index, "tuning_name"), slot.tuning.name) ||
            !parse_bool(values, key(slot_index, "euclidean_enabled"), slot.euclidean.enabled) ||
            !parse_int(values, key(slot_index, "euclidean_steps"), slot.euclidean.steps) ||
            !parse_int(values, key(slot_index, "euclidean_pulses"), slot.euclidean.pulses) ||
            !parse_int(values, key(slot_index, "euclidean_rotation"), slot.euclidean.rotation) ||
            !parse_float(values, key(slot_index, "euclidean_probability"), slot.euclidean.probability)) {''')

replace_once(path,
'''        for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {''',
'''        for (std::size_t degree = 0; degree < kScaleDegreeCount; ++degree) {
            if (!parse_float(values, key(slot_index, "tuning_cents_" + std::to_string(degree)),
                    slot.tuning.cents[degree])) {
                error = "invalid tuning in slot " + std::to_string(slot_index + 1U);
                return false;
            }
        }
        for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {''')

replace_once(path,
'''                !parse_float(values, mod_key(slot_index, mod_index, "depth"), mod.depth) ||
                !parse_float(values, mod_key(slot_index, mod_index, "offset"), mod.offset)) {''',
'''                !parse_float(values, mod_key(slot_index, mod_index, "depth"), mod.depth) ||
                !parse_float(values, mod_key(slot_index, mod_index, "offset"), mod.offset) ||
                !parse_int(values, mod_key(slot_index, mod_index, "rate_mod_source"), mod.rate_mod_source) ||
                !parse_float(values, mod_key(slot_index, mod_index, "rate_mod_amount"), mod.rate_mod_amount)) {''')

replace_once(path,
'''    loaded.performance.fade = std::clamp(loaded.performance.fade, 0.0F, 1.0F);
    if (schema->second != "4" && schema->second != "5" && schema->second != "6" &&
        schema->second != "7" && schema->second != "8") {''',
'''    loaded.performance.fade = std::clamp(loaded.performance.fade, 0.0F, 1.0F);
    loaded.performance.morph = std::clamp(loaded.performance.morph, 0.0F, 1.0F);
    for (auto& slot : loaded.slots) {
        slot.tuning.root_midi = std::clamp(slot.tuning.root_midi, 0, 127);
        slot.tuning.degree_count = std::clamp(slot.tuning.degree_count, 1, static_cast<int>(kScaleDegreeCount));
        slot.tuning.period_cents = std::clamp(slot.tuning.period_cents, 50.0F, 4800.0F);
        slot.euclidean.steps = std::clamp(slot.euclidean.steps, 1, 32);
        slot.euclidean.pulses = std::clamp(slot.euclidean.pulses, 0, slot.euclidean.steps);
        slot.euclidean.rotation = std::clamp(slot.euclidean.rotation, 0, slot.euclidean.steps - 1);
        slot.euclidean.probability = std::clamp(slot.euclidean.probability, 0.0F, 1.0F);
        for (auto& mod : slot.modulators) {
            mod.depth = std::clamp(mod.depth, -1.0F, 1.0F);
            mod.rate_mod_source = std::clamp(mod.rate_mod_source, -1, 3);
            mod.rate_mod_amount = std::clamp(mod.rate_mod_amount, -1.0F, 1.0F);
        }
    }
    if (schema->second != "4" && schema->second != "5" && schema->second != "6" &&
        schema->second != "7" && schema->second != "8" && schema->second != "9") {''')

replace_once(path, '    loaded.schema_version = 8;', '    loaded.schema_version = 9;')

replace_once(path,
'''std::string to_string(ModDestination value) { return enum_name(value, kDestinations); }

bool parse_locale''',
'''std::string to_string(ModDestination value) { return enum_name(value, kDestinations); }
std::string to_string(PlaitsModel value) { return enum_name(value, kPlaitsModels); }
std::string to_string(PlaitsOutputMode value) { return enum_name(value, kPlaitsOutputs); }

bool parse_locale''')

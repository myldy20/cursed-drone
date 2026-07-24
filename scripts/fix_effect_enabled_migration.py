#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: Path, before: str, after: str) -> None:
    source = path.read_text(encoding="utf-8")
    if before not in source:
        raise SystemExit(f"missing fragment in {path}: {before[:120]!r}")
    path.write_text(source.replace(before, after, 1), encoding="utf-8")


session = ROOT / "src/session.cpp"
replace_once(session,
'''    Session loaded = make_default_session();
    const auto locale = values.find("locale");''',
'''    const bool has_effect_enabled = schema->second == "12";
    Session loaded = make_default_session();
    const auto locale = values.find("locale");''')
replace_once(session,
'''        if (!parse_enum_value(values, master_effect_key(effect_index, "kind"), effect.kind, kEffects) ||
            !parse_bool(values, master_effect_key(effect_index, "enabled"), effect.enabled) ||
            !parse_float(values, master_effect_key(effect_index, "amount"), effect.amount) ||''',
'''        if (!parse_enum_value(values, master_effect_key(effect_index, "kind"), effect.kind, kEffects) ||
            (has_effect_enabled && !parse_bool(values,
                master_effect_key(effect_index, "enabled"), effect.enabled)) ||
            !parse_float(values, master_effect_key(effect_index, "amount"), effect.amount) ||''')
replace_once(session,
'''            if (!parse_enum_value(values, effect_key(slot_index, effect_index, "kind"), effect.kind, kEffects) ||
                !parse_bool(values, effect_key(slot_index, effect_index, "enabled"), effect.enabled) ||
                !parse_float(values, effect_key(slot_index, effect_index, "amount"), effect.amount) ||''',
'''            if (!parse_enum_value(values, effect_key(slot_index, effect_index, "kind"), effect.kind, kEffects) ||
                (has_effect_enabled && !parse_bool(values,
                    effect_key(slot_index, effect_index, "enabled"), effect.enabled)) ||
                !parse_float(values, effect_key(slot_index, effect_index, "amount"), effect.amount) ||''')

tests = ROOT / "tests/test_main.cpp"
replace_once(tests,
'''    expect(std::abs(loaded.fade_out_seconds - 8.25F) < 0.0001F, "fade-out time should roundtrip");

    auto updated = original;''',
'''    expect(std::abs(loaded.fade_out_seconds - 8.25F) < 0.0001F, "fade-out time should roundtrip");

    const auto legacy_path = std::filesystem::temp_directory_path() /
        "cursed-drone-test-schema11.cdrone";
    {
        std::ifstream input(path);
        std::ofstream output(legacy_path, std::ios::trunc);
        std::string line;
        while (std::getline(input, line)) {
            if (line.rfind("cursed_drone_session=", 0U) == 0U) {
                output << "cursed_drone_session=11\\n";
            } else if (line.find(".effect.") != std::string::npos &&
                line.find(".enabled=") != std::string::npos) {
                continue;
            } else {
                output << line << '\\n';
            }
        }
    }
    cd::Session legacy{};
    expect(cd::load_session(legacy_path, legacy, error),
        "schema 11 session without effect enabled fields should migrate");
    expect(legacy.schema_version == 12,
        "schema 11 session should upgrade to schema 12");
    expect(legacy.slots[2].effects[1].enabled,
        "legacy Actor FX should default to enabled");
    expect(legacy.master_effects[0].enabled,
        "legacy Master FX should default to enabled");
    std::filesystem::remove(legacy_path);

    auto updated = original;''')

print("Effect enabled schema 11 migration applied")

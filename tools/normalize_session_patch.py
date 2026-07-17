#!/usr/bin/env python3
from pathlib import Path

path = Path('tools/patch_session_lab.py')
text = path.read_text(encoding='utf-8')
old = """replace_once(path,
'''        for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {''',
'''        for (std::size_t degree = 0; degree < kScaleDegreeCount; ++degree) {
            if (!parse_float(values, key(slot_index, "tuning_cents_" + std::to_string(degree)),
                    slot.tuning.cents[degree])) {
                error = "invalid tuning in slot " + std::to_string(slot_index + 1U);
                return false;
            }
        }
        for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {''')"""
new = """replace_once(path,
'''            error = "invalid value in slot " + std::to_string(slot_index + 1U);
            return false;
        }
        for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {''',
'''            error = "invalid value in slot " + std::to_string(slot_index + 1U);
            return false;
        }
        for (std::size_t degree = 0; degree < kScaleDegreeCount; ++degree) {
            if (!parse_float(values, key(slot_index, "tuning_cents_" + std::to_string(degree)),
                    slot.tuning.cents[degree])) {
                error = "invalid tuning in slot " + std::to_string(slot_index + 1U);
                return false;
            }
        }
        for (std::size_t effect_index = 0; effect_index < kEffectsPerSlot; ++effect_index) {''')"""
if old not in text:
    raise SystemExit('ambiguous session selector not found')
path.write_text(text.replace(old, new), encoding='utf-8')

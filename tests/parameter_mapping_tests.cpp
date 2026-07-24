// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Myldy Design
// Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.
#include "cursed_drone/parameter_mapping.hpp"

#include <cmath>
#include <iostream>

namespace map = cursed_drone::mapping;

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool near(float left, float right, float tolerance = 0.0001F) {
    return std::abs(left - right) <= tolerance;
}

} // namespace

int main() {
    expect(near(map::frequency_from_normalized(0.0F), 20.0F),
        "frequency lower endpoint");
    expect(near(map::frequency_from_normalized(1.0F), 2'000.0F, 0.01F),
        "frequency upper endpoint");
    expect(near(map::frequency_from_normalized(
        map::normalized_frequency(440.0F)), 440.0F, 0.01F),
        "frequency mapping should roundtrip");

    expect(near(map::mod_rate_from_normalized(0.0F), 0.001F),
        "mod-rate lower endpoint");
    expect(near(map::mod_rate_from_normalized(1.0F), 40.0F, 0.001F),
        "mod-rate upper endpoint");
    expect(near(map::mod_rate_from_normalized(
        map::normalized_mod_rate(2.5F)), 2.5F, 0.001F),
        "mod-rate mapping should roundtrip");

    expect(near(map::fade_seconds_from_normalized(0.0F), 0.25F),
        "fade lower endpoint");
    expect(near(map::fade_seconds_from_normalized(1.0F), 30.0F),
        "fade upper endpoint");
    expect(near(map::fade_seconds_from_normalized(
        map::normalized_fade_seconds(8.25F)), 8.25F, 0.001F),
        "fade mapping should roundtrip");

    expect(map::tuning_root_from_normalized(0.0F) == 0,
        "tuning root lower endpoint");
    expect(map::tuning_root_from_normalized(1.0F) == 127,
        "tuning root upper endpoint");
    expect(map::tuning_root_from_normalized(
        map::normalized_tuning_root(36)) == 36,
        "tuning root mapping should roundtrip");

    expect(near(map::bipolar_from_normalized(0.0F), -1.0F),
        "bipolar lower endpoint");
    expect(near(map::bipolar_from_normalized(0.5F), 0.0F),
        "bipolar midpoint");
    expect(near(map::bipolar_from_normalized(1.0F), 1.0F),
        "bipolar upper endpoint");

    expect(map::next_rate_mod_source(-1, 0) == -1,
        "first modulator cannot have a rate source");
    expect(map::next_rate_mod_source(-1, 1) == 0 &&
        map::next_rate_mod_source(0, 1) == -1,
        "second modulator cycles only none and MOD 1");
    expect(map::next_rate_mod_source(-1, 3) == 0 &&
        map::next_rate_mod_source(0, 3) == 1 &&
        map::next_rate_mod_source(1, 3) == 2 &&
        map::next_rate_mod_source(2, 3) == -1,
        "fourth modulator cycles only earlier modulators");
    expect(map::next_rate_mod_source(3, 2) == 0,
        "invalid future source should be sanitized");

    expect(map::picker_max_page(21, 8) == 2,
        "picker max page");
    expect(map::picker_next_page(2, 21, 8) == 2,
        "picker next must clamp at final page");
    expect(map::picker_next_page(99, 21, 8) == 2,
        "stale picker page must recover to final page");
    expect(map::picker_previous_page(0) == 0,
        "picker previous must clamp at first page");

    if (failures == 0) {
        std::cout << "All parameter mapping tests passed\n";
        return 0;
    }
    std::cerr << failures << " parameter mapping test(s) failed\n";
    return 1;
}

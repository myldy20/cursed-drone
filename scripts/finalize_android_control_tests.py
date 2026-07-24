#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: Path, before: str, after: str) -> None:
    source = path.read_text(encoding="utf-8")
    if before not in source:
        raise SystemExit(f"missing fragment in {path}: {before[:120]!r}")
    path.write_text(source.replace(before, after, 1), encoding="utf-8")

android_touch = ROOT / "android/app/src/main/cpp/android_touch_main.cpp"
replace_once(android_touch,
'''        auto& effect = session.slots[static_cast<std::size_t>(state.actor)]
            .effects[static_cast<std::size_t>(state.actor_fx)];''',
'''        state.actor_fx = std::clamp(hit.a, 0, 3);
        auto& effect = session.slots[static_cast<std::size_t>(state.actor)]
            .effects[static_cast<std::size_t>(state.actor_fx)];''')
replace_once(android_touch,
'''        auto& effect = session.master_effects[static_cast<std::size_t>(state.master_fx)];''',
'''        state.master_fx = std::clamp(hit.a, 0, 3);
        auto& effect = session.master_effects[static_cast<std::size_t>(state.master_fx)];''')

cmake = ROOT / "CMakeLists.txt"
replace_once(cmake,
'''                target_link_libraries(cursed-drone-android-ui-snapshots
                    PRIVATE cursed_drone_core ${CURSED_DRONE_SDL_TARGET})
            endif()''',
'''                target_link_libraries(cursed-drone-android-ui-snapshots
                    PRIVATE cursed_drone_core ${CURSED_DRONE_SDL_TARGET})

                add_executable(cursed-drone-android-ui-interaction-tests
                    tools/android_ui_interaction_tests.cpp
                    src/bitmap_text.cpp)
                target_include_directories(cursed-drone-android-ui-interaction-tests
                    PRIVATE src include)
                target_compile_features(cursed-drone-android-ui-interaction-tests
                    PRIVATE cxx_std_20)
                target_compile_definitions(cursed-drone-android-ui-interaction-tests
                    PRIVATE CURSED_DRONE_VERSION="${PROJECT_VERSION}")
                target_compile_options(cursed-drone-android-ui-interaction-tests
                    PRIVATE -Wall -Wextra -Wpedantic)
                target_link_libraries(cursed-drone-android-ui-interaction-tests
                    PRIVATE cursed_drone_core ${CURSED_DRONE_SDL_TARGET})
                if(CURSED_DRONE_BUILD_TESTS)
                    add_test(NAME cursed-drone-android-ui-interaction-tests
                        COMMAND cursed-drone-android-ui-interaction-tests)
                    set_tests_properties(cursed-drone-android-ui-interaction-tests
                        PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy")
                endif()
            endif()''')

print("Android interaction test target integrated")

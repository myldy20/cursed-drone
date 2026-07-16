# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.16)

project(cursed_drone VERSION 0.1.0 LANGUAGES CXX)

option(CURSED_DRONE_BUILD_SDL "Build the SDL2 handheld/desktop frontend" ON)
option(CURSED_DRONE_BUILD_TESTS "Build the test executable" ON)

add_library(cursed_drone_core
    src/audio.cpp
    src/i18n.cpp
    src/session.cpp
    src/wav.cpp
)
target_include_directories(cursed_drone_core PUBLIC include)
target_compile_features(cursed_drone_core PUBLIC cxx_std_20)
target_compile_options(cursed_drone_core PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)

add_executable(cursed-drone src/main.cpp)
target_link_libraries(cursed-drone PRIVATE cursed_drone_core)

add_executable(cursed-drone-render tools/render_demo.cpp)
target_link_libraries(cursed-drone-render PRIVATE cursed_drone_core)

if(CURSED_DRONE_BUILD_TESTS)
    enable_testing()
    add_executable(cursed-drone-tests tests/test_main.cpp)
    target_link_libraries(cursed-drone-tests PRIVATE cursed_drone_core)
    add_test(NAME cursed-drone-tests COMMAND cursed-drone-tests)
endif()

if(CURSED_DRONE_BUILD_SDL)
    find_package(SDL2 QUIET)
    if(NOT TARGET SDL2::SDL2)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
        endif()
    endif()
    if(TARGET SDL2::SDL2)
        set(CURSED_DRONE_SDL_TARGET SDL2::SDL2)
    elseif(TARGET PkgConfig::SDL2)
        set(CURSED_DRONE_SDL_TARGET PkgConfig::SDL2)
    endif()
    if(CURSED_DRONE_SDL_TARGET)
        add_executable(cursed-drone-sdl src/sdl_main.cpp)
        target_link_libraries(cursed-drone-sdl PRIVATE cursed_drone_core ${CURSED_DRONE_SDL_TARGET})
        if(TARGET SDL2::SDL2main)
            target_link_libraries(cursed-drone-sdl PRIVATE SDL2::SDL2main)
        endif()

        add_executable(cursed-drone-probe tools/device_probe.cpp)
        target_link_libraries(cursed-drone-probe PRIVATE cursed_drone_core ${CURSED_DRONE_SDL_TARGET})
        if(TARGET SDL2::SDL2main)
            target_link_libraries(cursed-drone-probe PRIVATE SDL2::SDL2main)
        endif()
    else()
        message(STATUS "SDL2 development files not found; SDL frontend and device probe are disabled")
    endif()
endif()

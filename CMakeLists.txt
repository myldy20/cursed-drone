# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Myldy Design
# Additional terms under GPLv3 section 7: see ADDITIONAL_TERMS.md.
cmake_minimum_required(VERSION 3.16)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" CURSED_DRONE_VERSION LIMIT_COUNT 1)
if(NOT CURSED_DRONE_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "VERSION must contain one semantic version")
endif()
project(cursed_drone VERSION ${CURSED_DRONE_VERSION} LANGUAGES C CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(ANDROID)
    set(CURSED_DRONE_DESKTOP_DEFAULT OFF)
else()
    set(CURSED_DRONE_DESKTOP_DEFAULT ON)
endif()

option(CURSED_DRONE_BUILD_SDL "Build the SDL2 handheld/desktop frontend" ${CURSED_DRONE_DESKTOP_DEFAULT})
option(CURSED_DRONE_BUILD_TESTS "Build the test executable" ${CURSED_DRONE_DESKTOP_DEFAULT})
option(CURSED_DRONE_BUILD_TOOLS "Build command-line render and probe tools" ${CURSED_DRONE_DESKTOP_DEFAULT})
option(CURSED_DRONE_ENABLE_PLAITS "Build the pinned upstream Plaits actor when the submodule is present" ON)

add_library(cursed_drone_daisysp STATIC
    third_party/daisysp/Source/Filters/svf.cpp
    third_party/daisysp/Source/Noise/clockednoise.cpp
    third_party/daisysp/Source/Noise/grainlet.cpp
    third_party/daisysp/Source/Noise/particle.cpp
    third_party/daisysp/Source/PhysicalModeling/drip.cpp
    third_party/daisysp/Source/PhysicalModeling/modalvoice.cpp
    third_party/daisysp/Source/PhysicalModeling/resonator.cpp
    third_party/daisysp/Source/Synthesis/oscillator.cpp
)
target_include_directories(cursed_drone_daisysp SYSTEM PUBLIC
    third_party/daisysp/Source
    third_party/daisysp/Source/Filters
    third_party/daisysp/Source/Noise
    third_party/daisysp/Source/PhysicalModeling
    third_party/daisysp/Source/Synthesis
    third_party/daisysp/Source/Utility
)
target_compile_features(cursed_drone_daisysp PUBLIC cxx_std_20)

set(CURSED_DRONE_HAS_PLAITS 0)
if(CURSED_DRONE_ENABLE_PLAITS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/eurorack/plaits/dsp/voice.cc")
    file(GLOB_RECURSE CURSED_DRONE_PLAITS_SOURCES CONFIGURE_DEPENDS
        third_party/eurorack/plaits/dsp/*.cc
        third_party/eurorack/stmlib/dsp/*.cc
        third_party/eurorack/stmlib/utils/*.cc
    )
    list(FILTER CURSED_DRONE_PLAITS_SOURCES EXCLUDE REGEX "/test/")
    list(APPEND CURSED_DRONE_PLAITS_SOURCES third_party/eurorack/plaits/resources.cc)
    add_library(cursed_drone_plaits STATIC ${CURSED_DRONE_PLAITS_SOURCES})
    target_include_directories(cursed_drone_plaits SYSTEM PUBLIC third_party/eurorack)
    target_compile_definitions(cursed_drone_plaits PUBLIC TEST=1)
    target_compile_features(cursed_drone_plaits PUBLIC cxx_std_20)
    target_compile_options(cursed_drone_plaits PRIVATE
        -include stdio.h
        -Wno-unused-variable -Wno-unused-local-typedefs -Wno-sign-compare)
    set(CURSED_DRONE_HAS_PLAITS 1)
    message(STATUS "Cursed Drone: pinned upstream musical engine enabled")
else()
    message(WARNING "Cursed Drone: musical-engine submodule missing; building the audible fallback actor")
endif()

add_library(cursed_drone_core
    src/audio.cpp
    src/i18n.cpp
    src/plaits_actor.cpp
    src/scala.cpp
    src/session.cpp
    src/soundscape.cpp
    src/wav.cpp
)
target_include_directories(cursed_drone_core PRIVATE src)
target_include_directories(cursed_drone_core PUBLIC include)
target_compile_features(cursed_drone_core PUBLIC cxx_std_20)
target_compile_definitions(cursed_drone_core PRIVATE
    CURSED_DRONE_HAS_PLAITS=${CURSED_DRONE_HAS_PLAITS}
    CURSED_DRONE_VERSION="${PROJECT_VERSION}")
target_compile_options(cursed_drone_core PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
target_link_libraries(cursed_drone_core PRIVATE cursed_drone_daisysp)
if(CURSED_DRONE_HAS_PLAITS)
    target_link_libraries(cursed_drone_core PRIVATE cursed_drone_plaits)
endif()

if(ANDROID)
    include(FetchContent)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.32.10
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SDL2)

    add_library(main SHARED
        android/app/src/main/cpp/android_approved_main.cpp
        src/bitmap_text.cpp
    )
    target_include_directories(main PRIVATE src include)
    target_compile_features(main PRIVATE cxx_std_20)
    target_compile_definitions(main PRIVATE
        CURSED_DRONE_ANDROID=1
        CURSED_DRONE_VERSION="${PROJECT_VERSION}")
    target_compile_options(main PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(main PRIVATE cursed_drone_core SDL2::SDL2 android log)
    if(TARGET SDL2::SDL2main)
        target_link_libraries(main PRIVATE SDL2::SDL2main)
    elseif(TARGET SDL2main)
        target_link_libraries(main PRIVATE SDL2main)
    endif()
else()
    if(CURSED_DRONE_BUILD_TOOLS)
        add_executable(cursed-drone src/main.cpp)
        target_link_libraries(cursed-drone PRIVATE cursed_drone_core)

        add_executable(cursed-drone-render tools/render_demo.cpp)
        target_link_libraries(cursed-drone-render PRIVATE cursed_drone_core)

        add_executable(cursed-drone-benchmark tools/performance_benchmark.cpp)
        target_link_libraries(cursed-drone-benchmark PRIVATE cursed_drone_core)
    endif()

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
            add_executable(cursed-drone-sdl src/sdl_main.cpp src/bitmap_text.cpp)
            target_compile_definitions(cursed-drone-sdl PRIVATE CURSED_DRONE_VERSION="${PROJECT_VERSION}")
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
            message(WARNING "SDL2 was not found; cursed-drone-sdl and cursed-drone-probe will not be built")
        endif()
    endif()
endif()

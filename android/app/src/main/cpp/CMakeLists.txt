cmake_minimum_required(VERSION 3.22.1)
project(cursed_drone_android LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

get_filename_component(CURSED_DRONE_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../../../../.." ABSOLUTE)

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

add_library(cursed_drone_daisysp_android STATIC
    "${CURSED_DRONE_ROOT}/third_party/daisysp/Source/Filters/svf.cpp"
    "${CURSED_DRONE_ROOT}/third_party/daisysp/Source/Noise/clockednoise.cpp"
    "${CURSED_DRONE_ROOT}/third_party/daisysp/Source/Noise/grainlet.cpp"
    "${CURSED_DRONE_ROOT}/third_party/daisysp/Source/Noise/particle.cpp"
    "${CURSED_DRONE_ROOT}/third_party/daisysp/Source/PhysicalModeling/drip.cpp"
    "${CURSED_DRONE_ROOT}/third_party/daisysp/Source/PhysicalModeling/modalvoice.cpp"
    "${CURSED_DRONE_ROOT}/third_party/daisysp/Source/PhysicalModeling/resonator.cpp"
    "${CURSED_DRONE_ROOT}/third_party/daisysp/Source/Synthesis/oscillator.cpp"
)
target_include_directories(cursed_drone_daisysp_android SYSTEM PUBLIC
    "${CURSED_DRONE_ROOT}/third_party/daisysp/Source"
    "${CURSED_DRONE_ROOT}/third_party/daisysp/Source/Filters"
    "${CURSED_DRONE_ROOT}/third_party/daisysp/Source/Noise"
    "${CURSED_DRONE_ROOT}/third_party/daisysp/Source/PhysicalModeling"
    "${CURSED_DRONE_ROOT}/third_party/daisysp/Source/Synthesis"
    "${CURSED_DRONE_ROOT}/third_party/daisysp/Source/Utility"
)

set(CURSED_DRONE_HAS_PLAITS 0)
if(EXISTS "${CURSED_DRONE_ROOT}/third_party/eurorack/plaits/dsp/voice.cc")
    file(GLOB_RECURSE CURSED_DRONE_PLAITS_SOURCES CONFIGURE_DEPENDS
        "${CURSED_DRONE_ROOT}/third_party/eurorack/plaits/dsp/*.cc"
        "${CURSED_DRONE_ROOT}/third_party/eurorack/stmlib/dsp/*.cc"
        "${CURSED_DRONE_ROOT}/third_party/eurorack/stmlib/utils/*.cc"
    )
    list(FILTER CURSED_DRONE_PLAITS_SOURCES EXCLUDE REGEX "/test/")
    list(APPEND CURSED_DRONE_PLAITS_SOURCES
        "${CURSED_DRONE_ROOT}/third_party/eurorack/plaits/resources.cc")
    add_library(cursed_drone_plaits_android STATIC ${CURSED_DRONE_PLAITS_SOURCES})
    target_include_directories(cursed_drone_plaits_android SYSTEM PUBLIC
        "${CURSED_DRONE_ROOT}/third_party/eurorack")
    target_compile_definitions(cursed_drone_plaits_android PUBLIC TEST=1)
    target_compile_options(cursed_drone_plaits_android PRIVATE
        -include stdio.h
        -Wno-unused-variable
        -Wno-unused-local-typedefs
        -Wno-sign-compare
    )
    set(CURSED_DRONE_HAS_PLAITS 1)
endif()

add_library(cursed_drone_core_android STATIC
    "${CURSED_DRONE_ROOT}/src/audio.cpp"
    "${CURSED_DRONE_ROOT}/src/i18n.cpp"
    "${CURSED_DRONE_ROOT}/src/plaits_actor.cpp"
    "${CURSED_DRONE_ROOT}/src/scala.cpp"
    "${CURSED_DRONE_ROOT}/src/session.cpp"
    "${CURSED_DRONE_ROOT}/src/soundscape.cpp"
    "${CURSED_DRONE_ROOT}/src/wav.cpp"
)
target_include_directories(cursed_drone_core_android PRIVATE
    "${CURSED_DRONE_ROOT}/src")
target_include_directories(cursed_drone_core_android PUBLIC
    "${CURSED_DRONE_ROOT}/include")
target_compile_definitions(cursed_drone_core_android PRIVATE
    CURSED_DRONE_HAS_PLAITS=${CURSED_DRONE_HAS_PLAITS})
target_compile_options(cursed_drone_core_android PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
target_link_libraries(cursed_drone_core_android PRIVATE
    cursed_drone_daisysp_android)
if(CURSED_DRONE_HAS_PLAITS)
    target_link_libraries(cursed_drone_core_android PRIVATE
        cursed_drone_plaits_android)
endif()

add_library(main SHARED
    android_touch_main.cpp
    "${CURSED_DRONE_ROOT}/src/bitmap_text.cpp"
)
target_include_directories(main PRIVATE
    "${CURSED_DRONE_ROOT}/src"
    "${CURSED_DRONE_ROOT}/include"
)
target_compile_definitions(main PRIVATE CURSED_DRONE_ANDROID=1)
target_compile_options(main PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(main PRIVATE
    cursed_drone_core_android
    SDL2::SDL2
    android
    log
)
if(TARGET SDL2::SDL2main)
    target_link_libraries(main PRIVATE SDL2::SDL2main)
elseif(TARGET SDL2main)
    target_link_libraries(main PRIVATE SDL2main)
endif()

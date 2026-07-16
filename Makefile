# SPDX-License-Identifier: GPL-3.0-or-later
CXX ?= c++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic -Wshadow -Wconversion
VENDOR_CXXFLAGS ?= -std=c++20 -O2 -w
CPPFLAGS ?= -Iinclude -isystem third_party/daisysp/Source \
	-isystem third_party/daisysp/Source/Filters -isystem third_party/daisysp/Source/Noise \
	-isystem third_party/daisysp/Source/PhysicalModeling -isystem third_party/daisysp/Source/Synthesis \
	-isystem third_party/daisysp/Source/Utility
BUILD := build
DAISYSP := third_party/daisysp/Source/Filters/svf.cpp \
	third_party/daisysp/Source/Noise/clockednoise.cpp \
	third_party/daisysp/Source/Noise/grainlet.cpp \
	third_party/daisysp/Source/Noise/particle.cpp \
	third_party/daisysp/Source/PhysicalModeling/modalvoice.cpp \
	third_party/daisysp/Source/PhysicalModeling/resonator.cpp \
	third_party/daisysp/Source/Synthesis/oscillator.cpp
CORE := src/audio.cpp src/i18n.cpp src/session.cpp src/wav.cpp
DAISYSP_OBJECTS := $(patsubst %.cpp,$(BUILD)/%.o,$(DAISYSP))

.PHONY: all test render sdl probe clean

all: $(BUILD)/cursed-drone $(BUILD)/cursed-drone-tests $(BUILD)/cursed-drone-render

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/third_party/daisysp/%.o: third_party/daisysp/%.cpp | $(BUILD)
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(VENDOR_CXXFLAGS) -c $< -o $@

$(BUILD)/cursed-drone: $(CORE) $(DAISYSP_OBJECTS) src/main.cpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD)/cursed-drone-tests: $(CORE) $(DAISYSP_OBJECTS) tests/test_main.cpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD)/cursed-drone-render: $(CORE) $(DAISYSP_OBJECTS) tools/render_demo.cpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

test: $(BUILD)/cursed-drone-tests
	./$(BUILD)/cursed-drone-tests

render: $(BUILD)/cursed-drone-render
	mkdir -p out
	./$(BUILD)/cursed-drone-render out/cursed-drone-demo.wav 12

sdl: $(DAISYSP_OBJECTS) src/sdl_main.cpp src/bitmap_text.cpp third_party/font512/font_data.inc | $(BUILD)
	@if command -v sdl2-config >/dev/null 2>&1; then \
		$(CXX) $(CPPFLAGS) $(CXXFLAGS) $$(sdl2-config --cflags) $(CORE) $(DAISYSP_OBJECTS) src/sdl_main.cpp src/bitmap_text.cpp $$(sdl2-config --libs) -o $(BUILD)/cursed-drone-sdl; \
	else \
		echo "SDL2 development files are required for the SDL frontend"; exit 1; \
	fi

probe: $(DAISYSP_OBJECTS) | $(BUILD)
	@if command -v sdl2-config >/dev/null 2>&1; then \
		$(CXX) $(CPPFLAGS) $(CXXFLAGS) $$(sdl2-config --cflags) $(CORE) $(DAISYSP_OBJECTS) tools/device_probe.cpp $$(sdl2-config --libs) -o $(BUILD)/cursed-drone-probe; \
	else \
		echo "SDL2 development files are required for the device probe"; exit 1; \
	fi

clean:
	rm -rf $(BUILD) out

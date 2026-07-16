# SPDX-License-Identifier: GPL-3.0-or-later
CXX ?= c++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic -Wshadow -Wconversion
CPPFLAGS ?= -Iinclude
BUILD := build
CORE := src/audio.cpp src/i18n.cpp src/session.cpp src/wav.cpp

.PHONY: all test render sdl clean

all: $(BUILD)/cursed-drone $(BUILD)/cursed-drone-tests $(BUILD)/cursed-drone-render

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/cursed-drone: $(CORE) src/main.cpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD)/cursed-drone-tests: $(CORE) tests/test_main.cpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD)/cursed-drone-render: $(CORE) tools/render_demo.cpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

test: $(BUILD)/cursed-drone-tests
	./$(BUILD)/cursed-drone-tests

render: $(BUILD)/cursed-drone-render
	mkdir -p out
	./$(BUILD)/cursed-drone-render out/cursed-drone-demo.wav 12

sdl: | $(BUILD)
	@if command -v sdl2-config >/dev/null 2>&1; then \
		$(CXX) $(CPPFLAGS) $(CXXFLAGS) $$(sdl2-config --cflags) $(CORE) src/sdl_main.cpp $$(sdl2-config --libs) -o $(BUILD)/cursed-drone-sdl; \
	else \
		echo "SDL2 development files are required for the SDL frontend"; exit 1; \
	fi

clean:
	rm -rf $(BUILD) out

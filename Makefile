# SPDX-License-Identifier: GPL-3.0-or-later
# CMake is the single build definition for desktop, handheld and Android.
CMAKE ?= cmake
BUILD ?= build
BUILD_TYPE ?= Release
JOBS ?= 2

.PHONY: configure all test render sdl probe benchmark clean

configure:
	$(CMAKE) -S . -B $(BUILD) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

all: configure
	$(CMAKE) --build $(BUILD) --parallel $(JOBS)

test: all
	ctest --test-dir $(BUILD) --output-on-failure

render: all
	mkdir -p out
	./$(BUILD)/cursed-drone-render out/cursed-drone-demo.wav 12

sdl: configure
	$(CMAKE) --build $(BUILD) --target cursed-drone-sdl --parallel $(JOBS)

probe: configure
	$(CMAKE) --build $(BUILD) --target cursed-drone-probe --parallel $(JOBS)

benchmark: configure
	$(CMAKE) --build $(BUILD) --target cursed-drone-benchmark --parallel $(JOBS)
	./$(BUILD)/cursed-drone-benchmark 5

clean:
	rm -rf $(BUILD) out

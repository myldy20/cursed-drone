# Testing on macOS

Download the `cursed-drone-macos-arm64` artifact from **Actions → build → latest successful run**, then:

```bash
cd ~/Downloads/cursed-drone-macos-arm64
chmod +x cursed-drone cursed-drone-sdl cursed-drone-probe cursed-drone-render
xattr -dr com.apple.quarantine .
./cursed-drone-probe --windowed --seconds 20
./cursed-drone-sdl
```

The probe should draw four pulsing columns while playing the current four-slot patch. Escape exits after the first two seconds.

To build from source on Apple Silicon:

```bash
xcode-select --install
brew install cmake sdl2
git clone https://github.com/myldy20/cursed-drone.git
cd cursed-drone
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/cursed-drone-probe --windowed --seconds 20
./build/cursed-drone-sdl
```

Keyboard controls are Left/Right for slot, Up/Down for parameter, A/D to change, Tab or 1–4 to select Performance/Slot/FX/Master, Space to mute while preserving tails, F for a four-second fade, K to clear voices and FX memory, E to change the selected FX algorithm, L for RU/EN, and Escape to quit.

All labels now render inside the window with the built-in Cyrillic/Latin bitmap font. A/D starts at one-percent precision and accelerates after 1.05 and 2.2 seconds. Meters, peak lines, scope motion and FX brightness use live audio telemetry. Please check that Russian glyphs remain aligned, the performance macros sound dramatic enough, mute preserves delay tails, and K removes them.

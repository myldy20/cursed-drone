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

Keyboard controls are Left/Right for slot, Up/Down for parameter, A/D to change, Space to mute, L for RU/EN, and Escape to quit. Parameter text currently lives in the window title; the complete font UI follows the Brick video-driver probe.

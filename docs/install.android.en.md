# Android alpha

The Android port keeps the existing C++ audio engine and SDL interface. Android only adds a thin fullscreen shell, an ARM64 native build and a touch adapter.

## Status

This is an **alpha** target intended for real-device testing before a public release.

- Android 8.0 or newer (`minSdk 26`)
- ARM64 devices only (`arm64-v8a`)
- landscape orientation
- headphones, USB audio interfaces and the device speaker are handled through SDL/Android audio
- session autosave and eight memory slots use the app's private Android data directory

## Install a CI build

1. Open the GitHub Actions run for the `android` workflow.
2. Download the `cursed-drone-android-debug` artifact.
3. Unzip it and install `app-debug.apk` on the Android device.
4. Android may ask you to allow installation from the browser or file manager used to open the APK.

A debug APK is unsigned for store distribution but is signed with the normal Android debug key, so it can be installed directly for testing.

## Touch controls

The original 512×384 instrument UI is shown on the left. A dedicated touch controller is shown on the right.

| Touch action | Result |
| --- | --- |
| Tap the instrument area | A / open / apply |
| Double-tap the instrument area | Next section |
| Drag horizontally or vertically | D-pad movement and value editing |
| Hold an arrow on the touch panel | Repeated D-pad input with the original acceleration logic |
| PAGE− / PAGE+ | Previous / next page |
| BACK | Back or cancel |
| SECTION | Next section on the current page |
| HELP | Context help |
| MENU | Quick menu |
| FADE | Fade output in or out |
| KILL | Immediate panic / silence |

External keyboards and game controllers remain supported by SDL.

## Build locally

Requirements:

- Android Studio or Android command-line tools
- JDK 17
- Android SDK 35
- Android NDK `27.1.12297006`
- CMake 3.22.1
- Gradle 8.10.2 or a compatible Gradle installation

Clone with submodules, then build:

```bash
git clone --recurse-submodules https://github.com/myldy20/cursed-drone.git
cd cursed-drone
gradle -p android :app:assembleDebug
```

The APK will be written to:

```text
android/app/build/outputs/apk/debug/app-debug.apk
```

The Gradle build downloads the official SDL2 Android Java shim and SDL2 native source from the pinned `release-2.32.10` tag. The Cursed Drone audio core and bundled Plaits submodule are compiled directly into the APK.

## Known alpha limitations

- only ARM64 is packaged;
- no Play Store bundle or release signing configuration yet;
- touch targets use a fixed landscape layout and still need validation on phones and tablets with different aspect ratios;
- Android audio latency and USB audio behaviour depend on the device and Android firmware;
- interruption handling for phone calls, Bluetooth changes and audio-device hot-plug still needs hardware testing.

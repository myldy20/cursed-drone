# Android alpha 2

The Android port keeps the existing C++ audio engine and SDL interface. The Android layer handles fullscreen startup, the ARM64 build, app data, a stable audio-buffer profile and direct touch interaction.

## Status

This is an **alpha** target intended for real-device testing.

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

The debug APK uses the standard Android debug key. Play Protect can therefore report that the package comes from an unknown or unverified developer. A normal public distribution will require a permanent release key and developer registration; this warning does not indicate a problem in the audio engine.

## Touch controls

The separate virtual gamepad has been removed. The complete instrument UI scales into the available landscape area while preserving the original layout proportions.

- tap a page tab to open that page directly;
- tap an actor, FX slot, parameter row or memory slot to select it directly;
- drag horizontally on a parameter row to change its value;
- tap the landscape, engine or effect type to open the existing picker;
- tap an item in a picker to select and apply it immediately;
- use the bottom `FADE` control to fade the final output in or out;
- use Android Back to close a picker or return to the Place page.

On the Place page, the lower part of an actor card toggles mute while the rest of the card selects that actor.

External keyboards and game controllers remain supported through SDL, but the Android workflow no longer depends on their button model.

## Android audio profile

The activity queries Android for the device's native sample rate and preferred hardware burst size. SDL then opens a more conservative callback buffer of at least 2048 frames. This increases control latency slightly but significantly reduces the risk of underruns and grainy interruptions. For a drone instrument, stable output is more important than game-grade latency.

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
- no Play Store bundle or permanent release signing configuration yet;
- portrait layout is not supported because it needs a separate composition rather than simple scaling;
- the direct touch adapter mirrors the original SDL state machine and still needs validation across unusual navigation paths;
- Android audio latency and USB/Bluetooth routing depend on the device and firmware;
- phone-call interruption, device changes and audio-interface hot-plug still need hardware testing.

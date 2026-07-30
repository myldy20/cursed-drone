# Install Cursed Drone 1.0.0 on Android

Cursed Drone 1.0.0 is the feature-complete release. The Android build is a touch-first **ARM64 sideload package** for Android 8.0 or newer. It uses the same DSP, scenes, effects and session format as the handheld, macOS and WebAssembly builds. Android and Web share the same adaptive fullscreen landscape interface.

1. Download `cursed-drone-v1.0.0-android-arm64-preview.apk` from the GitHub release.
2. Allow installation from the browser or file manager used to open the APK.
3. Install and launch Cursed Drone; rotate the phone to landscape if necessary.

The APK is release-optimised but remains signed for public sideload testing rather than Google Play production distribution. Project version 1.0 describes feature and compatibility maturity; it does not turn the APK into a Play Store package. A differently signed future APK may require uninstalling the previous application first.

Android stores autosave, eight memory slots and imported Scala files inside the app's private data directory. Removing the app also removes that private data unless Android restores a backup. Save important patch settings separately before reinstalling.

Touch workflow: direct page tabs, actor mute buttons, draggable sliders and modal grids. The Actor Events tab includes Trigger Now, per-actor Event Rate/FMAX and Euclidean controls; Scala tuning and Musical macro models are available on Actor Sound.

For reliability reports, see [Support and log locations](support.en.md).

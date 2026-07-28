# Install Cursed Drone 0.13.0 on Android

The Android build is a touch-first **ARM64 public preview** for Android 8.0 or newer. It uses the same DSP, scenes, effects and session format as the handheld and macOS builds, but has a dedicated fullscreen landscape interface.

1. Download `cursed-drone-v0.13.0-android-arm64-preview.apk` from the GitHub release.
2. Allow installation from the browser or file manager used to open the APK.
3. Install and launch Cursed Drone; rotate the phone to landscape if necessary.

The APK is release-optimised and signed with the project preview key for sideloading. It is not a Google Play production package. Android stores autosave, eight memory slots and imported Scala files inside the app's private data directory. Removing the app also removes that private data unless Android restores a backup.

Touch workflow: direct page tabs, actor mute buttons, draggable sliders and modal grids. The Actor Events tab includes Trigger Now, per-actor Event Rate/FMAX and Euclidean controls; Scala tuning is available on Actor Sound.

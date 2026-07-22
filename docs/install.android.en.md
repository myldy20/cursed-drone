# Install Cursed Drone 0.13.0 on Android — preview

The Android build is a touch-first **ARM64 preview** for Android 8.0 or newer. It uses the same DSP, scenes, effects and session format as the handheld and macOS builds, but has a dedicated fullscreen landscape interface.

1. Download `cursed-drone-v0.13.0-android-arm64-preview.apk` from the GitHub release.
2. Allow installation from the browser or file manager used to open the APK.
3. Install and launch Cursed Drone; rotate the phone to landscape if necessary.

The preview APK is debug-signed for sideloading, not Play Store distribution. Android stores autosave, eight memory slots and imported Scala files inside the app's private data directory. Removing the app also removes that private data unless Android restores a backup.

Touch workflow: direct page tabs, actor mute buttons, draggable sliders and modal grids. The Actor Events tab includes Trigger Now, per-actor Event Rate/FMAX, Euclidean controls and tuning.

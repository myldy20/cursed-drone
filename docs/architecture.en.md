# Architecture — Cursed Drone 0.13.0

## One repository, one engine, platform-specific frontends

```text
                         ┌─ handheld/desktop SDL frontend (Brick, Linux, macOS)
Session + catalog + DSP ─┤
                         └─ touch-first SDL frontend (Android)
```

All platforms compile the same `cursed_drone_core`, DaisySP subset, pinned Plaits source, session schema, Scala support, effect catalogue and scene recipes from the root `CMakeLists.txt`. `VERSION` is the single application-version source used by CMake, Gradle, the desktop UI and release packaging. The frontends remain intentionally different: controller focus navigation is appropriate on the Brick, while Android uses direct tabs, touch sliders and modal grids.

```text
Place macros
   ↓
Actor 1 source → modulation → FX1 → FX2 → FX3 → FX4 → level/pan ┐
Actor 2 source → modulation → FX1 → FX2 → FX3 → FX4 → level/pan ├→ mix
Actor 3 source → modulation → FX1 → FX2 → FX3 → FX4 → level/pan ┤
Actor 4 source → modulation → FX1 → FX2 → FX3 → FX4 → level/pan ┘
                              mix → Master FX1..4 → DC blocker → saturator → fade
```

The audio thread receives complete `Session` snapshots through a bounded SPSC mailbox. Manual trigger requests use an atomic bit mask; event flashes are telemetry and are not persisted. Schema 11 remains shared by every platform, so a session contains the same actors, Event Rate, Scala tuning, Euclidean settings, modulation, actor FX, Master FX and performance macros everywhere.

## Performance model

- Muted actors are reset once and then removed from the per-sample loop; unmuting starts the actor from a clean state. Master FX tails remain global.
- Expensive, slowly changing event, modal and effect coefficients run at control rate while oscillators, envelopes, delay reads/writes and nonlinear audio processing remain at audio rate.
- Equal-power pan gains are read from an interpolated lookup table rather than calculated with two square roots per actor per sample.
- The DC blocker is derived from the actual sample rate with a 12 Hz corner, preserving Sub Drone and Earth Rumble fundamentals.
- Kill Silence invalidates delay history in constant time rather than clearing large buffers inside the callback.

Autosave and eight memory slots use the same readable `.cdrone` files on all platforms. Writes are atomic and preserve a `.bak` copy of the previous complete session.

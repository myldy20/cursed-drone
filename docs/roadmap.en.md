# Roadmap

## 0.2 slice — interface and autonomy (done)

The prototype now includes a dependency-free Cyrillic/Latin bitmap renderer, Performance/Slot/FX/Master views, accelerated hold input, non-destructive Texture/Pulse/Chaos/Space/Fade macros, BPM pulses, smoothed random events, audio-reactive telemetry, tail-preserving mute, hard Kill/Panic, and backward-compatible session schema 2. This is an early M4/M5 slice shipped before product engines so the control model can be tested on hardware.

## M0 — testable signal path (done)

GPL-3.0-or-later project, dependency-light C++20 core, 4 slots × 4 effects × 4 modulation lanes, lock-free SPSC session transfer, RU/EN catalog, `.cdrone` serialization, offline WAV, SDL2 shell and smoke tests.

## M1 — real TrimUI Brick probe

Build aarch64, verify SDL video/audio and every button, identify writable storage, measure callback sizes and xruns, map each supported firmware explicitly, verify the new Cyrillic/Latin font renderer on the physical display, and persist language/session.

**Gate:** ten continuous minutes of the diagnostic patch with no underrun or memory growth.

## M2 — DSP benchmark harness

Measure engine/effect cost, run 4-engine/16-effect scenarios at available CPU frequencies, publish the budget, expose overload health, and define Eco fallbacks.

**Gate:** a useful four-slot patch at 1008 MHz with at least 25% block headroom.

## M3 — product engines

1. `PARTICLE`: selected DaisySP noise/particle algorithms.
2. `MACRO`: audited MIT STM32 macro-oscillator DSP under a neutral product label.
3. `BODY`: resonator and physical-model DSP.
4. `GRAIN`: circular stereo capture and granular processing.

Every import records an exact commit, license, provenance and adapter tests. Any slot can host any engine kind.

## M4 — controlled corruption

Add small bounded `Mutate`, structural `Corrupt`, parameter locks, deterministic seeds, Brownian/logistic/follower/Euclidean/probability modulation, shared clock ratios, undo, and evolve the current hard Kill into a short click-free hardware Panic action.

## M5 — complete UX

Ship four slot pages, global mixer/performance views, bilingual focus help, debounced crash-safe autosave, factory sound spaces, and a CPU/memory/audio health screen.

## M6 — distribution

Publish PortMaster aarch64 first, Brick/Knulli instructions, armhf after benchmarking, release archives/checksums/license bundles, and only then consider a libretro adapter.

Near-term non-goals include a tracker note table, DAW timeline, plugin hosting, a Csound runtime, unlimited patching, or a large home-grown DSP collection.

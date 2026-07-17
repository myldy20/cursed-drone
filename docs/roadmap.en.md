# Roadmap

## 0.6 slice — different acoustic worlds (in development)

- Wet Cave, Metro Car and Broken Nursery add twelve actor models and bring the total to six landscapes / twenty-four actors;
- audited DaisySP `Drip`, physical water/stone events, rail mechanics and sparse pitch-set objects;
- high-pass, wavefolder, ring modulation and comb effects with parameter-reactive diagrams;
- target landscape picker, direct track-level editing on Scene and four visible source tracks on FX;
- post-limiter master/fade gain with a monotonicity regression test;
- schema 6 and six automatic WAV previews.

**Next gate:** subjective Mac listening, then CPU and underrun measurement on the Brick. Equal-power dual-bus scene morphing follows after the six-scene load is profiled.

## 0.5 slice — coherent live interface (done)

- consistent spatial D-pad navigation and independent slot rows;
- categorized engine/effect pickers and actor-specific control names;
- explicit output fade state, integer DSP telemetry and default English UI;
- mirrored English/Russian README and synthesis catalogue.

## 0.4 slice — procedural landscapes (ready for subjective testing)

- three scene recipes: Derelict, Factory and Wasteland;
- twelve actors with separate timing, excitation and material models;
- heel/toe approach footsteps, stick-slip door, wind, motors, machine, crowd, metal, birds and a rising signal;
- Material / Activity / Tension / Distance / Evolution macros instead of cosmetic shared-FX control;
- schema 4 with automatic migration of an old autosave to the new default scene;
- fallback glyphs for typography and arrows instead of question marks;
- three automatic WAV previews in GitHub Actions.

**Next gate:** subjective Mac listening, followed by DSP load and a ten-minute Brick run.

## 0.3 slice — distinct sources and a legible scene (done)

- pinned vendored DaisySP subset with the full MIT notice;
- four distinct default sources: `TONE`, `RESONATOR`, `GRAINLET`, and `PARTICLES`;
- a Scene view without four copies of one global macro;
- consistent Scene / Slots / FX / Master / Setup terminology;
- a new Events macro, per-engine pulse ratios/depths, and tempo-scaled events;
- captured waveform snapshots instead of decorative sine waves;
- explicit RMS/peak behavior, DSP load, and global auto-fade indication;
- separate fade-in/fade-out times and debounced schema 3 autosave.

## 0.2 slice — interface and autonomy (done)

The prototype now includes a dependency-free Cyrillic/Latin bitmap renderer, Performance/Slot/FX/Master views, accelerated hold input, non-destructive Texture/Pulse/Chaos/Space/Fade macros, BPM pulses, smoothed random events, audio-reactive telemetry, tail-preserving mute, hard Kill/Panic, and backward-compatible session schema 2. This is an early M4/M5 slice shipped before product engines so the control model can be tested on hardware.

## M0 — testable signal path (done)

GPL-3.0-or-later project, dependency-light C++20 core, 4 slots × 4 effects × 4 modulation lanes, lock-free SPSC session transfer, RU/EN catalog, `.cdrone` serialization, offline WAV, SDL2 shell and smoke tests.

## M1 — real TrimUI Brick probe

Build aarch64, verify SDL video/audio and every button, identify writable storage, measure callback sizes and xruns, map each supported firmware explicitly, verify the new Cyrillic/Latin font renderer on the physical display, and persist language/session.

**Gate:** ten continuous minutes of the four-engine patch with no underrun or memory growth.

## M2 — DSP benchmark harness

Measure engine/effect cost, run 4-engine/16-effect scenarios at available CPU frequencies, publish the budget, expose overload health, and define Eco fallbacks.

**Gate:** a useful four-slot patch at 1008 MHz with at least 25% block headroom.

## M3 — product sources (continues after 0.4)

1. `PARTICLE`: a lightweight DaisySP Oscillator/Dust/Particle/ClockedNoise/Grainlet set is complete.
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

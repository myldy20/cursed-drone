# Architecture

## Goals

1. Sustain four engines and up to sixteen effects on a roughly 1 GHz quad-core ARM.
2. Never block the audio thread on files, mutexes, logging or allocation.
3. Keep the product independent from any single DSP project through narrow adapters.
4. Preserve equivalent behavior on desktop, TrimUI Brick and other SDL2 handhelds.
5. Turn a deliberately small control surface into an expressive instrument through modulation and bounded randomness.

## Layers

```mermaid
flowchart TD
    UI["UI + input + i18n"] --> Model["Session model"]
    Model --> Queue["Bounded SPSC mailbox"]
    Queue --> Graph["Audio graph"]
    Graph --> Slots["4 engine slots"]
    Slots --> Mix["Mixer + safety"]
    Mix --> Device["SDL audio / offline WAV"]
```

`Session` is a trivially-copyable snapshot owned and edited by the UI. `SpscQueue<Session, 8>` transfers complete snapshots from one producer to one consumer and reports overflow. `AudioGraph` owns every DSP runtime object. Once `prepare()` returns, `process()` allocates no memory, takes no locks and throws no exceptions. SDL is only a platform adapter; the core has no SDL dependency.

## Slot graph

Each slot is `engine → FX1 → FX2 → FX3 → FX4 → level/pan`. Four modulation lanes alter smoothed engine, mix or effect parameters before each sample is processed.

The `0.3.0` default patch uses four distinct adapters around a pinned DaisySP subset: `TONE` (Oscillator + ClockedNoise), `RESONATOR` (ModalVoice/Resonator), `GRAINLET`, and `PARTICLES` (Particle/SVF). The old diagnostic source remains only for schema 1/2 loading. Future engines follow the same conceptual contract:

```cpp
prepare(sample_rate, max_block_frames)
reset()
process(parameters, output_block)
```

An adapter must allocate ahead of time, map stable product macros to upstream ranges, avoid UI/files/global singletons, declare a rough CPU tier, and pass finite/non-silent smoke tests.

## Parameters and modulation

The stable detailed surface is `frequency`, `timbre`, `color`, `motion`, `texture`, `level`, and `pan`, with engine-specific UI labels for the four character controls. Continuous detailed values use roughly 20 ms one-pole smoothing; master/performance use 80–120 ms. Schema 3 stores non-destructive `texture`, `pulse`, `chaos`, `space`, and `events` performance values plus separate fade-in/fade-out times. Fade remains a master-output state. The loader migrates schema 1/2 with safe defaults.

Current sources are sine, triangle, sample-and-hold and bounded random walk. Destinations are pitch, the engine macros, level, pan, and every FX amount. Pitch is exponential; other routes are additive and clamped. Pulse supplies per-engine nonlinear BPM envelopes with distinct ratios/depths. Events scales strike/burst frequency and bounded random gaps. Chaos updates rate-dependent random targets and slews them before affecting pitch/level/pan/texture. Planned sources include directed event ramps, a bounded quantizer, follower, Brownian/logistic, Euclidean, probability bursts and short step curves—without introducing a tracker grid.

## FX, memory and safety

Every FX cell currently preallocates a 1.3 second stereo delay line. Sixteen cells consume about 8 MB at 48 kHz, but algorithm changes never allocate in the callback. A later shared fixed pool may reduce that footprint.

After summing the four slots, master smoothing feeds a DC blocker and a `tanh` soft limiter whose output remains inside `[-1, 1]`. The current atomic Kill request clears sources, delay memory, DC state and telemetry at the next block boundary. A later revision will surround that hard reset with a short click-free fade.

| Thread | Allowed | Forbidden |
| --- | --- | --- |
| audio callback | bounded queue reads, DSP, atomics | allocation, files, mutexes, stdout, sleep |
| UI/main | input, drawing, Session edits and saving | direct mutation of DSP runtime |
| background (later) | loading into staging buffers | writes to active audio buffers |

The logical UI is `512×384`, scaling exactly to the Brick's `1024×768`; wider screens use letterboxing. A vendored 512-glyph bitmap font supplies in-window Russian and English without SDL_ttf. The audio thread publishes per-slot/master RMS, peak, and 64-point captured waveforms through atomics. The SDL callback measures processing time as a fraction of the available block and displays it as DSP load. The UI writes debounced schema 3 autosaves under `SDL_GetPrefPath`.

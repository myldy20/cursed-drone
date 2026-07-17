# Sound reference analysis

The user's WAV is not stored in the repository. This document records only measurements and design conclusions needed for reproducible engine work.

## Measurements

The reference is 47.426 seconds, stereo 44.1 kHz / float32. Integrated RMS is approximately −12.08 dBFS, crest factor 13.55 dB, and stereo correlation 0.88. The floating-point file briefly reaches about +1.47 dBFS; that is not a target output level for Cursed Drone.

- 42.5% of spectral energy sits between 20 and 80 Hz;
- 47.2% sits between 80 and 250 Hz;
- another 7.8% sits between 250 and 800 Hz;
- the strongest stable peak is near 64.6 Hz, followed by 129.2 Hz;
- median spectral centroid is approximately 517 Hz;
- the strongest broad envelope period is about 1.33 seconds, or 45.2 BPM.

The reference has a low-frequency harmonic body, but its identity does not come from a static oscillator. It contains resonant bursts, irregular attacks, changing spectral brightness, and a long final decay after roughly 38 seconds.

## Architectural consequences

A single drone engine with selectable waveforms is insufficient. The instrument needs three independent behaviours:

1. a continuous tonal body for background and beating;
2. an excitable modal resonator for metallic strikes, string-like color and digging/footstep events;
3. an event source for particles and grains with bounded random gaps.

In `0.4.0` these roles are direct: Derelict combines room bed, footsteps, door and pipe; Factory uses motor, machine, crowd and metal; Wasteland uses wind, birds, insects and signal. Shared tempo scales repeating processes, while every actor owns its gesture state.

## Target sound spaces

### Thin background plus crumbling foreground steps

- `TONE`: low shape/noise, moderate drift, low level;
- `RESONATOR`: low or mid frequency, strong excitation, moderate brightness;
- `GRAINLET` or `PARTICLES`: a quiet granular attack edge;
- `Activity` 40–70%, `Tension` 15–35%, and modest `Distance`.

### Rising impulse followed by a random pause

In 0.4 this is the dedicated `SIGNAL` actor: attack/ramp → release → a directed rising sequence → bounded random hold. It is a small state machine, not a tracker sequencer.

### Low metallic pulses plus an almost-piano upper layer

- use two `RESONATOR` slots with different roots and material settings;
- keep the lower layer dark, stiff, and sparsely struck;
- make the upper layer brighter, shorter, quieter, and non-continuous;
- a bounded quantizer is still needed for pseudo-melody: a small interval set, repeat probability, and a maximum-jump rule.

These three spaces are acceptance targets for future factory scenes. They should not collapse into three fixed presets: macro travel must stay musically useful between the extremes.

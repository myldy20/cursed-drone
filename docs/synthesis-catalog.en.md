# Synthesis catalogue — 1.0.0

Cursed Drone combines project-specific environmental engines, an audited DaisySP subset and a pinned upstream macro-oscillator implementation.

## Landscape engines

Continuous beds, footsteps, doors, pipes, machinery, crowds, wind, birds, insects, signals, cave air, water, rail motion, nursery mechanisms and bass-first drone engines are mapped into ten place recipes.

The five Place macros reshape the complete landscape rather than acting as simple effects:

- **Material** changes spectral weight and surface character;
- **Activity** increases motion and actor behaviour;
- **Tension** pushes instability and brighter or harsher states;
- **Distance** changes apparent proximity and space;
- **Evolution** increases long-term movement and event development.

## Musical actor

The user-facing source is **Musical**. Internally it wraps selected engines from Mutable Instruments Plaits commit `08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4` under MIT. The descriptions below are practical listening cues, not claims of exact acoustic emulation.

| Model | Practical character |
| --- | --- |
| Filter Tone | Resonant, filtered analogue-style tone; useful for stable drones and controlled bass. |
| Phase Tone | Phase-distorted harmonics with a sharper, animated edge. |
| Wave Terrain | Complex continuously changing waveform terrain; good for unstable textures. |
| String Machine | Ensemble-like sustained string colour and wide harmonic beds. |
| Chip Tone | Digital, stepped and game-like tone with clear upper harmonics. |
| Analog | Familiar analogue oscillator shapes and broad subtractive-drone material. |
| Waveshaper | Nonlinear and folded spectra that move from restrained to aggressive. |
| FM | Metallic, bell-like and inharmonic frequency-modulated material. |
| Grain | Granular or particle-like tone with broken and scattered edges. |
| Additive | Layered harmonic partials suited to organ-like or glassy sustained sounds. |
| Wavetable | Scanned digital spectra with strong movement under Timbre and Morph. |
| Chord | Stacked pitch intervals for immediate harmonic beds. |
| Swarm | Dense detuned clusters and beating oscillator clouds. |
| Noise | Noise-based spectra for air, hiss, percussion and non-pitched layers. |
| String | Physical-string behaviour for plucked, struck or sustained resonances. |
| Modal Body | Resonant body modes for struck objects, bells and hollow structures. |

## Musical controls

The same four macro controls are deliberately reused for all 16 models. Their exact effect is model-dependent:

- **Harmonics** changes the model's harmonic structure or internal variant and also colours the internal low-pass-gate response;
- **Timbre** changes the main spectral or synthesis dimension, usually from darker or simpler to brighter or denser;
- **Morph** controls the second model-specific dimension, often shape, spread, detune or internal balance;
- **Decay** controls envelope and resonant decay when triggering is active; on sustained models it can still change articulation or internal damping.

Use small movements first. Many models have musically useful discontinuities or strongly nonlinear regions.

## Output routing

The Musical engine renders separate MAIN and AUX signals:

- **MAIN** sends the main signal equally to left and right;
- **AUX** sends the alternate signal equally to left and right;
- **MIX** averages MAIN and AUX into mono;
- **STEREO** sends MAIN to the left channel and AUX to the right channel.

AUX is not simply a quieter copy: its meaning changes by model. Switching routing can therefore alter pitch emphasis, transient content or apparent width.

## Pitch, triggers and modulation

The Musical actor supports 12-TET, bundled Scala scales and user `.scl` files. Scala quantisation is applied after pitch modulation at a bounded control cadence.

Euclidean triggering can articulate Musical models rhythmically. The four modulation rows offer sine, triangle, sample-and-hold and random-walk sources routed to pitch, Musical macros, level, pan or Actor FX controls.

For stable concert patches, save the final routing and scale into a memory slot and rehearse the exact output mode: MAIN/AUX differences can be substantial.

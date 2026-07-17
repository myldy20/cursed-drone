# Procedural soundscape architecture

## Why four synthesizers were not enough

Version 0.3 proved the audio graph but not the musical premise. Four different generators still produced four continuous signals controlled by similar pitch, tone, noise and effect parameters. A global macro could move several numbers, but it could not create a footstep, a door gesture, an industrial duty cycle or a meaningful pause. Those require state, a gesture envelope, an excitation/material model and a slower law of change.

In 0.4 a default slot is a **scene actor** rather than just a synthesizer:

1. a potentially continuous environmental bed;
2. motion or a repeating process;
3. a sparse foreground object;
4. a distant, airborne or textural layer.

The expert page still exposes frequency, four actor-specific character controls, level, pan, FX and modulators. Scene control sits above it and changes how sound is caused, not only a single DSP value.

## Six scenes

| Scene | Actor | Procedural model |
| --- | --- | --- |
| Derelict | room bed | slowly detuned low components, air bands, a long directed pitch climb and amplitude breathing |
| Derelict | footsteps | separate heel/toe envelopes, low impact, filtered surface noise and a larger approach/recede cycle |
| Derelict | door | stick-slip micro-impulses, short inharmonic wood modes and a directed squeal/glissando |
| Derelict | pipe | noise flow through drifting resonances, howl only at strong gusts and sparse short body knocks |
| Factory | motor | separate rotor harmonics, high-frequency brushes, floating load and slow speed changes |
| Factory | machine | impact plus pneumatic exhaust, a short body and BPM used as a time scale rather than a note grid |
| Factory | crowd | three moving noise formants with independent phrase envelopes; vocal mass without speech synthesis |
| Factory | metal | long noise friction through moving narrow bands plus a short, strongly damped clank with no bell sustain |
| Wasteland | wind | fast noise, slow random gust and slower squall; tonal howl appears only near flow peaks |
| Wasteland | birds | random clusters of short frequency-shaped phrases with contour, gaps and a small grain component |
| Wasteland | insects | asynchronous impulse carriers, colony activity phases and narrow-band scratch |
| Wasteland | signal | a sequence of rising pulses followed by a bounded random pause and restart |
| Wet Cave | cave air | three filtered pressure bands, subsonic body and independent slow breathing |
| Wet Cave | drips | audited MIT DaisySP/Perry Cook physical water model with controllable pitch, damping and density |
| Wet Cave | water flow | turbulent multi-band water plus short bubble chirps with independent event timing |
| Wet Cave | stone | sparse impacts exciting short inharmonic rock modes without bell-length sustain |
| Metro Car | traction | rotor harmonics, accelerating inverter whine, bearing band and a long run/coast cycle |
| Metro Car | rail joints | paired impact envelopes, rail body modes and process-tempo-scaled repetition |
| Metro Car | brake | finite high-resonance friction gestures with a directional downward spectral sweep |
| Metro Car | carriage | low chassis pressure, sway, narrow-band rattle and speed-dependent loose parts |
| Broken Nursery | music box | random pentatonic tines, short excitation and a small decaying body |
| Broken Nursery | toy voice | bounded circuit-bent syllable gestures, pulse carrier and battery dropout |
| Broken Nursery | gears | tooth impulses, cheap motor harmonics and slowly collapsing battery voltage |
| Broken Nursery | lullaby | sparse pseudo-notes chosen from a small pitch set, glass partial and long fragile decay |

Every source is synthesized in real time. The repository contains no recorded footstep, door, bird or factory samples.

## Five macros

| Macro | Time scale | Examples of actual influence |
| --- | --- | --- |
| Material | inside a gesture | step surface, brushes and dust, friction, excitation brightness, noise/tone balance |
| Activity | events and processes | walking rate, creak probability, machine cycle, bird cluster size and signal density |
| Tension | directed change | background pitch climb, motor load, howl height, scrape range and pulse-sequence rise |
| Distance | mix and memory | dry/wet, feedback, tails and separation of foreground from distant layers |
| Evolution | long cycles | approach speed, gesture duration, sequence length, random-pause scale and environmental mutation |

Macros are smoothed. Randomness uses a separate deterministic seed per actor and bounded ranges, so offline rendering is reproducible and unsafe feedback cannot appear by accident.

## Open sources studied

- [DaisySP](https://github.com/electro-smith/DaisySP) remains the vendored MIT base: oscillators, stable SVF, clocked noise, grainlet, particle and modal blocks. The new layer primarily uses its state-variable filters; Cursed Drone owns scene orchestration and event memory.
- Andy Farnell's openly available [Designing Sound code examples](https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/index.html) were used as decomposition references: wind as multiple noise time scales; motor as rotor + brushes + body; footsteps as consecutive support phases; door as stick-slip + panel modes. The Pure Data patches are not copied or shipped.
- [STK](https://github.com/thestk/stk) validates compact physically informed instruments and controllable noise/resonance models. It is not a dependency yet: its runtime, raw-wave tables and broader API offer no first-scene advantage over the DaisySP subset already built for the Brick.
- [Faust libraries](https://github.com/grame-cncm/faustlibraries) provide useful noise, reverb and physical-model families for experiments. The Faust generator is not yet part of the Brick toolchain to keep cross-compilation small.
- [Soundpipe](https://github.com/PaulBatchelor/Soundpipe) is a useful catalogue of compact algorithms, but its upstream is archived. It remains a reference rather than a required dependency.

## Acceptance criteria

Tests cover finite output, limiting, session round-trip, master-gain monotonicity and non-silent output from all twenty-four actors. Musical acceptance also requires:

- at least 30 unattended seconds from every scene;
- clearly different spectra and time structures across all six scenes;
- audible negative space and foreground events rather than permanent maximum density;
- radically different low and high macro states;
- no accidental bell sustain in wood or metal gestures;
- DSP-load measurement and a ten-minute underrun test on TrimUI Brick.

GitHub Actions renders six 30-second WAV files and publishes them as `cursed-drone-soundscape-previews` next to the macOS and PortMaster builds.

# Synthesis source catalogue

This document separates three things that are easy to confuse: an audio algorithm, an instrument preset, and a landscape. A preset can make one engine useful, but a convincing factory, cave or carriage needs several actors with different event laws, spatial roles and timescales.

## Audited open sources

| Source | Useful models | Licence | Brick fit | Decision |
| --- | --- | --- | --- | --- |
| [DaisySP](https://github.com/electro-smith/DaisySP) | `StringVoice`, `ModalVoice`, `Drip`, resonators, noise and filters | MIT, with upstream Plaits and Soundpipe notices | Designed for embedded real-time DSP; a pinned subset is already vendored | `Drip` was audited and added in 0.6 with a documented upstream port correction. Review `StringVoice` after Brick profiling. |
| [Mutable Instruments Plaits firmware](https://github.com/pichenettes/eurorack/tree/master/plaits) | modal, string, particle, speech and percussion engines | MIT per source file | Embedded code, but its internal support graph is broader than one actor | Prefer the already isolated DaisySP ports; import a direct engine only after dependency and CPU review. |
| [STK](https://github.com/thestk/stk) | `Shakers`, `Whistle`, blown pipes, bowed and struck bodies | MIT-style | Mature and portable; some instruments need tables or a larger object graph | Prototype `Shakers` for debris, footsteps and broken mechanisms. Do not vendor the whole toolkit. |
| [Faust libraries](https://github.com/grame-cncm/faustlibraries) | strings, drums, reeds, waveguides, filters and reverbs | LGPL 2.1+ with a generated-code exception; `instruments.lib` contains STK-4.3 material | Generate fixed C++ offline; no Faust runtime on the console | Good route for small self-contained pipe, bow and membrane actors after generated-code audit. |
| [Surge XT](https://github.com/surge-synthesizer/surge) | large synth and factory patch library | GPL-3.0 | The complete engine and asset set are too large for the current target | Listening and parameter-mapping reference only. Preset assets are not imported without an explicit asset licence audit. |

Code and preset assets are licensed separately. The project will not copy a patch bank merely because its host synthesizer is open source.

## Landscape catalogue and backlog

| Landscape | Continuous bed | Foreground actors | Character |
| --- | --- | --- | --- |
| Wet cave (0.6) | low air pressure and filtered water roar | physical drips, rock ticks, distant stream surges | cold, spacious, sparse to overwhelming |
| Metro carriage (0.6) | body and traction-motor resonance | rail joints, braking scrape and carriage rattle | enclosed, kinetic, anxious |
| Substation | mains-related transformer hum | relay chatter, ceramic ticks, electrical arcs | static pressure punctured by dangerous transients |
| Ship engine room | several unsynchronised rotating masses | valve knocks, hull groans, steam leaks | heavy, warm, claustrophobic |
| Frozen marsh | almost silent wind and ice body | ice cracks, dry reeds, distant bird fragments | exposed, brittle, patient |
| Storm drain | turbulent air and low water | pipe knocks, splashes, resonant drops | wet, close, subterranean |
| Broken nursery (0.6) | weak room tone | music-box tines, bent toy speech, gear motor, battery dropout | deliberately toy-like, intimate and frightening |
| Data archive | ventilation and fluorescent buzz | disk seeks, relay clusters, corrupted speech grains | dry, repetitive, increasingly unstable |

These are not skins over one drone. Each landscape needs at least one different excitation model, one different event scheduler and one different spectral centre.

## Transition model

Different engines do not need parameter-to-parameter equivalence. The practical model is two complete scene buses:

1. keep scene A running;
2. instantiate and pre-warm scene B silently;
3. copy the five performance macros as semantic starting coordinates;
4. equal-power crossfade A and B over a chosen duration while both continue their own events;
5. stop A and reclaim its memory after its effect tails end.

Only one destination scene is pre-warmed, so doubled CPU use is temporary. A hard cut, a short rupture and a long morph are the same mechanism with different transition curves and durations.

## Live control target

The concert surface should keep the normal D-pad meaning: left/right selects a track or column, up/down selects a row, shoulders change a value, and a face button performs the current action. Scene changes should use an armed workflow: choose destination, choose duration/curve, arm, then launch. This prevents accidental landscape changes while preserving a one-button performance gesture once armed.

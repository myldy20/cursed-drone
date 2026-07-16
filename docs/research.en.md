# GitHub and DSP research

This snapshot was recorded on 16 July 2026. It looks for reusable architecture, porting precedents and DSP rather than one repository to fork. Activity and star counts change; licenses are re-audited at the exact pinned commit before every import.

## Related applications

| Project | Useful precedent | Why not fork it |
| --- | --- | --- |
| [ZicBox](https://github.com/apiel/zicBox) | embedded Linux groovebox, many engines, independent FX, framebuffer, Pi Zero 2 W | compatible GPLv3, but its tracks/UI and code coupling exceed this product; use as reference |
| [Picoloop](https://github.com/yoyz/picoloop) | SDL, Linux, PSP/Vita/OpenDingux, multiple synth engines | older heterogeneous code and component-level license questions |
| [picoTracker](https://github.com/xiphonics/picoTracker) | modern constrained handheld UX | tracker model and RP2040 target differ |
| [LittleGPTracker](https://github.com/djdiskmachine/LittleGPTracker) | mature handheld and PortMaster patterns | tracker semantics are deliberately out of scope |
| [m8c](https://github.com/laamaa/m8c) / [Brick port](https://github.com/f32-0/m8c-brick-knulli) | exact SDL, controller, packaging and Brick/Knulli precedent | client for another device, not an engine host |
| [OTTO](https://github.com/bitfieldaudio/OTTO) | excellent embedded music UI ideas | CC BY-NC-SA limits distribution and it is no longer a good base |
| [Zynthian UI](https://github.com/zynthian/zynthian-ui) | mature Linux instrument/preset workflow | full plugin/Python stack is too large |
| [norns](https://github.com/monome/norns) | sound-space and modulation UX | runtime/script ecosystem exceeds handheld budget |
| [Organelle OS](https://github.com/critterandguitari/Organelle_OS) | C++/Pure Data instrument precedent | patch-centric runtime is broader than this fixed instrument |

The conclusion is a compact standalone C++/SDL2 shell using proven patterns, not a groovebox fork stripped of tracker/DAW concepts.

## DSP sources

- **[DaisySP](https://github.com/electro-smith/DaisySP), MIT:** primary embedded C++ source for particles, noise, filters and lightweight effects.
- **[Mutable Instruments eurorack](https://github.com/pichenettes/eurorack):** MIT STM32 DSP (with separate GPL AVR areas) for macro, physical-model and granular engines. Import audited files only and follow upstream derivative naming guidance.
- **[Signalsmith Stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch), MIT:** optional heavy pitch/time tier.
- **[Airwindows](https://github.com/airwindows/airwindows), MIT:** source of selectively audited effects.
- **[Soundpipe](https://github.com/PaulBatchelor/Soundpipe), MIT:** broad but archived; reference or selected algorithms only.
- **[Faust libraries](https://github.com/grame-cncm/faustlibraries):** useful generation workflow, with per-function licensing checks.
- **[Google MSFA](https://github.com/google/music-synthesizer-for-android), Apache-2.0:** possible later DX7-style FM engine.
- **[Audible Instruments](https://github.com/VCVRack/AudibleInstruments), GPLv3:** useful wrapper precedent compatible with this project's GPL choice.

The initial product set is `MACRO` (macro oscillator), `BODY` (modal/string/resonator models), `GRAIN` (circular capture and granular freeze), and `PARTICLE` (dust, fractal/clocked noise and bursts). Any of the four slots can host any engine.

`GRAIN` may capture another slot or the master through preallocated circular memory. Feedback always crosses a delayed, gain-bounded path with DC removal and a Panic reset; direct algebraic loops are forbidden.

## Runtime choices

The primary runtime is an original C++ graph with SDL2. [libpd](https://github.com/libpd/libpd) remains an optional future patch engine. A [libretro](https://github.com/libretro/libretro-samples) adapter comes only after standalone stability because frame/audio timing and filesystem constraints weaken instrument UX. Csound, LV2/CLAP hosting, and full ZicBox/Zynthian forks were rejected as disproportionate.

## Controlled corruption

The differentiator is bounded, reproducible disorder rather than algorithm count. `Mutate` makes small changes to unlocked parameters. `Corrupt` may alter routes/engines/effects but respects CPU and feedback bounds and records a deterministic seed. A hardware Panic gesture always restores a safe signal.

## License decision

The project uses GPL-3.0-or-later. This permits GPLv3 adapter reuse alongside MIT and Apache-2.0 DSP with preserved notices. Derived binaries and modified code must remain GPL-compatible; a later proprietary branch cannot be produced from this code without relicensing every contributor's rights.

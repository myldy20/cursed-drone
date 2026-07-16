# Сторонний код / Third-party notices

Начиная с `0.3.0`, репозиторий содержит проверенный поднабор DaisySP. Он собирается непосредственно в core и обеспечивает четыре разных источника. Старый собственный диагностический генератор оставлен только для обратной совместимости с session schema 1/2 и не используется стартовым патчем.

Starting with `0.3.0`, the repository contains an audited DaisySP subset built directly into the core. It powers four distinct sources. The original diagnostic generator remains only for schema 1/2 session compatibility and is no longer used by the default patch.

## DaisySP DSP subset / Поднабор DaisySP

| Project | Files | Pinned commit | License | Local use |
| --- | --- | --- | --- | --- |
| [electro-smith/DaisySP](https://github.com/electro-smith/DaisySP) | `third_party/daisysp/Source/{Utility,Filters,Noise,PhysicalModeling,Synthesis}`; exact list in the local README | `599511b740f8f3a9b8db72a0642aa45b8a23c3a3` | MIT; full text in `third_party/daisysp/LICENSE` | Oscillator/ClockedNoise for `TONE`; ModalVoice/Resonator/Dust for `RESONATOR`; GrainletOscillator for `GRAINLET`; Particle/SVF for `PARTICLES` |

The vendored upstream source files are unmodified. Product glue, macro mapping, event scheduling and the audio graph live in `src/audio.cpp` under this repository's GPL-3.0-or-later license.

## Vendored UI font / Встроенный UI-шрифт

| Project | Files | Pinned commit | License | Local use |
| --- | --- | --- | --- | --- |
| [alexfru/512_8](https://github.com/alexfru/512_8) | `third_party/font512/font_data.inc` | `6bd43ef930697ac54567e9fcf59e6ffc24c6813e` | Unlicense / public domain | 512-glyph 8×8 bold bitmap font; UTF-8 UI maps Latin and Russian glyphs |

The unmodified upstream initializer data and full license text are stored beside the font.

## Planned dependencies / Планируемые зависимости

| Project | Intended use | Upstream license | Integration rule |
| --- | --- | --- | --- |
| [Mutable Instruments eurorack](https://github.com/pichenettes/eurorack) | macro, physical-model and granular engines | STM32 code: MIT; AVR areas: GPL-3.0 | import only audited files; preserve notices; follow naming guidelines |
| [Signalsmith Stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) | optional heavy pitch/time effect | MIT | optional build feature and CPU warning |
| [Airwindows](https://github.com/airwindows/airwindows) | selectively audited effects | MIT | import individual effects, not the entire collection |
| [Google MSFA](https://github.com/google/music-synthesizer-for-android) | possible DX7-compatible FM engine | Apache-2.0 | optional later engine; preserve NOTICE |
| [Soundpipe](https://github.com/PaulBatchelor/Soundpipe) | algorithm reference or selected effects | MIT | avoid framework dependency; repo is archived |
| [Faust libraries](https://github.com/grame-cncm/faustlibraries) | generated experimental effects | mixed/per-function | audit each algorithm and generated output separately |

GPL compatibility does not remove attribution obligations. Every imported source set must record: upstream URL, exact commit, file list, upstream license, local modifications and a repeatable update procedure.

The product and engine labels must not imply endorsement by Mutable Instruments. In particular, this project is not named after Mutable Instruments and does not market engines using the original module product names.

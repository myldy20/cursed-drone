# Сторонний код / Third-party notices

Начиная с `0.3.0`, репозиторий содержит проверенный поднабор DaisySP. Он собирается непосредственно в core. В 0.4 прежние четыре источника сохранены для экспертного режима, а DaisySP SVF также используется двенадцатью процедурными актёрами. Старый диагностический генератор оставлен только для обратной совместимости.

Starting with `0.3.0`, the repository contains an audited DaisySP subset built directly into the core. In 0.4 the original four sources remain available in expert mode, while DaisySP SVF also filters the twelve procedural actors. The diagnostic generator remains only for backward compatibility.

## DaisySP DSP subset / Поднабор DaisySP

| Project | Files | Pinned commit | License | Local use |
| --- | --- | --- | --- | --- |
| [electro-smith/DaisySP](https://github.com/electro-smith/DaisySP) | `third_party/daisysp/Source/{Utility,Filters,Noise,PhysicalModeling,Synthesis}`; exact list in the local README | `599511b740f8f3a9b8db72a0642aa45b8a23c3a3` | MIT; full text in `third_party/daisysp/LICENSE` | Existing `TONE/RESONATOR/GRAINLET/PARTICLES`; stable SVF filtering for procedural wind, surfaces, crowd, machinery and friction |

The vendored upstream source files are unmodified. Product glue, macro mapping, event scheduling and the audio graph live in `src/audio.cpp` and `src/soundscape.cpp` under this repository's GPL-3.0-or-later license. The Designing Sound, STK, Faust and Soundpipe links in the research documents are references only; no source from them is vendored in 0.4.

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

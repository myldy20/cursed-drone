# Сторонний код / Third-party notices

Начиная с `0.3.0`, репозиторий содержит проверенный поднабор DaisySP. Он собирается непосредственно в core. В 0.6 прежние четыре источника сохранены для экспертного режима, SVF используется процедурными актёрами, а модель `Drip` создаёт капли мокрой пещеры. Старый диагностический генератор оставлен только для обратной совместимости.

Starting with `0.3.0`, the repository contains an audited DaisySP subset built directly into the core. In 0.6 the original four sources remain available in expert mode, SVF filters procedural actors, and the `Drip` model creates the Wet Cave drops. The diagnostic generator remains only for backward compatibility.

## DaisySP DSP subset / Поднабор DaisySP

| Project | Files | Pinned commit | License | Local use |
| --- | --- | --- | --- | --- |
| [electro-smith/DaisySP](https://github.com/electro-smith/DaisySP) | `third_party/daisysp/Source/{Utility,Filters,Noise,PhysicalModeling,Synthesis}`; exact list in the local README | `599511b740f8f3a9b8db72a0642aa45b8a23c3a3` | MIT; full text in `third_party/daisysp/LICENSE` | `TONE/RESONATOR/GRAINLET/PARTICLES`, SVF material filters and Perry Cook/Soundpipe `Drip` water model |

The local `Drip` copy has a documented two-line resonator correction and three parameter setters; details are in the subset README. Other vendored upstream files remain unmodified. Product glue, macro mapping, event scheduling and the audio graph live in `src/audio.cpp` and `src/soundscape.cpp` under this repository's GPL-3.0-or-later license. The Designing Sound, STK and Faust links in the research documents are references only.

## Experimental macro-oscillator / Экспериментальный macro-oscillator

| Project | Files | Pinned commit | License | Local use |
| --- | --- | --- | --- | --- |
| [pichenettes/eurorack](https://github.com/pichenettes/eurorack) | Git submodule `third_party/eurorack`; build is restricted to `plaits/dsp`, generated `plaits/resources.cc` and required `stmlib` DSP/utility files | `08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4` | MIT notices are preserved in every compiled upstream source file | Original Plaits `Voice` wrapped by `src/plaits_actor.cpp`; curated musical models, OUT/AUX routing and Scala quantisation |

This dependency exists only in the experimental `0.10` branch. The upstream DSP source is not renamed or presented as original Cursed Drone code. Cursed Drone uses neutral UI labels and does not imply endorsement by Mutable Instruments. The wrapper, output routing, tuning system, event integration and handheld UI are GPL-3.0-or-later project code.

Эта зависимость используется только в экспериментальной ветке `0.10`. Оригинальный DSP-код не переименовывается и не выдаётся за разработку Cursed Drone. Нейтральные названия интерфейса не подразумевают одобрение со стороны Mutable Instruments.

## Vendored UI font / Встроенный UI-шрифт

| Project | Files | Pinned commit | License | Local use |
| --- | --- | --- | --- | --- |
| [alexfru/512_8](https://github.com/alexfru/512_8) | `third_party/font512/font_data.inc` | `6bd43ef930697ac54567e9fcf59e6ffc24c6813e` | Unlicense / public domain | 512-glyph 8×8 bold bitmap font; UTF-8 UI maps Latin and Russian glyphs |

The unmodified upstream initializer data and full license text are stored beside the font.

## Possible later dependencies / Возможные будущие зависимости

| Project | Intended use | Upstream license | Integration rule |
| --- | --- | --- | --- |
| [Signalsmith Stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) | optional heavy pitch/time effect | MIT | optional build feature and CPU warning |
| [Airwindows](https://github.com/airwindows/airwindows) | selectively audited effects | MIT | import individual effects, not the entire collection |
| [Google MSFA](https://github.com/google/music-synthesizer-for-android) | possible DX7-compatible FM engine | Apache-2.0 | optional later engine; preserve NOTICE |
| [Soundpipe](https://github.com/PaulBatchelor/Soundpipe) | algorithm reference or selected effects | MIT | avoid framework dependency; repo is archived |
| [Faust libraries](https://github.com/grame-cncm/faustlibraries) | generated experimental effects | mixed/per-function | audit each algorithm and generated output separately |

GPL compatibility does not remove attribution obligations. Every imported source set must record: upstream URL, exact commit, file list, upstream license, local modifications and a repeatable update procedure.

The product and engine labels must not imply endorsement by Mutable Instruments. In particular, this project is not named after Mutable Instruments and does not market engines using the original module product names.

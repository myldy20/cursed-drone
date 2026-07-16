# Сторонний код / Third-party notices

На стадии `0.2.0` репозиторий **не содержит импортированного DSP-кода**. Исполняемый прототип использует стандартную библиотеку C++, системный SDL2 (когда он доступен), собственный диагностический генератор и public-domain bitmap-шрифт. Генератор предназначен для проверки аудиографа и будет скрыт из пользовательского списка после подключения продуктовых движков.

At `0.2.0`, the repository contains **no imported DSP implementation**. The executable prototype uses the C++ standard library, system SDL2 when present, an original diagnostic signal generator, and a public-domain bitmap font. The diagnostic source will be hidden from the product engine list once real engines land.

## Vendored UI font / Встроенный UI-шрифт

| Project | Files | Pinned commit | License | Local use |
| --- | --- | --- | --- | --- |
| [alexfru/512_8](https://github.com/alexfru/512_8) | `third_party/font512/font_data.inc` | `6bd43ef930697ac54567e9fcf59e6ffc24c6813e` | Unlicense / public domain | 512-glyph 8×8 bold bitmap font; UTF-8 UI maps Latin and Russian glyphs |

The unmodified upstream initializer data and full license text are stored beside the font.

## Planned dependencies / Планируемые зависимости

| Project | Intended use | Upstream license | Integration rule |
| --- | --- | --- | --- |
| [DaisySP](https://github.com/electro-smith/DaisySP) | particle/noise DSP, filters and lightweight effects | MIT | vendor pinned commit; retain copyright/license |
| [Mutable Instruments eurorack](https://github.com/pichenettes/eurorack) | macro, physical-model and granular engines | STM32 code: MIT; AVR areas: GPL-3.0 | import only audited files; preserve notices; follow naming guidelines |
| [Signalsmith Stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) | optional heavy pitch/time effect | MIT | optional build feature and CPU warning |
| [Airwindows](https://github.com/airwindows/airwindows) | selectively audited effects | MIT | import individual effects, not the entire collection |
| [Google MSFA](https://github.com/google/music-synthesizer-for-android) | possible DX7-compatible FM engine | Apache-2.0 | optional later engine; preserve NOTICE |
| [Soundpipe](https://github.com/PaulBatchelor/Soundpipe) | algorithm reference or selected effects | MIT | avoid framework dependency; repo is archived |
| [Faust libraries](https://github.com/grame-cncm/faustlibraries) | generated experimental effects | mixed/per-function | audit each algorithm and generated output separately |

GPL compatibility does not remove attribution obligations. Every imported source set must record: upstream URL, exact commit, file list, upstream license, local modifications and a repeatable update procedure.

The product and engine labels must not imply endorsement by Mutable Instruments. In particular, this project is not named after Mutable Instruments and does not market engines using the original module product names.

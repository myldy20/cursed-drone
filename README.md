<p align="center"><img src="assets/branding/cursed-drone-banner.svg" alt="Cursed Drone — developed by Myldy design" width="100%"></p>
<p align="center">
<a href="https://github.com/myldy20/cursed-drone/releases/latest"><img src="https://img.shields.io/badge/release-1.0.0-eee2c5" alt="release 1.0.0"></a>
<img src="https://img.shields.io/badge/status-feature--complete-50a99a" alt="feature complete">
<img src="https://img.shields.io/badge/lifecycle-maintenance-6b7280" alt="maintenance">
<img src="https://img.shields.io/badge/verified-TrimUI_Brick_/_Pixel_8_Pro_/_Web-7550ab" alt="verified platforms">
<a href="LICENSE"><img src="https://img.shields.io/badge/license-GPL--3.0--or--later-555555" alt="GPL-3.0-or-later"></a>
</p>

# Cursed Drone

**Cursed Drone is a feature-complete handheld, Android and browser-based live soundscape instrument.** It creates the atmosphere of a place populated by four procedural actors, evolving events, modulation and long effect tails.

Version **1.0.0** closes the standalone product roadmap. The repository is now maintained for reproducible defects, data loss, serious audio failures and supported-platform compatibility. New instrument concepts and larger performance-system features will be developed as modules for **BRKSTN** rather than added here.

## Launch, download and install

**[Launch the WebAssembly version](https://myldy20.github.io/cursed-drone/)** · **[Download Cursed Drone 1.0.0](https://github.com/myldy20/cursed-drone/releases/latest)** · [1.0.0 release notes](docs/releases/v1.0.0.md)

- **Web:** modern WebAssembly/Web Audio browser; mouse, drag gestures and touch are supported;
- **Knulli / PortMaster:** [English](docs/install.en.md) · [Русский](docs/install.ru.md)
- **NextUI:** [English](docs/install.nextui.en.md) · [Русский](docs/install.nextui.ru.md)
- **Android ARM64 sideload:** [English](docs/install.android.en.md) · [Русский](docs/install.ru.md)
- **macOS Apple Silicon:** the native 1.0.0 archive remains available in the GitHub release; the WebAssembly build is the maintained macOS path under the self-hosted CI standard.

The verified 1.0.0 PortMaster/Knulli, NextUI and native macOS packages are frozen release artifacts. New native packages for those architectures require an explicitly approved matching self-hosted toolchain; GitHub-hosted fallback runners are not used.

The Android APK is release-optimised but remains a manually installed package rather than a Google Play production release. Android and Web share the same adaptive fullscreen interface; TrimUI and desktop SDL use the controller-first 512×384 interface.

## Guided workflow

`PLACE → ACTOR → FX → MASTER → MEMORY`

- **Place:** ten landscape recipes, five performance macros and four actor levels/mutes.
- **Actor:** landscape engines or the **Musical** source with 16 curated macro models, MAIN/AUX/MIX/STEREO routing, Scala tuning, Euclidean triggers and modulation.
- **FX:** four serial effects per actor.
- **Master:** level, tempo and four post-mix effects for long tails and transitions.
- **Memory:** autosave plus eight explicit user slots and landscape restore.

Detailed guide: [English](docs/workflow.en.md) · [Русский](docs/workflow.ru.md)

## Features

- four procedural actors and ten places;
- 34 engines, including Sub Drone, Tape Drone, Bowed Metal, Earth Rumble and Musical;
- 16 Musical macro models with separate MAIN and AUX signals;
- four modulation rows per actor with bipolar depth and bounded cross-modulation;
- manual and Euclidean triggering with visible event feedback;
- built-in and user Scala `.scl` tuning;
- 21 effects, including Reverse Grains and five compound ambient processors;
- four Actor FX plus four Master FX;
- autosave, eight memories, atomic writes, sanitisation and `.bak` recovery;
- English and Russian interfaces;
- one shared C++ core, session format and CMake graph;
- no recorded samples.

## Field-tested performance

The maintainer tested the 1.0 release candidate on all primary targets:

- **TrimUI Brick:** about 50% load in ordinary use; the highest observed load stayed below 80%;
- **Pixel 8 Pro:** low observed load with no noticeable audio or lifecycle failures;
- **macOS browser:** low observed load with no noticeable audio or interaction failures.

These are practical observations, not laboratory guarantees. Firmware, CPU clock, browser, audio routing and patch complexity affect the result.

For live use, rehearse the exact build and memories, avoid last-minute updates and keep a fallback audio source. No release-blocking defect is known at publication time, but Cursed Drone is independent experimental music software rather than certified stage infrastructure.

[Support and log locations](docs/support.en.md) · [Поддержка и логи](docs/support.ru.md)

## Musical quick reference

The four Musical macros are model-dependent:

- **Harmonics:** harmonic structure or internal variant;
- **Timbre:** primary spectral axis;
- **Morph:** secondary model-specific axis;
- **Decay:** envelope, articulation or damping.

Output routing is exact:

- **MAIN:** main signal in mono;
- **AUX:** alternate signal in mono;
- **MIX:** average of MAIN and AUX in mono;
- **STEREO:** MAIN left, AUX right.

All 16 model descriptions: [Synthesis catalogue](docs/synthesis-catalog.en.md) · [Каталог синтеза](docs/synthesis-catalog.ru.md)

## Controls on TrimUI / desktop SDL

| Button | Meaning |
| --- | --- |
| D-pad | navigate; edit the selected value; hold to accelerate |
| A | open, confirm, perform the selected action, or mute an actor |
| B | back/cancel; hold for emergency Kill |
| X | next focus section on the current page |
| Y | contextual help |
| L / R | previous / next page |
| Select | fade the final output |
| Start | quick menu |
| Start + Select | save the current state and exit |

## Runtime data

Knulli: `curseddrone/conf/` · NextUI: `.userdata/tg5040/cursed-drone/` · Web: browser IndexedDB · Android: private app storage.

Runtime data includes `autosave.cdrone`, `memory-1.cdrone` … `memory-8.cdrone`, and optional `scales/*.scl`. If a primary session is corrupt, Cursed Drone attempts to recover its previous atomic `.bak` copy.

## Build and CI

```bash
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

WebAssembly:

```bash
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web --target cursed-drone-web --parallel 2
```

Heavy repository CI runs on the existing home self-hosted build class `[self-hosted, myldy-home]`; the Selectel production VPS is reserved for small activation/production checks through `[self-hosted, myldy-vps, cursed-drone]`. Same-repository pull requests can run CI; untrusted fork PR code is never executed on a self-hosted runner. Mandatory maintenance CI validates the portable Linux core, Android ARM64 and WebAssembly. Browser smoke tests cover mouse, touch, drag gestures, DPR 1/2 and a letterboxed Retina viewport. Heavy jobs check disk space and clean build trees, SDK/toolchain temp data, browser profiles and child processes even on failure/cancellation.

The Myldy web preview is built/tested on the home runner, transferred as a compact one-day Actions artifact, then verified and atomically activated on the VPS from `preview` or `workflow_dispatch`. The VPS does not rebuild the app. Production retention is `current + 2` rollback releases, and failed builds cannot change `current`. GitHub Pages remains enabled until the Myldy-hosted preview is separately accepted; its WASM build runs at home and only the small Pages activation job uses the VPS.

## Documentation

- [Changelog](CHANGELOG.md) · [1.0.0 release notes](docs/releases/v1.0.0.md)
- [Workflow](docs/workflow.en.md) · [Сквозная логика](docs/workflow.ru.md)
- [Synthesis catalogue](docs/synthesis-catalog.en.md) · [Каталог синтеза](docs/synthesis-catalog.ru.md)
- [Effects](docs/effects.en.md) · [Эффекты](docs/effects.ru.md)
- [Architecture](docs/architecture.en.md) · [Архитектура](docs/architecture.ru.md)
- [Support](docs/support.en.md) · [Поддержка](docs/support.ru.md)
- [Deployment](docs/deployment.en.md) · [Развёртывание](docs/deployment.ru.md)
- [Completed roadmap](docs/roadmap.en.md) · [Завершённая дорожная карта](docs/roadmap.ru.md)

## Credits and licence

Developed by **Myldy design — [@myldy20](https://github.com/myldy20)**. First-party code is GPL-3.0-or-later with the attribution, origin-marking and trademark terms in [ADDITIONAL_TERMS.md](ADDITIONAL_TERMS.md). Modified distributions must preserve [NOTICE.md](NOTICE.md), identify themselves as based on Cursed Drone and clearly state that they are unofficial.

The Musical source compiles selected MIT-licensed DSP from Mutable Instruments Plaits. Copyright and permission notices are retained in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and `third_party/PLAITS_LICENSE.txt`. Cursed Drone is independent and is not affiliated with or endorsed by Mutable Instruments or Emilie Gillet.

---

# Русский

**Проклятый гудёж — функционально завершённый карманный, Android- и браузерный инструмент живых звуковых пространств.** Он создаёт атмосферу места с четырьмя процедурными актёрами, событиями, модуляцией и длинными хвостами эффектов.

Версия **1.0.0** завершает самостоятельную дорожную карту продукта. Дальше репозиторий поддерживается ради воспроизводимых ошибок, потери данных, серьёзных сбоев звука и совместимости. Новые инструменты и крупные исполнительские идеи будут развиваться как модули **BRKSTN**, а не наращиваться здесь.

## Запустить и скачать

**[Веб-версия](https://myldy20.github.io/cursed-drone/)** · **[последний релиз](https://github.com/myldy20/cursed-drone/releases/latest)** · [что нового в 1.0.0](docs/releases/v1.0.0.md)

Проверенные варианты: TrimUI Brick с Knulli/PortMaster и NextUI, Pixel 8 Pro, нативный macOS 1.0.0, Android ARM64 и современные WebAssembly/Web Audio-браузеры. Проверенные нативные пакеты 1.0 для handheld и macOS остаются замороженными релизными артефактами; GitHub-hosted fallback для их пересборки не используется.

## Полевой ориентир

На TrimUI Brick обычная загрузка составляет около 50%, максимальная замеченная оставалась ниже 80%. На Pixel 8 Pro и в браузере macOS нагрузка существенно ниже; заметных блокирующих проблем со звуком или управлением при проверке не обнаружено.

Это не гарантия для любой прошивки, аудиосхемы и патча. Перед концертом отрепетируй именно эту сборку и слоты памяти, не обновляй всё в последний момент и держи резервный источник звука.

[Поддержка и расположение логов](docs/support.ru.md) · [English](docs/support.en.md)

## Инфраструктура

Тяжёлые CI/build jobs выполняются на домашнем self-hosted build class `[self-hosted, myldy-home]`; Selectel VPS используется только для маленьких production-facing операций через `[self-hosted, myldy-vps, cursed-drone]`. Недоверенный fork PR код на self-hosted runners не исполняется. Linux core, Android ARM64, WebAssembly и Playwright/browser smoke строятся и тестируются дома; workflow проверяет свободное место и обязан убрать build trees, временные SDK/toolchains, browser profiles и child processes даже после failure/cancellation.

Preview `myldy.ru` собирается и тестируется на домашнем runner, передаётся на VPS как компактный artifact с retention один день, затем только проверяется и атомарно активируется. VPS приложение заново не собирает. Production retention — `current + 2` rollback releases. GitHub Pages пока остаётся включённым; тяжёлая WASM-сборка Pages идёт на домашнем runner, маленькая activation job — на VPS.

[Развёртывание](docs/deployment.ru.md) · [English](docs/deployment.en.md)

## Статус разработки

Cursed Drone остаётся маленьким и законченным инструментом. После 1.0 принимаются исправления падений, потери данных, серьёзных аудиосбоев, совместимости, сборки, лицензий и документации. Новые функции и экспериментальные порты в план сопровождения не входят.

## Лицензия и Plaits

Разработано **Myldy design — [@myldy20](https://github.com/myldy20)**. Собственный код распространяется по GPL-3.0-or-later с дополнительными условиями об атрибуции, происхождении и брендинге в [ADDITIONAL_TERMS.md](ADDITIONAL_TERMS.md).

Источник «Музыкальный» использует выбранные части DSP Mutable Instruments Plaits по MIT. Уведомления сохранены в [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) и `third_party/PLAITS_LICENSE.txt`. Cursed Drone — самостоятельный проект, не связанный с Mutable Instruments и не одобренный Mutable Instruments или Эмили Жилле.

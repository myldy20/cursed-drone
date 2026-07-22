<p align="center"><img src="assets/branding/cursed-drone-banner.svg" alt="Cursed Drone — developed by Myldy design" width="100%"></p>
<p align="center">
<a href="https://github.com/myldy20/cursed-drone/actions/workflows/build.yml"><img src="https://github.com/myldy20/cursed-drone/actions/workflows/build.yml/badge.svg" alt="build"></a>
<img src="https://img.shields.io/badge/version-0.12.3-eee2c5" alt="version 0.12.3">
<img src="https://img.shields.io/badge/verified-TrimUI_Brick_Knulli-50a99a" alt="verified Knulli">
<img src="https://img.shields.io/badge/verified-TrimUI_Brick_NextUI-50a99a" alt="verified NextUI">
<img src="https://img.shields.io/badge/architecture-AArch64-7550ab" alt="AArch64">
</p>

# Cursed Drone

**Cursed Drone is a handheld live soundscape instrument.** It does not merely generate a pure drone: it creates the atmosphere of a place, populated by four procedural actors and evolving events.

> Verified on real **TrimUI Brick** hardware with both **Knulli/PortMaster** and **NextUI**. Packages are firmware-specific and not interchangeable.

## Install

- Knulli / PortMaster: [English](docs/install.en.md) · [Русский](docs/install.ru.md)
- NextUI: [English](docs/install.nextui.en.md) · [Русский](docs/install.nextui.ru.md)

## Guided Workflow

`PLACE → ACTOR → FX → MASTER → MEMORY`

- **Place:** ten landscape recipes, five performance macros and four actor levels/mutes.
- **Actor:** landscape engines or the **Musical** source with 16 curated upstream macro models, MAIN/AUX/MIX/STEREO routing and Scala tuning. Event actors can be triggered immediately and have their own Event Rate.
- **FX:** four serial effects per actor, edited in one unified slot view.
- **Master:** level, tempo and four post-mix effects for long tails and transitions.
- **Memory:** autosave plus eight explicit user slots and landscape restore.

Detailed guide: [English](docs/workflow.en.md) · [Русский](docs/workflow.ru.md)

## Features

- four procedural actors and ten places;
- 34 engines, including Sub Drone, Tape Drone, Bowed Metal, Earth Rumble and the neutral **Musical** actor;
- four modulation rows per actor, bipolar depth and bounded rate cross-modulation;
- manual triggering and visible activity flashes for event actors;
- per-actor Event Rate plus the global Events macro;
- Euclidean event generation;
- built-in and user Scala `.scl` tuning;
- 21 effects including Reverse Grains and five compound drone/ambient processors;
- four actor FX plus four Master FX;
- eight memories, autosave, English/Russian UI;
- native SDL UI at 512×384, scaled exactly to the Brick display;
- no recorded samples.

## Controls

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

Knulli: `curseddrone/conf/` · NextUI: `.userdata/tg5040/cursed-drone/`.
Both contain `autosave.cdrone`, `memory-1.cdrone` … `memory-8.cdrone`, and optional `scales/*.scl`.

## Build

```bash
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

CI tests Linux, macOS and Ubuntu-20.04-compatible AArch64 and packages both PortMaster and NextUI.

## Documentation

- [Workflow](docs/workflow.en.md) · [Сквозная логика](docs/workflow.ru.md)
- [Architecture](docs/architecture.en.md) · [Архитектура](docs/architecture.ru.md)
- [Synthesis catalogue](docs/synthesis-catalog.en.md) · [Каталог синтеза](docs/synthesis-catalog.ru.md)
- [Effects](docs/effects.en.md) · [Эффекты](docs/effects.ru.md)
- [TrimUI Brick](docs/trimui-brick.en.md) · [TrimUI Brick](docs/trimui-brick.ru.md)
- [Roadmap](docs/roadmap.en.md) · [Дорожная карта](docs/roadmap.ru.md)

## Credits and licence

Developed by **Myldy design — [@myldy20](https://github.com/myldy20)**. First-party code is GPL-3.0-or-later with the narrowly scoped attribution, origin-marking and trademark terms in [ADDITIONAL_TERMS.md](ADDITIONAL_TERMS.md). Modified distributions must preserve [NOTICE.md](NOTICE.md) and identify themselves as based on Cursed Drone and unofficial. Bundled third-party code retains its own licences; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

---

# Русский

**Проклятый гудёж — карманный инструмент живых звуковых пространств.** Он не просто выдаёт чистый дрон, а создаёт атмосферу места, населённого четырьмя процедурными актёрами и развивающимися событиями.

> Проверено на реальной **TrimUI Brick** с **Knulli/PortMaster** и **NextUI**. Пакеты для разных прошивок не взаимозаменяемы.

## Установка

- Knulli / PortMaster: [русская инструкция](docs/install.ru.md) · [English](docs/install.en.md)
- NextUI: [русская инструкция](docs/install.nextui.ru.md) · [English](docs/install.nextui.en.md)

## Сквозная логика

`МЕСТО → АКТЁР → FX → МАСТЕР → ПАМЯТЬ`

- **Место:** десять ландшафтов, пять исполнительских макросов, уровни и mute четырёх актёров.
- **Актёр:** движки ландшафта или **Музыкальный** источник с 16 моделями, MAIN/AUX/MIX/STEREO и Scala-строями. Событийного актёра можно запустить сразу и отдельно настроить частоту событий.
- **FX:** четыре последовательных эффекта актёра в едином редакторе.
- **Мастер:** уровень, темп и четыре эффекта после сведения.
- **Память:** autosave, восемь явных слотов и восстановление рецепта ландшафта.

Подробно: [сквозной workflow](docs/workflow.ru.md) · [English](docs/workflow.en.md)

## Возможности

- четыре процедурных актёра и десять мест;
- 34 движка, включая саб-дрон, ленточный дрон, смычковый металл, гул земли и нейтрально названный **Музыкальный** источник;
- четыре строки модуляции на актёра, биполярная глубина и ограниченная cross-modulation скорости;
- ручной запуск и видимая вспышка событийных актёров;
- отдельная частота событий актёра плюс глобальный макрос «События»;
- Euclidean-события;
- встроенные и пользовательские Scala-файлы `.scl`;
- 21 эффект, включая Reverse Grains и пять составных процессоров;
- четыре actor FX и четыре Master FX;
- восемь слотов памяти, autosave, русский и английский интерфейс;
- нативный SDL-интерфейс 512×384 без записанных семплов.

## Управление

| Кнопка | Значение |
| --- | --- |
| D-pad | навигация и изменение значения; удержание ускоряет шаг |
| A | открыть, подтвердить, выполнить действие или заглушить актёра |
| B | назад/отмена; удержание — аварийный Kill |
| X | следующий раздел текущей страницы |
| Y | контекстная помощь |
| L / R | предыдущая / следующая страница |
| Select | fade итогового выхода |
| Start | быстрое меню |
| Start + Select | сохранить состояние и выйти |

## Данные

Knulli: `curseddrone/conf/` · NextUI: `.userdata/tg5040/cursed-drone/`.
Там находятся `autosave.cdrone`, `memory-1.cdrone` … `memory-8.cdrone` и пользовательские `scales/*.scl`.

Разработано **Myldy design — [@myldy20](https://github.com/myldy20)**. Собственный код проекта распространяется по GPL-3.0-or-later с узкими дополнительными условиями об атрибуции, обозначении происхождения и брендинге в [ADDITIONAL_TERMS.md](ADDITIONAL_TERMS.md). Производные версии должны сохранять [NOTICE.md](NOTICE.md), указывать основу Cursed Drone и явно обозначаться как неофициальные. Лицензии сторонних компонентов перечислены в [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

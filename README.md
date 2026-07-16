# Проклятый гудёж / Cursed Drone

[![build](https://github.com/myldy20/cursed-drone/actions/workflows/build.yml/badge.svg)](https://github.com/myldy20/cursed-drone/actions/workflows/build.yml)

**RU** · [English below](#english)

`Cursed Drone` — нативный звуковой инструмент для маленьких Linux-консолей. Его задача — создавать дроны, шумовые поля, пульсации и управляемые звуковые катастрофы без трекерной сетки и без необходимости программировать синтезатор.

Главная целевая консоль — **TrimUI Brick**. Архитектура не привязана к одной прошивке: SDL2-оболочка и компактное C++-ядро рассчитаны также на Anbernic, PortMaster-совместимые устройства и обычный Linux desktop.

> Статус: ранний исполняемый прототип `0.1.0`. Аудиограф, сессии, автоматизации, базовые FX и двуязычная инфраструктура работают. Текущий генератор звука — только диагностический тест тракта. Продуктовые движки DaisySP/Mutable Instruments подключаются на следующем этапе.

## Идея

На экране всегда четыре независимых звуковых слота. Каждый слот имеет один синтезаторный движок, четыре последовательных эффекта и четыре независимых модулятора:

```text
Движок → FX 1 → FX 2 → FX 3 → FX 4 → уровень/панорама
    ↑       ↑      ↑      ↑      ↑
        четыре свободных модулятора

4 слота → микшер → DC blocker → мягкий лимитер → SDL audio
```

Это «около-трекерный» инструмент без дорожек, нотной таблицы и транспорта. Время существует как общий пульс, LFO, случайные процессы и вероятностные события.

## Уже работает

- четыре одновременно работающих аудиослота;
- четыре FX-ячейки на слот: drive, low-pass, tremolo, delay, crusher и bypass;
- четыре модулятора на слот: sine, triangle, sample & hold и random walk;
- назначения на pitch, timbre, color, motion, texture, level, pan и amount любого FX;
- bounded SPSC mailbox между UI и аудиопотоком;
- сглаживание параметров, DC blocker и master soft limiter;
- сохранение/загрузка человекочитаемых `.cdrone`-сессий;
- русский и английский каталоги строк, переключаемые без перезапуска;
- CLI offline-render в stereo 48 kHz float WAV;
- SDL2-прототип с логическим разрешением 512×384, масштабируемым до 1024×768;
- тесты очереди, локализации, аудиографа и round-trip сессии.

## Сборка

Требования к ядру: C++20 и `make` или CMake 3.16+. SDL2 нужен только для интерактивного приложения.

```bash
make -j2
make test
make render
```

Результаты:

- `build/cursed-drone` — CLI и offline-render;
- `build/cursed-drone-tests` — тесты;
- `build/cursed-drone-render` — генератор демонстрационного WAV;
- `out/cursed-drone-demo.wav` — 12-секундная проверка полного графа.

При установленном `sdl2-config`:

```bash
make sdl
./build/cursed-drone-sdl
```

Альтернативная сборка:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

## CLI

```bash
# справка по-русски
./build/cursed-drone --lang ru

# сохранить стартовый патч
./build/cursed-drone --save-default default.cdrone

# загрузить сессию и отрендерить 30 секунд
./build/cursed-drone --load default.cdrone --render drone.wav --seconds 30
```

## Управление SDL-прототипом

| Действие | Клавиатура | Планируемая кнопка консоли |
| --- | --- | --- |
| выбрать слот | ← / → | D-pad ← / → |
| выбрать параметр | ↑ / ↓ | D-pad ↑ / ↓ |
| изменить значение | A / D | L / R |
| mute слота | Space | A |
| RU / EN | L | Select + Y |
| выход | Esc | Menu |

На текущем этапе экран визуализирует четыре слота и значения без шрифтового рендера; выбранный локализованный параметр показывается в заголовке окна. Полный bitmap/font UI — часть следующего этапа.

## Продуктовые движки

План первой версии:

| Имя в Cursed Drone | Основа | Роль |
| --- | --- | --- |
| `MACRO` | Plaits DSP | широкий макро-осциллятор |
| `BODY` | Elements/Rings DSP | физическое моделирование и резонанс |
| `GRAIN` | Clouds DSP | гранулярная память, freeze и перезапись других слотов |
| `PARTICLE` | DaisySP Dust/Particle/ClockedNoise/Grainlet | шум, частицы и нестабильные импульсы |

Мы не называем продукт или движки именем **Mutable Instruments** и не используем оригинальные названия модулей как торговые имена. Это соответствует опубликованным правилам автора исходного DSP.

## Документация

- [Архитектура](docs/architecture.ru.md) · [Architecture](docs/architecture.en.md)
- [Исследование проектов и DSP](docs/research.ru.md) · [Research](docs/research.en.md)
- [Дорожная карта](docs/roadmap.ru.md) · [Roadmap](docs/roadmap.en.md)
- [TrimUI Brick и портирование](docs/trimui-brick.ru.md) · [TrimUI Brick and porting](docs/trimui-brick.en.md)
- [Тестирование на macOS](docs/testing-macos.ru.md) · [Testing on macOS](docs/testing-macos.en.md)
- [Лицензии и сторонний код](THIRD_PARTY_NOTICES.md)

## Лицензия

Код проекта распространяется по **GNU GPL v3.0 or later**. См. [LICENSE](LICENSE). Сторонние движки сохраняют собственные лицензии и уведомления; каждый импорт проходит отдельную проверку.

---

## English

`Cursed Drone` is a native sound instrument for small Linux handhelds. It creates drones, noise fields, pulses and controllable sonic disasters without a tracker grid or synth programming.

The primary target is the **TrimUI Brick**, while the SDL2 shell and compact C++ core are intended to remain portable to Anbernic devices, PortMaster systems and desktop Linux.

> Status: early executable prototype `0.1.0`. The audio graph, sessions, modulation, basic effects and bilingual infrastructure work. The current sound source is a diagnostic signal-path probe only; DaisySP and permissively licensed Mutable Instruments DSP adapters come next.

Each of four independent slots contains one engine, four serial effects and four free modulation lanes. The mixer ends in DC removal and a soft master limiter. There is a shared sense of time, but no notes table, transport or tracker sequencing.

### Build

The core needs C++20 and Make or CMake 3.16+. SDL2 is only required by the interactive frontend.

```bash
make -j2
make test
make render

# when sdl2-config is installed
make sdl
./build/cursed-drone-sdl
```

See the [English architecture](docs/architecture.en.md), [research](docs/research.en.md), [roadmap](docs/roadmap.en.md), [TrimUI Brick notes](docs/trimui-brick.en.md), and [third-party notices](THIRD_PARTY_NOTICES.md).

The project is licensed under **GNU GPL v3.0 or later**.

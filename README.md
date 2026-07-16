# Проклятый гудёж / Cursed Drone

[![build](https://github.com/myldy20/cursed-drone/actions/workflows/build.yml/badge.svg)](https://github.com/myldy20/cursed-drone/actions/workflows/build.yml)

**RU** · [English below](#english)

`Cursed Drone` — нативный генератор процедурных звуковых ландшафтов для маленьких Linux-консолей. Его задача — создавать фон, движения, редкие жесты и управляемые звуковые катастрофы без трекерной сетки и без необходимости программировать синтезатор.

Главная целевая консоль — **TrimUI Brick**. Архитектура не привязана к одной прошивке: SDL2-оболочка и компактное C++-ядро рассчитаны также на Anbernic, PortMaster-совместимые устройства и обычный Linux desktop.

> Статус: экспериментальный прототип `0.4.0`. Вместо стартового набора из четырёх похожих синтезаторов появились три процедурных ландшафта: заброшенное помещение, цех и пустошь. Каждый состоит из фонового слоя и событийных актёров со своей памятью, огибающими и временными законами.

## Идея

На экране всегда четыре звуковых актёра текущего ландшафта. Они намеренно выполняют разные роли: непрерывная среда, движение, передний жест и дальний/текстурный слой. В детальном режиме каждый остаётся обычным слотом с четырьмя последовательными эффектами и четырьмя модуляторами:

```text
Актёр/движок → FX 1 → FX 2 → FX 3 → FX 4 → уровень/панорама
    ↑       ↑      ↑      ↑      ↑
        четыре свободных модулятора

4 актёра → микшер → DC blocker → мягкий лимитер → SDL audio
```

Это «около-трекерный» инструмент без дорожек, нотной таблицы и транспорта. Время существует как общий пульс, LFO, случайные процессы и вероятностные события.

## Уже работает

- четыре одновременно работающих аудиослота;
- три радикально разные scene recipes, переключаемые без перезапуска;
- двенадцать процедурных актёров: room bed, footsteps, door, pipe, motor, machinery, crowd, metal, wind, birds, insects и rising signal;
- четыре FX-ячейки на слот: drive, low-pass, tremolo, delay, crusher и bypass;
- четыре модулятора на слот: sine, triangle, sample & hold и random walk;
- назначения на pitch, timbre, color, motion, texture, level, pan и amount любого FX;
- bounded SPSC mailbox между UI и аудиопотоком;
- сглаживание параметров, DC blocker и master soft limiter;
- сохранение/загрузка человекочитаемых `.cdrone`-сессий;
- русский и английский каталоги строк, переключаемые без перезапуска;
- CLI offline-render в stereo 48 kHz float WAV;
- SDL2-прототип с логическим разрешением 512×384, масштабируемым до 1024×768;
- встроенный public-domain bitmap-шрифт 8×8 с латиницей и кириллицей, без SDL_ttf;
- пять интерфейсных экранов: `Сцена`, `Слоты`, `FX`, `Мастер`, `Настр.`;
- глобальные макросы `Material`, `Activity`, `Tension`, `Distance`, `Evolution`, управляющие сразу временем, материалом, высотой, плотностью, пространством и составом жестов;
- отдельные времена fade-in/fade-out, сквозная индикация автофейда и `KILL`, очищающий голоса и хвосты;
- RMS/peak и реальные waveform snapshots из аудиопотока для понятных реактивных индикаторов;
- оценка загрузки DSP callback на экранах `Мастер` и `Настр.`;
- debounce-autosave текущей сессии, включая язык и скорости фейда;
- ускорение изменения значения после 1,05 и 2,2 секунды удержания;
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

# сразу отрендерить один из ландшафтов
./build/cursed-drone --scene factory --render factory.wav --seconds 45
./build/cursed-drone-render wasteland.wav 45 wasteland
```

## Управление SDL-прототипом

| Действие | Клавиатура | Планируемая кнопка консоли |
| --- | --- | --- |
| выбрать слот (кроме FX) | ← / → | D-pad ← / → |
| выбрать параметр / FX | ↑ / ↓ | D-pad ↑ / ↓ |
| изменить значение | A / D | L / R |
| сменить ландшафт на экране `Сцена` | E | Start |
| следующий экран | Tab или 1…5 | X |
| mute слота | Space | A |
| плавный fade in/out | F | Select / Back |
| очистить голоса и FX-хвосты | K | B |
| сменить тип выбранного FX на экране `FX` | E | Start |
| на экране FX: поле amount/tone/feedback | ← / → | D-pad ← / → |
| на экране FX: следующий слот | S | Y |
| RU / EN | экран `Настр.`, A / D | экран `Настр.`, L / R |
| выход | Esc | Menu |

Одиночное нажатие меняет обычный параметр на 1%. После 1,05 секунды удержания шаги идут вдвое быстрее, после 2,2 секунды — резко ускоряются. Mute прекращает новый сигнал, но намеренно оставляет музыкальный delay-tail; `KILL` очищает и источники, и память эффектов.

Макросы экрана `Сцена` не являются переименованными FX. Они подаются в планировщики событий и внутрь моделей материала:

| Макрос | Что меняет в аудиографе |
| --- | --- |
| `Material` | поверхность шагов, долю воздуха/пыли, щётки мотора, трение, шумовую часть жестов и мягкое насыщение |
| `Activity` | частоту шагов, рабочих циклов, фраз, скрипов и других событий; на непрерывных слоях — ощущение нагрузки |
| `Tension` | направленные подъёмы высоты, нестабильность скорости, резкость скрипа, диапазон свипов и давление верхнего спектра |
| `Distance` | dry/wet, feedback и длительность хвостов; воспринимаемую близость переднего и фонового плана |
| `Evolution` | длительность больших циклов, приближение/удаление, длину пачек, паузы и скорость изменения среды |

`Fade` теперь является состоянием мастер-выхода, а не творческим макросом. В `Настр.` независимо задаются времена открытия и закрытия.

## Ландшафты `0.4.0`

| Ландшафт | Четыре актёра | Характер |
| --- | --- | --- |
| `DERELICT` / `ЗАБРОШЕННОЕ` | тихий гул, приближающиеся шаги, stick-slip двери, ветер и стуки в трубе | низкий и разреженный, с редкими передними жестами |
| `FACTORY` / `ЦЕХ` | rotor/brush motor, цикл станка, формантный гул людей, движущийся металлический скрежет | плотный механический спектр и повторения с плавающей нагрузкой |
| `WASTELAND` / `ПУСТОШЬ` | многоскоростной ветер, птичьи фразы, рой насекомых, серия повышающихся импульсов со случайной паузой | воздушный верх, большие пустоты и отдельные сигналы |

Старые `TONE / RESONATOR / GRAINLET / PARTICLES` оставлены как ручные альтернативы в экспертном экране слота, но больше не образуют стартовый патч. Новый слой использует DaisySP-фильтры и осцилляторы, а процедурная оркестрация реализует отдельные модели возбуждения, материала и поведения. Записанных сэмплов нет.

## Документация

- [Архитектура](docs/architecture.ru.md) · [Architecture](docs/architecture.en.md)
- [Исследование проектов и DSP](docs/research.ru.md) · [Research](docs/research.en.md)
- [Дорожная карта](docs/roadmap.ru.md) · [Roadmap](docs/roadmap.en.md)
- [Разбор звукового референса](docs/reference-sound.ru.md) · [Sound reference analysis](docs/reference-sound.en.md)
- [Архитектура процедурных ландшафтов](docs/soundscapes.ru.md) · [Procedural soundscape architecture](docs/soundscapes.en.md)
- [TrimUI Brick и портирование](docs/trimui-brick.ru.md) · [TrimUI Brick and porting](docs/trimui-brick.en.md)
- [Тестирование на macOS](docs/testing-macos.ru.md) · [Testing on macOS](docs/testing-macos.en.md)
- [Лицензии и сторонний код](THIRD_PARTY_NOTICES.md)

## Лицензия

Код проекта распространяется по **GNU GPL v3.0 or later**. См. [LICENSE](LICENSE). Сторонние движки сохраняют собственные лицензии и уведомления; каждый импорт проходит отдельную проверку.

---

## English

`Cursed Drone` is a native procedural soundscape instrument for small Linux handhelds. It creates evolving environments, foreground gestures and controllable sonic disasters without a tracker grid or synth programming.

The primary target is the **TrimUI Brick**, while the SDL2 shell and compact C++ core are intended to remain portable to Anbernic devices, PortMaster systems and desktop Linux.

> Status: experimental prototype `0.4.0`. The default four-synth patch has been replaced by three procedural landscapes: Derelict, Factory and Wasteland. Each combines a continuous bed with actors that have independent event timing, excitation and gesture memory.

Each of four independent slots contains one engine, four serial effects and four free modulation lanes. The mixer ends in DC removal and a soft master limiter. There is a shared sense of time, but no notes table, transport or tracker sequencing.

Keyboard controls: Left/Right selects a slot, Up/Down selects a parameter, A/D changes it, Tab (or 1–5) changes page, Space mutes with effect tails, F runs the configured fade, K kills all voices/tails, and Escape exits. E changes the landscape on Scene and the algorithm on FX. The five performance macros are Material, Activity, Tension, Distance and Evolution.

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

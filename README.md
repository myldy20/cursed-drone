# Cursed Drone / Проклятый гудёж

[![build](https://github.com/myldy20/cursed-drone/actions/workflows/build.yml/badge.svg)](https://github.com/myldy20/cursed-drone/actions/workflows/build.yml)

**English** · [Русский ниже](#русский)

`Cursed Drone` is a native procedural soundscape instrument for small Linux handhelds. It creates environments, foreground gestures and controllable sonic disasters without a tracker grid or the need to program a conventional synthesizer.

The primary target is the **TrimUI Brick**. The SDL2 frontend and compact C++ core are also intended for Anbernic devices, PortMaster-compatible systems, macOS and desktop Linux.

> Status: experimental `0.5.0` development version. Three procedural landscapes and sixteen selectable engines work now. This iteration rebuilds the live interface around consistent D-pad navigation, explicit engine/effect pickers, independent slot editing and clear output-fade state.

## Concept

A landscape contains four actors with different jobs: a continuous bed, movement, a foreground gesture and a distant or textural layer. In detailed mode every actor remains a slot with four serial effects and four modulation lanes.

```text
actor / engine -> FX 1 -> FX 2 -> FX 3 -> FX 4 -> level / pan
       ^           ^       ^       ^       ^
                  four modulation lanes

four actors -> mixer -> DC blocker -> soft limiter -> SDL audio
```

Time exists as a shared process tempo, long cycles, envelopes, random walks and bounded probabilistic events. There is no note table, transport or tracker sequence.

## What works

- four simultaneous audio slots;
- `Derelict`, `Factory` and `Wasteland` landscape recipes;
- twelve procedural actors: room bed, footsteps, door, pipe, motor, machinery, crowd, metal, wind, birds, insects and rising signal;
- four general engines: tone, resonator, grainlet and particles;
- four serial FX slots per actor: drive, low-pass, tremolo, delay, crusher or empty;
- four modulators per actor: sine, triangle, sample-and-hold and random walk;
- performance macros `Material`, `Activity`, `Tension`, `Distance` and `Evolution` that change several parts of the scene at once;
- separate fade-in and fade-out times, tail-preserving mute and hard `Kill`;
- live per-slot and master waveform, RMS, peak and throttled DSP-load telemetry;
- readable `.cdrone` sessions with debounced autosave;
- English and Russian UI; English is the default for a new installation;
- SDL2 UI at logical `512×384`, scaling exactly to the Brick's `1024×768` display;
- built-in Latin/Cyrillic bitmap font without SDL_ttf;
- offline stereo 48 kHz float WAV rendering;
- tests for localization, the lock-free queue, audio output, every landscape actor and session round-trips.

Existing autosaves preserve their selected language. Change it on `Setup`, or remove the autosave if a completely fresh English session is desired.

## Controls

The direction of navigation matches the screen: left/right selects a horizontal track or FX column; up/down selects a vertical row.

| Action | Keyboard | Handheld mapping |
| --- | --- | --- |
| select track / FX column | Left / Right | D-pad Left / Right |
| select parameter row | Up / Down | D-pad Up / Down |
| change value | A / D | L / R |
| open engine or effect picker | E | Start |
| confirm / cancel picker | Enter / Escape | A / B |
| next page | Tab or 1–5 | X |
| mute selected track | Space | A outside a picker |
| select next source track on FX | S | Y |
| run output fade | F | Select / Back |
| clear voices and effect tails | K | B outside a picker |
| exit | Escape | Menu / OS action |

A short value press changes most parameters by one percent. Holding accelerates after 1.05 seconds and again after 2.2 seconds. Mute stops new source audio but keeps delay tails; `Kill` clears both source and effect memory.

The `Scene` macro endpoints describe the audible direction rather than implementation details: smooth/rough, sparse/busy, stable/unstable, near/far and static/changing. Detailed edits mark the landscape as `Modified`; loading a landscape recipe restores its four actors and clears that marker.

## Build and test

The core needs C++20 and Make or CMake 3.16+. SDL2 is only required for the interactive frontend.

```bash
make -j2
make test
make render

# with sdl2-config installed
make sdl
./build/cursed-drone-sdl
```

Equivalent CMake build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

## macOS

```bash
brew install cmake sdl2
git clone https://github.com/myldy20/cursed-drone.git
cd cursed-drone
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/cursed-drone-sdl
```

The macOS autosave is stored under the SDL preference directory. A previous Russian autosave remains Russian until changed on `Setup`.

## CLI and offline render

```bash
# help in English or Russian
./build/cursed-drone --lang en
./build/cursed-drone --lang ru

# save the default session
./build/cursed-drone --save-default default.cdrone

# render a saved session or a landscape
./build/cursed-drone --load default.cdrone --render drone.wav --seconds 30
./build/cursed-drone --scene factory --render factory.wav --seconds 45
./build/cursed-drone-render wasteland.wav 45 wasteland
```

## Landscapes

| Landscape | Four actors | Character |
| --- | --- | --- |
| `Derelict` | room bed, approaching footsteps, stick-slip door, pipe wind and knocks | low and sparse with rare close gestures |
| `Factory` | rotor/brush motor, machine cycle, crowd-formant bed, moving metal scrape | dense mechanical spectrum with drifting load |
| `Wasteland` | multi-rate wind, bird phrases, insect swarm, rising signal and bounded pause | airy high end, large gaps and isolated signals |

Recorded samples are not used. The current procedural layer combines a pinned DaisySP subset with project-specific excitation, material and event scheduling. The [synthesis catalogue](docs/synthesis-catalog.en.md) records the next audited engines and why preset assets require a separate licence check.

## Documentation

- [Architecture](docs/architecture.en.md) · [Архитектура](docs/architecture.ru.md)
- [Research](docs/research.en.md) · [Исследование](docs/research.ru.md)
- [Synthesis catalogue](docs/synthesis-catalog.en.md) · [Каталог синтеза](docs/synthesis-catalog.ru.md)
- [Procedural soundscapes](docs/soundscapes.en.md) · [Процедурные ландшафты](docs/soundscapes.ru.md)
- [Sound reference analysis](docs/reference-sound.en.md) · [Разбор референса](docs/reference-sound.ru.md)
- [Roadmap](docs/roadmap.en.md) · [Дорожная карта](docs/roadmap.ru.md)
- [TrimUI Brick and porting](docs/trimui-brick.en.md) · [TrimUI Brick и портирование](docs/trimui-brick.ru.md)
- [Testing on macOS](docs/testing-macos.en.md) · [Тестирование на macOS](docs/testing-macos.ru.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)

## Licence

Project code is licensed under **GNU GPL v3.0 or later**. See [LICENSE](LICENSE). Third-party engines retain their own licences and notices; every import is audited separately.

---

## Русский

`Проклятый гудёж` — нативный генератор процедурных звуковых ландшафтов для маленьких Linux-консолей. Он создаёт среды, жесты переднего плана и управляемые звуковые катастрофы без трекерной сетки и необходимости программировать обычный синтезатор.

Главная целевая консоль — **TrimUI Brick**. SDL2-интерфейс и компактное C++-ядро также рассчитаны на устройства Anbernic, PortMaster-совместимые системы, macOS и обычный Linux.

> Статус: экспериментальная версия `0.5.0` в разработке. Уже работают три процедурных ландшафта и шестнадцать выбираемых движков. Эта итерация пересобирает концертный интерфейс: единая логика D-pad, явный выбор движков и эффектов, независимое редактирование слотов и понятное состояние фейда выхода.

## Идея

Ландшафт содержит четырёх актёров с разными задачами: непрерывный фон, движение, жест переднего плана и дальний или текстурный слой. В детальном режиме каждый актёр остаётся слотом с четырьмя последовательными эффектами и четырьмя линиями модуляции.

```text
актёр / движок -> FX 1 -> FX 2 -> FX 3 -> FX 4 -> уровень / панорама
       ^           ^       ^       ^       ^
                     четыре модулятора

четыре актёра -> микшер -> DC blocker -> мягкий лимитер -> SDL audio
```

Время существует как общий темп процессов, длинные циклы, огибающие, случайные блуждания и ограниченные вероятностные события. Таблицы нот, транспорта и трекерной последовательности нет.

## Что работает

- четыре одновременно звучащих аудиослота;
- рецепты ландшафтов `Заброшенное`, `Цех` и `Пустошь`;
- двенадцать процедурных актёров: комнатный фон, шаги, дверь, труба, мотор, станок, толпа, металл, ветер, птицы, насекомые и повышающийся сигнал;
- четыре общих движка: тон, резонатор, зерно и частицы;
- четыре последовательных FX-слота на актёра: drive, low-pass, tremolo, delay, crusher или пусто;
- четыре модулятора на актёра: sine, triangle, sample-and-hold и random walk;
- исполнительские макросы `Материал`, `Активность`, `Напряжение`, `Дистанция` и `Развитие`, одновременно меняющие несколько частей сцены;
- раздельные времена открытия и закрытия, mute с сохранением хвостов и жёсткий `Kill`;
- живые waveform, RMS и peak по слотам и мастеру, а также замедленная индикация загрузки DSP;
- читаемые `.cdrone`-сессии с debounce-autosave;
- английский и русский интерфейс; для новой установки по умолчанию выбран английский;
- SDL2-интерфейс в логическом разрешении `512×384`, ровно масштабируемый до экрана Brick `1024×768`;
- встроенный bitmap-шрифт с латиницей и кириллицей без SDL_ttf;
- offline-render в stereo 48 kHz float WAV;
- тесты локализации, lock-free-очереди, аудиовыхода, каждого актёра и round-trip сессии.

Существующий автосейв сохраняет выбранный язык. Его можно поменять на экране `Настр.` или удалить автосейв, если нужна полностью новая английская сессия.

## Управление

Направление навигации совпадает с экраном: влево/вправо выбирает горизонтальную дорожку или колонку FX, вверх/вниз — вертикальную строку.

| Действие | Клавиатура | Кнопка консоли |
| --- | --- | --- |
| выбрать дорожку / колонку FX | Left / Right | D-pad Left / Right |
| выбрать строку параметра | Up / Down | D-pad Up / Down |
| изменить значение | A / D | L / R |
| открыть выбор движка или эффекта | E | Start |
| подтвердить / отменить выбор | Enter / Escape | A / B |
| следующий экран | Tab или 1–5 | X |
| mute выбранной дорожки | Space | A вне окна выбора |
| следующая исходная дорожка на FX | S | Y |
| запустить фейд выхода | F | Select / Back |
| очистить голоса и хвосты эффектов | K | B вне окна выбора |
| выход | Escape | Menu / действие ОС |

Короткое нажатие меняет большинство параметров на один процент. Удержание ускоряется через 1,05 секунды и ещё раз через 2,2 секунды. Mute прекращает новый сигнал, сохраняя хвосты дилея; `Kill` очищает и источник, и память эффектов.

Крайние положения макросов `Сцены` описывают слышимое направление, а не внутреннюю реализацию: гладко/грубо, редко/плотно, спокойно/нестабильно, близко/далеко и статично/меняется. Детальное редактирование помечает ландшафт как `Изменён`; загрузка рецепта восстанавливает четырёх актёров и снимает эту отметку.

## Сборка и тесты

Ядру нужны C++20 и Make или CMake 3.16+. SDL2 требуется только интерактивному интерфейсу.

```bash
make -j2
make test
make render

# если установлен sdl2-config
make sdl
./build/cursed-drone-sdl
```

Эквивалентная сборка CMake:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

## macOS

```bash
brew install cmake sdl2
git clone https://github.com/myldy20/cursed-drone.git
cd cursed-drone
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/cursed-drone-sdl
```

В macOS автосейв лежит в каталоге настроек SDL. Старый русский автосейв останется русским, пока язык не будет изменён на экране `Настр.`.

## CLI и offline-render

```bash
# справка на английском или русском
./build/cursed-drone --lang en
./build/cursed-drone --lang ru

# сохранить стартовую сессию
./build/cursed-drone --save-default default.cdrone

# отрендерить сохранённую сессию или ландшафт
./build/cursed-drone --load default.cdrone --render drone.wav --seconds 30
./build/cursed-drone --scene factory --render factory.wav --seconds 45
./build/cursed-drone-render wasteland.wav 45 wasteland
```

## Ландшафты

| Ландшафт | Четыре актёра | Характер |
| --- | --- | --- |
| `Заброшенное` | комнатный фон, приближающиеся шаги, stick-slip двери, ветер и стуки в трубе | низкий и разреженный, с редкими близкими жестами |
| `Цех` | rotor/brush-мотор, цикл станка, формантный фон толпы, движущийся скрежет металла | плотный механический спектр с плавающей нагрузкой |
| `Пустошь` | многоскоростной ветер, фразы птиц, рой насекомых, повышающийся сигнал с ограниченной случайной паузой | воздушный верх, большие пустоты и отдельные сигналы |

Записанные сэмплы не используются. Текущий процедурный слой объединяет закреплённый поднабор DaisySP с собственными моделями возбуждения, материала и событий. В [каталоге синтеза](docs/synthesis-catalog.ru.md) записаны следующие проверенные движки и причины, по которым лицензия пресетов проверяется отдельно.

## Документация

- [Architecture](docs/architecture.en.md) · [Архитектура](docs/architecture.ru.md)
- [Research](docs/research.en.md) · [Исследование](docs/research.ru.md)
- [Synthesis catalogue](docs/synthesis-catalog.en.md) · [Каталог синтеза](docs/synthesis-catalog.ru.md)
- [Procedural soundscapes](docs/soundscapes.en.md) · [Процедурные ландшафты](docs/soundscapes.ru.md)
- [Sound reference analysis](docs/reference-sound.en.md) · [Разбор референса](docs/reference-sound.ru.md)
- [Roadmap](docs/roadmap.en.md) · [Дорожная карта](docs/roadmap.ru.md)
- [TrimUI Brick and porting](docs/trimui-brick.en.md) · [TrimUI Brick и портирование](docs/trimui-brick.ru.md)
- [Testing on macOS](docs/testing-macos.en.md) · [Тестирование на macOS](docs/testing-macos.ru.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)

## Лицензия

Код проекта распространяется по **GNU GPL v3.0 or later**. См. [LICENSE](LICENSE). Сторонние движки сохраняют собственные лицензии и уведомления; каждый импорт проверяется отдельно.

# Исследование GitHub и выбор основы

> Примечание к релизу: в 0.12 закреплённый upstream-поднабор музыкального движка уже интегрирован; старые планы ниже сохранены как история исследования.

Срез проведён перед началом разработки и зафиксирован 16 июля 2026 года. Цель — найти не один «идеальный форк», а проверенные решения для архитектуры, портирования и DSP. Звёзды и активность меняются; лицензия и конкретный импорт всегда проверяются повторно на pinned commit.

## Ближайшие по идее приложения

| Проект | Что в нём полезно | Почему не берём целиком |
| --- | --- | --- |
| [ZicBox](https://github.com/apiel/zicBox) | Linux/embedded groovebox, много движков, отдельные FX, framebuffer, Pi Zero 2 W | GPLv3 подходит, но UI/треки и связность кода тяжелее нашей задачи; используем как архитектурный справочник |
| [Picoloop](https://github.com/yoyz/picoloop) | SDL, Linux, PSP/Vita/OpenDingux, несколько synth engines | старый и неоднородный код; статус общей лицензии требует покомпонентной проверки |
| [picoTracker](https://github.com/xiphonics/picoTracker) | современный handheld UX и дисциплина ограниченного управления | tracker и RP2040 — другая модель исполнения |
| [LittleGPTracker](https://github.com/djdiskmachine/LittleGPTracker) | зрелый handheld tracker, PortMaster-порт, полезные key/file patterns | логика трекера нам не нужна |
| [m8c](https://github.com/laamaa/m8c) и [m8c-brick-knulli](https://github.com/f32-0/m8c-brick-knulli) | конкретные решения SDL/gamepad/пакетирования и живой Brick/Knulli precedent | m8c — клиент другого устройства, не synth host |
| [OTTO](https://github.com/bitfieldaudio/OTTO) | сильная идея музыкального embedded UI | CC BY-NC-SA ограничивает коммерческое распространение; проект давно не основной upstream |
| [Zynthian UI](https://github.com/zynthian/zynthian-ui) | зрелый Linux-аудио прибор и preset workflow | полноразмерный Linux stack, плагины и Python UI слишком тяжелы |
| [norns](https://github.com/monome/norns) | пространства звука, scriptable modulation, отличный UX precedent | мощная runtime/script экосистема не соответствует бюджету handheld |
| [Organelle OS](https://github.com/critterandguitari/Organelle_OS) | C++/Pure Data устройство и patch workflow | Pd-ориентированная модель сложнее нужного фиксированного инструмента |

Вывод: лучше написать компактный standalone C++/SDL2 shell и заимствовать проверенные паттерны, чем форкать groovebox и удалять из неё tracker/DAW-сущности.

## DSP-источники

### Основные

**[DaisySP](https://github.com/electro-smith/DaisySP), MIT.** Embedded-friendly C++, понятный API, фильтры, эффекты и интересные источники `Dust`, `Particle`, `ClockedNoise`, `FractalNoise`, `Grainlet`. Это первый внешний DSP dependency и основа `PARTICLE`.

**[Mutable Instruments eurorack](https://github.com/pichenettes/eurorack).** STM32-часть опубликована под MIT, отдельные AVR-области — GPLv3. Репозиторий содержит самые подходящие макро-, физические и гранулярные движки. Импортировать нужно только проверенный набор файлов. Автор отдельно просит не называть производные продукты и модули брендом Mutable Instruments и оригинальными продуктовыми именами.

### Дополнительные

- **[Signalsmith Stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch), MIT** — качественный, но тяжёлый pitch/time эффект; только optional heavy tier.
- **[Airwindows](https://github.com/airwindows/airwindows), MIT** — большая шахта эффектов; брать отдельные audited алгоритмы.
- **[Soundpipe](https://github.com/PaulBatchelor/Soundpipe), MIT** — широкий набор компактного DSP, но upstream архивирован; использовать выборочно или как справочник.
- **[Faust libraries](https://github.com/grame-cncm/faustlibraries)** — быстрый способ экспериментировать и генерировать C++, но лицензия проверяется на уровне конкретной функции.
- **[Google MSFA](https://github.com/google/music-synthesizer-for-android), Apache-2.0** — возможный FM/DX7 engine; upstream архивирован, поэтому не входит в первый набор.
- **[Audible Instruments](https://github.com/VCVRack/AudibleInstruments), GPLv3** — полезные host wrappers вокруг некоторых Mutable engines; GPL проекта позволяет заимствовать адаптационные идеи с сохранением происхождения.
- **[Dexed](https://github.com/asb2m10/dexed), GPLv3** — источник UI/preset knowledge для будущего FM, но не первая зависимость.

## Рассмотренные runtime-подходы

| Подход | Решение | Причина |
| --- | --- | --- |
| собственный C++ graph + SDL2 | основной | наименьший runtime, полный контроль callback и UX |
| [libpd](https://github.com/libpd/libpd) | резервный optional engine | полезен для пользовательских патчей, но не нужен ядру |
| [libretro](https://github.com/libretro/libretro-samples) | адаптер после standalone | распространение удобно, но audio/frame timing и файловая модель ограничивают приборный UX |
| Csound runtime | не использовать | мощно, но чрезмерно по размеру, запуску и слою абстракции |
| LV2/CLAP host | не использовать | discovery, ABI, UI и dependency management несоразмерны устройству |
| форк ZicBox/Zynthian | не использовать | удаление чужой продуктовой модели сложнее чистого узкого shell |

## Алгоритмическое наполнение

### Движки первой версии

1. `MACRO` — 24-ish моделей макро-осциллятора через Plaits DSP.
2. `BODY` — модальный/струнный/резонаторный физический синтез через Elements/Rings DSP.
3. `GRAIN` — circular capture, freeze, density, size, position и feedback через Clouds DSP.
4. `PARTICLE` — пыль, clocked noise, fractal noise, particle bursts и grainlet из DaisySP.

Все четыре пользовательских слота могут выбрать любой тип; это выразительнее фиксированного «по одному экземпляру».

`GRAIN` должен уметь брать сигнал выбранного слота или master в заранее выделенный circular buffer. Обратные связи вводятся только через задержанный путь с ограничением gain, DC removal и Panic reset, чтобы граф не получил мгновенный algebraic loop.

### FX

Базовые: morph/SVF filter, ladder, drive, wavefolder, bit/sample crusher, ring mod, tremolo, chorus, flanger, phaser, delay, diffusion/reverb, pitch.

Экспериментальные после benchmark: granular freeze, spectral smear/stretch, waveset, vocoder, resonator и frequency shifter.

### Автоматизации без трекера

- LFO: sine, triangle, ramp, square;
- sample & hold и probability steps;
- Brownian/random walk;
- logistic/chaotic source с ограничением диапазона;
- envelope и envelope follower;
- clocked/Euclidean pulses;
- probability burst;
- короткая 4–8-точечная loop curve.

Общий clock задаёт tempo и отношения, но не требует playhead. Каждый источник может быть free-running или clock-ratio.

## Продуктовый вывод

Ключевая особенность — не число алгоритмов, а два уровня контролируемой случайности:

- `Mutate` слегка сдвигает только незаблокированные параметры в музыкально допустимых диапазонах;
- `Corrupt` может менять маршруты/движки/FX, но соблюдает CPU budget, feedback bounds и deterministic seed;
- `Panic` всегда доступен аппаратной комбинацией и восстанавливает безопасный звук.

Так «проклятый звук» становится воспроизводимым инструментом, а не случайным генератором клиппинга.

## Лицензионное решение

Проект выбран под GPL-3.0-or-later. Это разрешает совместить собственный код с GPLv3 adapters и при этом использовать MIT/Apache-2.0 зависимости с сохранением уведомлений. Цена решения: производные бинарники и изменённый код должны распространяться на условиях GPL, а будущая закрытая proprietary-ветка из этой кодовой базы невозможна без переоформления прав всех правообладателей.

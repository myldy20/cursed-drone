# Дорожная карта

## Срез 0.2 — интерфейс и автономность (готово)

- встроенный RU/EN bitmap renderer без SDL_ttf;
- экраны Performance, Slot, FX и Master;
- нелинейное ускорение при удержании;
- недеструктивные Texture/Pulse/Chaos/Space/Fade;
- BPM-пульсации и сглаженные случайные события;
- реактивные RMS/peak/осциллографические данные;
- музыкальный mute с хвостом и отдельный аппаратный Kill/Panic;
- schema 2 с обратной загрузкой schema 1.

Это ранний срез M4/M5, сделанный до продуктовых движков ради проверки управления на реальном устройстве.

## M0 — каркас и проверяемый тракт (готово)

- GPL-3.0-or-later репозиторий;
- C++20 core без обязательного framework;
- 4 слота × 4 FX × 4 modulation lanes;
- lock-free SPSC-передача сессий;
- RU/EN catalog и сохранение языка;
- `.cdrone` serialization;
- offline WAV и SDL2 shell;
- автоматические smoke-тесты.

## M1 — probe на реальном TrimUI Brick

- собрать aarch64 binary на совместимом sysroot;
- проверить SDL video, ALSA/SDL audio, все аппаратные кнопки и путь writable data;
- измерить реальные callback size, underrun/xrun и стабильность 48 kHz;
- составить mapping Knulli и CrossMix/stock, не предполагая одинаковые keycodes;
- проверить встроенный bitmap/font renderer с кириллицей и английским на реальном дисплее;
- сохранить language/session после перезапуска.

**Критерий:** 10 минут непрерывного diagnostic patch без underrun и утечки памяти.

## M2 — DSP benchmark harness

- per-engine и per-FX CPU timers вне callback hot path;
- сценарии 4 engines + 16 light/medium/heavy FX;
- прогоны на 816, 1008 и 1416 MHz, если firmware разрешает;
- таблица budget и автоматический overload indicator;
- Eco fallback для тяжёлых алгоритмов.

**Критерий:** рабочая конфигурация четырёх слотов при 1008 MHz с запасом не менее 25% блока.

## M3 — первые продуктовые движки

1. `PARTICLE`: DaisySP Dust, Particle, ClockedNoise, FractalNoise, Grainlet.
2. `MACRO`: audited MIT STM32 DSP из Plaits под нейтральным продуктовым именем.
3. `BODY`: resonator/physical modelling из Rings/Elements.
4. `GRAIN`: circular stereo memory и granular DSP из Clouds.

Каждый импорт получает pinned commit, license notice, файл происхождения и adapter tests. Пользователь может выбрать любой из четырёх типов в любом слоте.

## M4 — проклятость как управляемая функция

- `Mutate`: маленькое изменение незаблокированных параметров;
- `Corrupt`: структурное, но bounded изменение engine/FX/mod routes;
- parameter locks и deterministic seed;
- Brownian, logistic chaos, follower, Euclidean и probability sources;
- общий clock и ratio без tracker transport;
- развить текущий `KILL` до короткого click-free fade и аппаратной комбинации.

## M5 — UX и пресеты

- четыре slot pages, global mixer и performance page;
- focus/help overlay на обоих языках;
- undo для последней mutation/corruption;
- autosave с debounce и crash-safe temp/rename;
- factory sound-spaces вместо «патчей-пресетов»;
- экран CPU/memory/audio health.

## M6 — распространение

- PortMaster package, сначала aarch64;
- TrimUI Brick/Knulli installation guide;
- armhf build после benchmark;
- release archive, checksums и license bundle;
- libretro adapter только после стабильного standalone-релиза.

## Не входит в ближайшую версию

- трекерная нотная таблица;
- DAW-подобный timeline;
- LV2/CLAP host;
- Csound runtime;
- неограниченная patching-среда;
- собственная большая коллекция DSP-алгоритмов.

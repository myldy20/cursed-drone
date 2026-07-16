# TrimUI Brick и портирование

## Базовая цель

Референсное устройство проекта — пользовательский TrimUI Brick под Knulli Scarab. Наблюдавшаяся конфигурация: `aarch64`, 4 CPU cores, частоты до примерно 1416 MHz, около 1 ГБ памяти и дисплей `1024×768`. Это измерение конкретного устройства/прошивки, а не гарантия для всех ревизий.

Приложение рисует в логических `512×384`, поэтому Brick получает ровное масштабирование 2× без дробного blur. Целевой аудиоформат — stereo float 48 kHz с буфером 256 или 512 frames после измерения xruns.

## Почему SDL2

SDL2 даёт общий слой для окна/framebuffer, game controller и аудиоустройства, уже используемый множеством PortMaster-приложений. При этом аудиоядро SDL не знает, поэтому platform layer можно заменить прямым ALSA/KMS backend, если конкретная firmware этого потребует.

## Device probe перед оптимизацией

На каждом firmware необходимо записать:

```sh
uname -a
cat /proc/cpuinfo
cat /proc/meminfo | head
find /sys/devices/system/cpu -name scaling_available_frequencies -print -exec cat {} \;
ls -l /dev/input/by-path /dev/input/by-id 2>/dev/null
aplay -l 2>/dev/null
```

Затем отдельный probe-бинарник проверяет:

- реальный SDL renderer/driver и fullscreen mode;
- фактический audio format, sample rate и callback frames;
- все кнопки, стики, Menu/Select и удерживаемые комбинации;
- writable directory и атомарный `rename()`;
- поведение suspend/resume и возврат из launcher;
- количество callback overruns за 10 минут.

Нельзя предполагать одинаковые SDL keycodes у stock, CrossMix, Knulli и разных Anbernic firmware. Mapping хранится в platform profile, а не в общей UI-логике.

## Запуск

Черновой launcher находится в `platform/trimui-brick/launch.sh`. Он не меняет governor и системные настройки: сначала нужно безопасно определить доступную firmware API и гарантировать восстановление исходного состояния.

Ожидаемая структура пакета:

```text
CursedDrone/
├── bin/aarch64/cursed-drone-sdl
├── data/
├── licenses/
├── platform/trimui-brick/launch.sh
└── README.txt
```

User data должен находиться вне read-only program area. Путь передаётся через `CURSED_DRONE_DATA_DIR`. Autosave будет писать временный файл, вызывать flush при поддержке и заменять рабочую сессию атомарным rename.

## CPU budget

Оптимизация проводится по measured worst case, а не по средней загрузке desktop:

| Конфигурация | Назначение |
| --- | --- |
| 4 engines, без FX | baseline engine budget |
| 4 engines + 16 light FX | обязательный сценарий |
| 4 engines + 8 medium + 8 light | типичный сложный patch |
| 4 engines + heavy algorithms | определить допустимый лимит/Eco mode |

Внутри callback не должно быть malloc, файлов, mutex, форматирования строк и очистки больших буферов. CPU meter вычисляется на platform layer по времени блока, а UI получает сглаженную статистику через lock-free telemetry.

Критерий раннего успеха: на Brick при 1008 MHz десять минут воспроизведения четырёх движков без underrun. Только после этого имеет смысл полировать графику.

## PortMaster

Первая публичная упаковка — PortMaster aarch64. Она должна содержать:

- self-contained binary или явно перечисленные системные зависимости;
- launcher, который возвращает устройство в исходное состояние;
- mapping profiles;
- license bundle для проекта и всех DSP imports;
- writable save path;
- release checksum и краткий двуязычный README.

armhf идёт после aarch64 benchmark. libretro не является первым способом доставки: standalone лучше контролирует latency, autosave, системные кнопки и продолжительную работу без frame-driven audio.

## Тестовый пакет из GitHub Actions

Каждый коммит собирает артефакт `cursed-drone-portmaster-aarch64` в **Actions → build**. Распакуйте скачанный artifact: внутри находится `curseddrone-aarch64-test.zip`. Его содержимое нужно положить в `/userdata/roms/ports/` с сохранением структуры.

Первый запуск автоматически включает 90-секундный device probe. Нажмите все кнопки по очереди; после двух секунд `Start` завершает probe и открывает основной прототип. Отчёт сохраняется здесь:

```text
/userdata/roms/ports/curseddrone/conf/device-probe.log
```

Чтобы повторить диагностику:

```sh
rm /userdata/roms/ports/curseddrone/conf/probe-v1.complete
```

Для ручного запуска по SSH можно остановить EmulationStation, как рекомендует PortMaster:

```sh
/etc/init.d/S31emulationstation stop
cd /userdata/roms/ports/curseddrone
./cursed-drone-probe.aarch64 2>&1 | tee conf/device-probe-manual.log
/etc/init.d/S31emulationstation start
```

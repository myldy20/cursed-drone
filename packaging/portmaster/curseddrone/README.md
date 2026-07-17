# Cursed Drone v0.7 — TrimUI Brick public test

## RU

Это публичная тестовая сборка для TrimUI Brick и других AArch64-устройств с PortMaster.

При первом запуске диагностический экран проверяет SDL2, звук и кнопки. Нажмите Start, чтобы перейти в основную программу. Отчёт сохраняется в `curseddrone/conf/device-probe.log`. Чтобы повторить диагностику, удалите `curseddrone/conf/probe-v1.complete`.

## Управление на TrimUI Brick

| Кнопка | Действие |
| --- | --- |
| D-pad Left/Right | дорожка, слот или колонка FX |
| D-pad Up/Down | параметр или строка FX |
| L/R | уменьшить / увеличить значение |
| B | mute выбранной дорожки; подтвердить выбор |
| A | Kill: очистить голоса и хвосты FX; отменить выбор |
| X | следующий экран |
| Y | следующая исходная дорожка на экране FX |
| Select | автофейд выхода |
| Start | выбрать ландшафт, движок или эффект |
| удерживать Start, затем нажать Select | сохранить и выйти |

Язык и отдельные времена fade-in/fade-out меняются на экране `SETUP / НАСТР.` кнопками L/R.

Логи и автосохранение находятся внутри `curseddrone/conf/`.

## EN

This is a public test build for TrimUI Brick and other AArch64 PortMaster devices.

On first launch, the device probe checks SDL2 video, audio and controls. Press Start to enter the main application. The report is stored at `curseddrone/conf/device-probe.log`. Delete `curseddrone/conf/probe-v1.complete` to run the probe again.

## TrimUI Brick controls

| Button | Action |
| --- | --- |
| D-pad Left/Right | track, slot or FX column |
| D-pad Up/Down | parameter or FX row |
| L/R | decrease / increase value |
| B | mute selected track; confirm picker |
| A | Kill voices and FX tails; cancel picker |
| X | next page |
| Y | next source track on the FX page |
| Select | output auto-fade |
| Start | choose landscape, engine or effect |
| hold Start, then press Select | save and exit |

Language and separate fade-in/fade-out times are changed on `SETUP` with L/R.

Logs and autosave data are stored under `curseddrone/conf/`.

Thanks to the PortMaster project for its packaging conventions and platform helpers.

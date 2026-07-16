# Cursed Drone — pre-release probe

## RU

Это тестовый пакет, а не готовый релиз PortMaster. При первом запуске он:

1. открывает SDL2 fullscreen;
2. проигрывает текущий четырёхслотовый гудёж;
3. показывает четыре пульсирующие панели;
4. записывает video/audio/input параметры и все нажатые кнопки;
5. после нажатия Start запускает основной прототип.

Лог probe сохраняется в `curseddrone/conf/device-probe.log`. Чтобы повторить probe, удалите `curseddrone/conf/probe-v1.complete`.

## Controls / Управление

| Button | Action |
| --- | --- |
| D-pad Left/Right | slot / слот |
| D-pad Up/Down | parameter / параметр |
| L1/R1 | decrease/increase / изменить значение |
| A | mute slot / заглушить слот |
| Y | RU/EN |
| Start | finish first-run probe |

## EN

This is a test package, not a finished PortMaster release. On first launch it opens SDL2 fullscreen, plays the current four-slot patch, draws four pulsing panels, logs video/audio/input details and every pressed button, then starts the main prototype after Start is pressed.

The report is stored at `curseddrone/conf/device-probe.log`. Delete `curseddrone/conf/probe-v1.complete` to run it again.

Thanks to the PortMaster project for its packaging conventions and platform helpers.

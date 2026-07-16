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
| D-pad Left/Right | slot, or FX field / слот или поле FX |
| D-pad Up/Down | parameter or FX cell / параметр или ячейка FX |
| L1/R1 | decrease/increase / изменить значение |
| A | mute slot / заглушить слот |
| B | kill voices and FX tails / сбросить голоса и хвосты FX |
| X | next screen / следующий экран |
| Y | next slot on FX screen / следующий слот на экране FX |
| Select/Back | auto-fade in/out / автофейд входа/выхода |
| Start | change selected FX type / сменить тип выбранного FX |

Language and separate fade-in/fade-out times are changed on the `SETUP / НАСТР.` screen with L1/R1.

Язык и отдельные скорости fade-in/fade-out меняются на экране `SETUP / НАСТР.` кнопками L1/R1.

During the first-run probe, Start still finishes input capture and opens the main program.

Во время первого диагностического запуска Start по-прежнему завершает сбор кнопок и открывает основную программу.

## EN

This is a test package, not a finished PortMaster release. On first launch it opens SDL2 fullscreen, plays the current four-slot patch, draws four pulsing panels, logs video/audio/input details and every pressed button, then starts the main prototype after Start is pressed.

The report is stored at `curseddrone/conf/device-probe.log`. Delete `curseddrone/conf/probe-v1.complete` to run it again.

Thanks to the PortMaster project for its packaging conventions and platform helpers.

# Cursed Drone v0.8 — AArch64 PortMaster public test

## Important / Важно

**Real hardware verification exists only for TrimUI Brick running Knulli.** Other AArch64 PortMaster handhelds are currently unverified.

**На реальном железе проверен только TrimUI Brick с Knulli.** Другие AArch64-консоли с PortMaster пока не проверены.

Detailed guides included in the package:

```text
curseddrone/docs/install.en.md
curseddrone/docs/install.ru.md
```

## Correct installation layout / Правильная структура установки

Extract the package into the folder containing other PortMaster launchers:

```text
<ports>/Cursed Drone.sh
<ports>/curseddrone/cursed-drone-sdl.aarch64
<ports>/curseddrone/cursed-drone-probe.aarch64
<ports>/curseddrone/assets/cursed-drone-splash.bmp
```

For TrimUI Brick with Knulli, `<ports>` is:

```text
/userdata/roms/ports
```

Do not leave the launcher inside an extra ZIP-named directory.

Не оставляйте `Cursed Drone.sh` внутри лишней папки с названием архива — он должен лежать непосредственно в папке Ports.

## First launch / Первый запуск

The first launch opens a hardware probe. Press the controls you want to test, then press **Start** to enter the synth.

При первом запуске откроется диагностика железа. Нажмите проверяемые кнопки, затем **Start**, чтобы открыть синтезатор.

Logs:

```text
curseddrone/conf/device-probe.log
curseddrone/conf/cursed-drone.log
```

## Controls / Управление

| Button | Action |
| --- | --- |
| D-pad Left/Right | track, slot or FX column / дорожка, слот или колонка FX |
| D-pad Up/Down | parameter or FX row / параметр или строка FX |
| L/R | decrease/increase / изменить значение |
| B | mute; confirm picker / mute; подтвердить выбор |
| A | hard Kill; cancel picker / Kill; отменить выбор |
| X | next page / следующий экран |
| Y | next source track on FX / следующая исходная дорожка FX |
| Select | output fade / автофейд выхода |
| Start | choose landscape, engine or effect / открыть выбор |
| hold Start, then press Select | save and exit / сохранить и выйти |

Language and separate fade times are changed on `SETUP / НАСТР.` with L/R.

Язык и времена fade-in/fade-out меняются на экране `SETUP / НАСТР.` кнопками L/R.

## Saves / Сохранения

Version 0.8 stores the current session in:

```text
curseddrone/conf/autosave.cdrone
```

It also reads the legacy v0.7 path once if required:

```text
curseddrone/conf/myldy20/cursed-drone/autosave.cdrone
```

Developed by **Myldy design — @myldy20**.

Thanks to the PortMaster project for its packaging conventions and platform helpers.

# Cursed Drone Experimental 0.10 — TrimUI Brick test

## Important / Важно

This package belongs to the isolated `experiment/plaits-performance-lab` branch. It installs beside public v0.9 as **Cursed Drone Lab** and keeps a separate autosave.

Этот пакет собран из отдельной ветки `experiment/plaits-performance-lab`. Он ставится рядом с публичной v0.9 как **Cursed Drone Lab** и использует отдельное сохранение.

**Real hardware verification exists only for TrimUI Brick running Knulli.** Other AArch64 PortMaster handhelds remain unverified.

**На реальном железе проверен только TrimUI Brick с Knulli.** Другие AArch64-консоли с PortMaster пока не проверены.

Detailed guides included in the package:

```text
curseddrone-lab/docs/install.en.md
curseddrone-lab/docs/install.ru.md
curseddrone-lab/docs/experiment-lab.en.md
curseddrone-lab/docs/experiment-lab.ru.md
```

## Installation / Установка

Extract into the folder containing other PortMaster launchers:

```text
<ports>/Cursed Drone Lab.sh
<ports>/curseddrone-lab/cursed-drone-sdl.aarch64
<ports>/curseddrone-lab/cursed-drone-probe.aarch64
<ports>/curseddrone-lab/assets/cursed-drone-splash.bmp
<ports>/curseddrone-lab/assets/scales/*.scl
```

For TrimUI Brick with Knulli, `<ports>` is:

```text
/userdata/roms/ports
```

Do not leave the launcher inside an extra ZIP-named directory.

Не оставляйте `Cursed Drone Lab.sh` внутри лишней папки с названием архива.

## First launch / Первый запуск

The first launch opens a hardware probe. Press controls to test them, then press **Start** to enter the synth.

При первом запуске откроется диагностика железа. Нажмите проверяемые кнопки, затем **Start**.

Logs:

```text
curseddrone-lab/conf/device-probe.log
curseddrone-lab/conf/cursed-drone.log
```

## Simplified controls / Упрощённое управление

| Button | Action |
| --- | --- |
| D-pad | navigation only / только навигация |
| L/R | change value / изменить значение |
| A | open, act, confirm / открыть, выполнить, подтвердить |
| B | back or cancel; hold for Kill / назад или отмена; удерживать для Kill |
| X | next main page / следующий основной экран |
| Y | next actor / следующий актёр |
| Select | output fade / fade выхода |
| Start | open Performance Lab / открыть Performance Lab |
| Start + Select | save and exit / сохранить и выйти |

## Performance Lab

Press **Start**. The advanced functions are kept in this overlay and do not add more pages to the normal live workflow.

Нажмите **Start**. Расширенные функции находятся в отдельном оверлее и не раздувают обычный цикл экранов.

Lab sections:

- `ACTOR`: original upstream macro actor, 16 models, MAIN/AUX/MIX/STEREO, Scala and root;
- `MOD`: four bipolar modulation rows with safe rate cross-modulation;
- `EVENT`: Euclidean steps, pulses, rotation, probability and Reverse Grains;
- `MORPH`: target landscape and continuous morph;
- `STAGE`: minimal performance screen.

User Scala files go to:

```text
curseddrone-lab/conf/scales/
```

## Saves / Сохранения

Experimental 0.10 stores its schema-9 session separately:

```text
curseddrone-lab/conf/autosave.cdrone
```

The public v0.9 save under `curseddrone/conf/` is not touched.

Публичное сохранение v0.9 в `curseddrone/conf/` не изменяется.

Developed by **Myldy design — @myldy20**.

The experimental macro actor uses pinned original Plaits/stmlib source under MIT terms. Exact components and notices are in `curseddrone-lab/licenses/THIRD_PARTY_NOTICES.md`.

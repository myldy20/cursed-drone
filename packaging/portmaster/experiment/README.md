# Cursed Drone Experimental 0.11 — TrimUI Brick test

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

## Guided controls / Единое управление

| Button | Stable action |
| --- | --- |
| D-pad | select or change / выбрать или изменить |
| A | open, apply, execute / открыть, применить, выполнить |
| B | back or cancel; hold for Kill / назад или отмена; удерживать для Kill |
| X | next section on page / следующий раздел экрана |
| Y | contextual help / контекстная помощь |
| L/R | previous / next page / предыдущая / следующая страница |
| Select | output fade / fade выхода |
| Start | quick menu / быстрое меню |
| Start + Select | save and exit / сохранить и выйти |

Workflow: **Place → Actor → FX → Master → Memory**. The advanced musical source, Scala, Euclidean events and modulation are now part of the Actor page instead of a separate Lab overlay. Four additional FX slots process the final master signal.

Путь работы: **Место → Актёр → FX → Мастер → Память**. Музыкальный источник, Scala, Euclidean-события и модуляция находятся внутри страницы актёра, а не в отдельном Lab. Ещё четыре FX-слота обрабатывают итоговый Master.

## Saves / Сохранения

Experimental 0.11 stores its schema-9 session separately:

```text
curseddrone-lab/conf/autosave.cdrone
```

The public v0.9 save under `curseddrone/conf/` is not touched.

Публичное сохранение v0.9 в `curseddrone/conf/` не изменяется.

Developed by **Myldy design — @myldy20**.

The experimental macro actor uses pinned original Plaits/stmlib source under MIT terms. Exact components and notices are in `curseddrone-lab/licenses/THIRD_PARTY_NOTICES.md`.

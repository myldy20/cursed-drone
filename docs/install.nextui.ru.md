# Установка Cursed Drone 0.12.3 на NextUI

> Проверено на реальной TrimUI Brick с NextUI: работают запуск, изображение, звук и управление.

1. Скачайте артефакт `cursed-drone-nextui-tg5040`.
2. Откройте внешний ZIP от GitHub и найдите `cursed-drone-nextui-tg5040-test.zip`.
3. Распакуйте внутренний ZIP **в корень SD-карты NextUI**.
4. Проверьте путь `Tools/tg5040/Cursed Drone.pak/launch.sh`.
5. Откройте **Tools → Cursed Drone**.

Не устанавливайте PortMaster-архив на NextUI и не кладите сторонний Pak в скрытую `.system`.

## Обновление без потери настроек

Замените папку `Tools/tg5040/Cursed Drone.pak/`. Рабочие данные остаются в `.userdata/tg5040/cursed-drone/`.

## Управление

| Кнопка | Значение |
| --- | --- |
| D-pad | навигация и изменение значения; удержание ускоряет шаг |
| A | открыть, подтвердить, выполнить действие или заглушить актёра |
| B | назад/отмена; удержание — аварийный Kill |
| X | следующий раздел текущей страницы |
| Y | контекстная помощь |
| L / R | предыдущая / следующая страница |
| Select | fade итогового выхода |
| Start | быстрое меню |
| Start + Select | сохранить состояние и выйти |

## Данные и лог

```text
.userdata/tg5040/cursed-drone/autosave.cdrone
.userdata/tg5040/cursed-drone/memory-1.cdrone ... memory-8.cdrone
.userdata/tg5040/cursed-drone/scales/*.scl
.userdata/tg5040/logs/Cursed Drone.txt
```

Save states и auto-resume NextUI не применяются: Cursed Drone запускается как отдельное SDL-приложение.

В Tool Pak входят `NOTICE.md` и `ADDITIONAL_TERMS.md` в папке `licenses/`.

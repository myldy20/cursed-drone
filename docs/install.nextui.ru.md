# Установка Cursed Drone 0.14.1 на NextUI

> Проверено на реальной TrimUI Brick с NextUI: работают запуск, изображение, звук и управление.

1. Скачайте `cursed-drone-v0.14.1-nextui-tg5040.zip` из GitHub Release.
2. Распакуйте ZIP **в корень SD-карты NextUI**.
3. Проверьте путь `Tools/tg5040/Cursed Drone.pak/launch.sh`.
4. Откройте **Tools → Cursed Drone**.

CI-артефакты имеют технические имена и могут лежать внутри дополнительного внешнего архива. Для обычной установки используйте версионный ZIP из GitHub Releases.

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

В Tool Pak входят лицензия проекта, уведомления о стороннем коде, MIT-лицензия Musical-движка, `NOTICE.md` и `ADDITIONAL_TERMS.md` в папке `licenses/`.

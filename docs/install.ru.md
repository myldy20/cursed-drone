# Установка Cursed Drone 0.12.1 на Knulli / PortMaster

> Проверено на реальной TrimUI Brick с Knulli. Другие AArch64-консоли с PortMaster пока не подтверждены.

1. Скачайте артефакт `cursed-drone-portmaster-aarch64` из успешной сборки.
2. Откройте внешний ZIP от GitHub и найдите `curseddrone-aarch64-test.zip`.
3. Распакуйте внутренний ZIP в `/userdata/roms/ports/`.
4. Проверьте наличие `/userdata/roms/ports/Cursed Drone.sh` и `/userdata/roms/ports/curseddrone/cursed-drone-sdl.aarch64`.
5. Перезагрузите консоль или обновите Ports, откройте **Ports → Cursed Drone**. После первой диагностики нажмите Start.

## Обновление без потери настроек

Распакуйте новый пакет поверх старого. Не удаляйте `curseddrone/conf/`: там лежат autosave, восемь слотов памяти и пользовательские строи.

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

## Данные и логи

```text
curseddrone/conf/autosave.cdrone
curseddrone/conf/memory-1.cdrone ... memory-8.cdrone
curseddrone/conf/scales/*.scl
curseddrone/conf/device-probe.log
curseddrone/conf/cursed-drone.log
```

Встроенные строи находятся в `curseddrone/assets/scales/`. Дополнительные Scala-файлы `.scl` копируйте в `conf/scales/`.

Архивы для Knulli и NextUI не взаимозаменяемы.

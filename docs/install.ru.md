# Установка Cursed Drone 0.14.0 на Knulli / PortMaster

> Проверено на реальной TrimUI Brick с Knulli. Другие AArch64-консоли с PortMaster пока не подтверждены.

1. Скачайте `cursed-drone-v0.14.0-portmaster-aarch64.zip` из GitHub Release.
2. Распакуйте ZIP в `/userdata/roms/ports/`.
3. Проверьте наличие `/userdata/roms/ports/Cursed Drone.sh` и `/userdata/roms/ports/curseddrone/cursed-drone-sdl.aarch64`.
4. Перезагрузите консоль или обновите Ports, откройте **Ports → Cursed Drone**. После первой диагностики нажмите Start.

CI-артефакты имеют технические имена и могут лежать внутри дополнительного внешнего архива. Для обычной установки используйте версионный ZIP из GitHub Releases.

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

В пакет входят лицензия проекта, уведомления о стороннем коде, MIT-лицензия Musical-движка, `NOTICE.md` и `ADDITIONAL_TERMS.md` в `curseddrone/licenses/`.

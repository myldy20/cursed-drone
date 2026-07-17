# Установка Cursed Drone на игровую консоль

[English version](install.en.md)

## Сначала важное

Cursed Drone пока является **публичной тестовой версией**, а не официальным релизом из каталога PortMaster.

На реальном железе проверена только следующая конфигурация:

- **TrimUI Brick**;
- **Knulli scarab 2026/05/11**;
- Linux AArch64;
- рендерер Mali/OpenGL ES 2;
- звук ALSA, stereo 48 кГц, буфер 512 сэмплов;
- встроенный контроллер `TRIMUI Brick Controller`.

Пакет собирается под **AArch64**. На других PortMaster-совместимых приставках он может заработать, но они пока не проверены. Не стоит объявлять устройство поддерживаемым, пока от него не получены нормальные логи и результат реального теста.

## 1. Скачайте пакет

Откройте последнюю успешную [сборку GitHub Actions](https://github.com/myldy20/cursed-drone/actions/workflows/build.yml) и скачайте артефакт:

```text
cursed-drone-portmaster-aarch64
```

GitHub упаковывает артефакт во внешний ZIP. Сначала распакуйте его. Внутри должен лежать:

```text
curseddrone-aarch64-test.zip
```

Именно внутренний ZIP является установочным пакетом PortMaster.

## 2. Найдите папку Ports

Архив нужно распаковать в ту же папку, где уже лежат лаунчеры других PortMaster-портов.

| Прошивка / конфигурация | Обычная папка Ports | Статус |
| --- | --- | --- |
| TrimUI Brick + Knulli | `/userdata/roms/ports/` | **Проверено** |
| Другие устройства с Knulli | `/userdata/roms/ports/` | Не проверено |
| ArkOS, ROMs на первой карте | `/roms/ports/` | Не проверено |
| ArkOS, ROMs на второй карте | `/roms2/ports/` | Не проверено |
| ROCKNIX / системы, происходящие от JELOS | папка `ports` внутри активной библиотеки ROMs | Не проверено |
| AmberELEC | папка `ports` внутри активной библиотеки ROMs | Не проверено |

Пути у прошивок различаются. Надёжное правило простое: найдите папку, в которой уже лежат `.sh`-лаунчеры работающих портов, и распакуйте Cursed Drone туда.

## 3A. Установка через SD-карту или файловый менеджер

Распакуйте `curseddrone-aarch64-test.zip` непосредственно в папку Ports.

Правильный результат:

```text
<ports>/Cursed Drone.sh
<ports>/curseddrone/cursed-drone-sdl.aarch64
<ports>/curseddrone/cursed-drone-probe.aarch64
<ports>/curseddrone/assets/cursed-drone-splash.bmp
<ports>/curseddrone/README.md
```

Частая ошибка — лишняя вложенная папка:

```text
<ports>/curseddrone-aarch64-test/Cursed Drone.sh   # неправильно
```

`Cursed Drone.sh` должен лежать непосредственно в папке Ports.

После копирования обновите список игр или перезагрузите консоль.

## 3B. Установка на TrimUI Brick через SCP

Команды ниже предполагают, что:

- IP консоли — `10.53.219.134`;
- архив лежит в папке `Downloads` на Mac;
- пользователь SSH — `root`.

Команда копирования выполняется в **локальном окне Terminal на Mac**, а не внутри уже открытой SSH-сессии:

```bash
scp ~/Downloads/curseddrone-aarch64-test.zip \
  root@10.53.219.134:/userdata/system/
```

Подключитесь к Brick:

```bash
ssh root@10.53.219.134
```

Установите или обновите программу:

```bash
unzip -o /userdata/system/curseddrone-aarch64-test.zip \
  -d /userdata/roms/ports/

chmod +x "/userdata/roms/ports/Cursed Drone.sh"
chmod +x /userdata/roms/ports/curseddrone/*.aarch64
sync
reboot
```

Такая установка поверх старой версии не удаляет текущие настройки.

## 4. Первый запуск

Откройте:

```text
Ports → Cursed Drone
```

При первом запуске перед основной программой появляется диагностический экран. Он проверяет:

- видеодрайвер и рендерер;
- аудиодрайвер, частоту, число каналов и размер буфера;
- доступные контроллеры;
- физический маппинг кнопок.

Нажмите кнопки, которые хотите проверить, затем нажмите **Start**. Диагностика запишет отчёт и откроет Cursed Drone.

После диагностики показывается фирменная заставка. Её можно пропустить нажатием клавиши. Для отладки заставку можно отключить переменной:

```bash
export CURSED_DRONE_SKIP_SPLASH=1
```

## 5. Главное управление

| Действие | Кнопка |
| --- | --- |
| навигация | D-pad |
| уменьшить / увеличить значение | L / R |
| открыть окно выбора | Start |
| подтвердить / отменить выбор | B / A |
| следующий экран | X |
| mute выбранной дорожки | B вне окна выбора |
| жёсткий Kill | A вне окна выбора |
| следующая исходная дорожка на экране FX | Y |
| автофейд выхода | Select |
| сохранить и выйти | удерживать Start, затем нажать Select |

Комбинация выхода намеренно зависит от порядка: сначала удерживайте **Start**, затем нажмите **Select**.

## 6. Сохранения и логи

Все данные PortMaster-версии находятся внутри:

```text
<ports>/curseddrone/conf/
```

Файлы:

```text
conf/autosave.cdrone       текущая сессия начиная с v0.8
conf/device-probe.log      отчёт диагностики железа
conf/cursed-drone.log      stdout/stderr основной программы
conf/probe-v1.complete     маркер, отключающий повторную диагностику
```

Старые сборки сохраняли сессию сюда:

```text
conf/myldy20/cursed-drone/autosave.cdrone
```

Версия 0.8 при необходимости один раз прочитает старый путь, после чего новые сохранения будут записываться в `conf/autosave.cdrone`.

### Повторно запустить диагностику

```bash
rm -f /userdata/roms/ports/curseddrone/conf/probe-v1.complete
```

### Сбросить только сохранённое состояние синтезатора

```bash
rm -f /userdata/roms/ports/curseddrone/conf/autosave.cdrone
rm -f /userdata/roms/ports/curseddrone/conf/myldy20/cursed-drone/autosave.cdrone
```

## 7. Как отправить полезные логи

На TrimUI Brick соберите один архив:

```bash
tar -czf /userdata/system/cursed-drone-logs.tar.gz \
  -C /userdata/roms/ports/curseddrone/conf .
```

Скопируйте его на компьютер:

```bash
scp root@10.53.219.134:/userdata/system/cursed-drone-logs.tar.gz \
  ~/Downloads/
```

Прикрепите архив к GitHub issue и укажите:

- модель консоли;
- название и версию прошивки;
- какие действия выполнялись перед проблемой;
- продолжал ли идти звук;
- примерную загрузку DSP, которую показывала программа.

## 8. Решение проблем

### Порт не появился в меню

Проверьте, что лаунчер лежит именно здесь:

```text
<ports>/Cursed Drone.sh
```

После этого обновите список игр или перезагрузите консоль.

### Чёрный экран или мгновенный возврат в меню

Посмотрите:

```text
<ports>/curseddrone/conf/cursed-drone.log
<ports>/curseddrone/conf/device-probe.log
```

Также проверьте наличие и executable-бит у обоих файлов `.aarch64`.

### Диагностика запускается каждый раз

Прошивка, вероятно, не может создать:

```text
conf/probe-v1.complete
```

Проверьте свободное место и возможность записи на раздел с ROMs.

### Кнопки не соответствуют подписям

Удалите `probe-v1.complete`, снова пройдите диагностику и приложите `device-probe.log` к issue. Разные версии прошивок могут отдавать SDL разные маппинги контроллера.

### Звук трещит

Запишите показанную загрузку DSP, название сцены и используемые движки. Не начинайте сразу с разгона консоли: сначала нужен воспроизводимый патч и лог, потому что проект должен оставаться пригодным для слабых AArch64-устройств.

## 9. Удаление

Удалите:

```text
<ports>/Cursed Drone.sh
<ports>/curseddrone/
```

Удаление папки `curseddrone/` также удалит автосейв и логи. Если текущий патч нужен, сначала сохраните копию `conf/autosave.cdrone`.

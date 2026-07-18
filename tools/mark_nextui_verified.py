#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text(encoding='utf-8')
    count = text.count(old)
    if count != 1:
        raise SystemExit(f'{path}: expected one match, found {count}: {old!r}')
    p.write_text(text.replace(old, new), encoding='utf-8')

replace_once(
    'README.md',
    '  <img src="https://img.shields.io/badge/verified-TrimUI_Brick_Knulli-50a99a" alt="verified on TrimUI Brick with Knulli">',
    '  <img src="https://img.shields.io/badge/verified-TrimUI_Brick_Knulli-50a99a" alt="verified on TrimUI Brick with Knulli">\n'
    '  <img src="https://img.shields.io/badge/verified-TrimUI_Brick_NextUI-50a99a" alt="verified on TrimUI Brick with NextUI">')
replace_once(
    'README.md',
    '> **Hardware status:** real-device testing has only been completed on a **TrimUI Brick running Knulli**. The Knulli/PortMaster package is verified there. The separate NextUI Tool Pak is built and inspected in AArch64 CI but remains **unverified on physical NextUI hardware**.',
    '> **Hardware status:** real-device testing has been completed on a **TrimUI Brick with Knulli/PortMaster** and on a **TrimUI Brick with NextUI**. Both installation methods are verified. Other handheld models remain unverified.')
replace_once(
    'README.md',
    'The verified Knulli probe reported Mali/OpenGL ES 2 rendering, ALSA audio at 48 kHz stereo with a 512-sample buffer, and a correctly detected `TRIMUI Brick Controller`.',
    'The verified Knulli probe reported Mali/OpenGL ES 2 rendering, ALSA audio at 48 kHz stereo with a 512-sample buffer, and a correctly detected `TRIMUI Brick Controller`. A separate community hardware test confirmed that the NextUI Tool Pak also launches and works correctly on TrimUI Brick.')
replace_once(
    'README.md',
    '> **Статус железа:** реальная проверка выполнена только на **TrimUI Brick с Knulli**. Пакет Knulli/PortMaster на ней проверен. Отдельный Tool Pak для NextUI собирается и проверяется в AArch64 CI, но пока **не проверен на реальной консоли с NextUI**.',
    '> **Статус железа:** реальная проверка выполнена на **TrimUI Brick с Knulli/PortMaster** и на **TrimUI Brick с NextUI**. Оба способа установки подтверждены. Другие модели консолей пока не проверены.')
replace_once(
    'README.md',
    'По логам проверенного TrimUI Brick с Knulli приложение корректно использует Mali/OpenGL ES 2, ALSA 48 кГц stereo с буфером 512 сэмплов и контроллер `TRIMUI Brick Controller`.',
    'По логам проверенного TrimUI Brick с Knulli приложение корректно использует Mali/OpenGL ES 2, ALSA 48 кГц stereo с буфером 512 сэмплов и контроллер `TRIMUI Brick Controller`. Отдельный внешний тест подтвердил, что NextUI Tool Pak также запускается и корректно работает на TrimUI Brick.')

replace_once(
    'docs/install.nextui.en.md',
    '> Status: the package is built and validated in AArch64 CI, but has not yet been tested on a physical TrimUI Brick running NextUI.',
    '> Status: **verified on a physical TrimUI Brick running NextUI**. Video, audio, controls and normal application startup were confirmed in a community hardware test.')
replace_once(
    'docs/install.nextui.en.md',
    '## Limitations',
    '## Verified configuration\n\n- Device: TrimUI Brick\n- Firmware: NextUI\n- Pak location: `Tools/tg5040/Cursed Drone.pak/`\n- Result: application launched and worked correctly\n\nOther NextUI devices and platform folders remain unverified.\n\n## Limitations')

replace_once(
    'docs/install.nextui.ru.md',
    '> Статус: пакет собран и проверен в CI под AArch64, но пока не проверен на реальной TrimUI Brick с NextUI.',
    '> Статус: **проверено на реальной TrimUI Brick с NextUI**. В ходе внешнего теста подтверждены запуск приложения, изображение, звук и управление.')
replace_once(
    'docs/install.nextui.ru.md',
    '## Ограничения',
    '## Проверенная конфигурация\n\n- Устройство: TrimUI Brick\n- Прошивка: NextUI\n- Путь Pak: `Tools/tg5040/Cursed Drone.pak/`\n- Результат: приложение запустилось и корректно работало\n\nДругие устройства NextUI и другие platform-папки пока не проверены.\n\n## Ограничения')

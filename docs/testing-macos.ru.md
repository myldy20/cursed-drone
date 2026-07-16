# Тестирование на macOS

## Вариант 1 — готовый артефакт GitHub Actions

После успешной сборки откройте вкладку **Actions → build → последний успешный запуск** и скачайте `cursed-drone-macos-arm64`. Это нативная сборка для Apple Silicon, включая MacBook Air M2.

После распаковки в Terminal:

```bash
cd ~/Downloads/cursed-drone-macos-arm64
chmod +x cursed-drone cursed-drone-sdl cursed-drone-probe cursed-drone-render
xattr -dr com.apple.quarantine .
./cursed-drone-probe --windowed --seconds 20
./cursed-drone-sdl
```

Probe должен открыть окно с четырьмя пульсирующими колонками и одновременно проигрывать текущий четырёхслотовый патч. Нажмите несколько клавиш; `Escape` завершает probe после первых двух секунд.

## Вариант 2 — сборка из исходников

Нужны Xcode Command Line Tools, Homebrew, CMake и SDL2:

```bash
xcode-select --install
brew install cmake sdl2
git clone https://github.com/myldy20/cursed-drone.git
cd cursed-drone
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/cursed-drone-probe --windowed --seconds 20
./build/cursed-drone-sdl
```

Для offline-render:

```bash
./build/cursed-drone --lang ru --save-default default.cdrone
./build/cursed-drone --load default.cdrone --render test.wav --seconds 30
open test.wav
```

## Управление текущим SDL-прототипом

| Действие | Клавиша |
| --- | --- |
| выбрать слот | ← / → |
| выбрать параметр текущего экрана | ↑ / ↓ |
| изменить значение | A / D |
| переключить Performance / Slot / FX / Master | Tab или 1 / 2 / 3 / 4 |
| mute выбранного слота | Space |
| плавный fade out / fade in за 4 секунды | F |
| немедленно очистить голоса и FX-хвосты | K |
| сменить алгоритм выбранной FX-ячейки | E |
| русский / английский | L |
| выход | Escape |

Все подписи теперь находятся внутри окна. A/D меняет обычное значение на 1% одним нажатием; при удержании скорость повышается после 1,05 и 2,2 секунды. Цвет, RMS-заполнение, peak-линия, осциллограмма и яркость FX-полос реагируют на фактический выход аудиографа.

На экране Performance доступны недеструктивные `Текстура / Пульс / Хаос / Пространство / Фейд`. Экран Slot оставляет точную ручную настройку семи параметров. FX открывает amount/tone/feedback каждого из четырёх эффектов; E циклически меняет алгоритм. Master показывает общую громкость, BPM, fade и итоговый метр.

## Что прислать после теста

- слышен ли непрерывный звук без щелчков;
- реагируют ли панели и управление;
- создаётся ли `test.wav`;
- вывод Terminal от `cursed-drone-probe`, если там есть `ERROR`;
- субъективно: достаточно ли драматично работают Texture/Pulse/Chaos/Space;
- читается ли кириллица и не съезжают ли подписи;
- комфортна ли трёхступенчатая скорость удержания A/D;
- продолжает ли mute оставлять хвост delay, а K полностью его обрывать.

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
| выбрать frequency/timbre/color/motion/texture/level | ↑ / ↓ |
| изменить значение | A / D |
| mute выбранного слота | Space |
| русский / английский | L |
| выход | Escape |

Пока текст параметра находится в заголовке окна, а экран использует цветовые панели. Полный шрифтовой UI будет добавлен после проверки видеодрайвера Brick.

## Что прислать после теста

- слышен ли непрерывный звук без щелчков;
- реагируют ли панели и управление;
- создаётся ли `test.wav`;
- вывод Terminal от `cursed-drone-probe`, если там есть `ERROR`;
- субъективно: меняется ли звук достаточно заметно при каждой из шести ручек.

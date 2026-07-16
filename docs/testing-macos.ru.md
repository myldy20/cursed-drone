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
| выбрать слот (кроме FX) | ← / → |
| выбрать параметр / FX | ↑ / ↓ |
| изменить значение | A / D |
| переключить Сцена / Слоты / FX / Мастер / Настр. | Tab или 1…5 |
| mute выбранного слота | Space |
| плавный fade out / fade in с заданной скоростью | F |
| немедленно очистить голоса и FX-хвосты | K |
| сменить алгоритм выбранной FX-ячейки | E |
| на FX выбрать amount/tone/feedback | ← / → |
| на FX перейти к следующему слоту | S |
| русский / английский | экран `Настр.`, A / D |
| выход | Escape |

Все подписи находятся внутри окна. A/D меняет обычное значение на 1% одним нажатием; при удержании скорость повышается после 1,05 и 2,2 секунды. Осциллограмма теперь строится из захваченных samples конкретного слота. Узкая бежевая полоса под ней — сглаженный RMS, короткая красная отметка — peak hold. На Master это же обозначено легендой.

На экране `Сцена` доступны недеструктивные `Текстура / Пульс / Хаос / Пространство / События`, каждый с краткой картой затрагиваемых процессов. `Слоты` позволяет выбрать `ТОН / РЕЗОНАТОР / ЗЕРНО / ЧАСТИЦЫ` и показывает осмысленные для движка подписи параметров. FX открывает четыре последовательных эффекта выбранного слота. Master показывает общую громкость, tempo пульса/событий, fade, итоговый scope и DSP load. В `Настр.` находятся язык и разные времена fade-in/fade-out. Изменения автоматически сохраняются примерно через 0,75 с бездействия.

## Что прислать после теста

- слышен ли непрерывный звук без щелчков;
- реагируют ли панели и управление;
- создаётся ли `test.wav`;
- вывод Terminal от `cursed-drone-probe`, если там есть `ERROR`;
- субъективно: различаются ли четыре источника и достаточно ли драматично работают Texture/Pulse/Chaos/Space/Events;
- читается ли кириллица и не съезжают ли подписи;
- комфортна ли трёхступенчатая скорость удержания A/D;
- продолжает ли mute оставлять хвост delay, а K полностью его обрывать;
- остаются ли язык и времена фейда после перезапуска;
- какой DSP load показывает Master на Mac и на Brick.

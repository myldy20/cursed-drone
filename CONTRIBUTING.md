# Участие в проекте / Contributing

## RU

Проект на ранней стадии; сначала обсуждайте большие DSP/UI-изменения в issue. Небольшие исправления, тесты и platform profiles можно сразу присылать pull request.

Перед коммитом:

```bash
make -j2
make test
```

Правила для DSP:

- никакого выделения памяти, файлов, mutex и логирования в audio callback;
- новый upstream-код требует exact commit, лицензии, provenance и записи в `THIRD_PARTY_NOTICES.md`;
- product macros должны быть ограничены, сглажены и доступны модуляции;
- feedback-пути обязаны иметь delay, ограничение gain и Panic reset;
- пользовательские строки добавляются одновременно на русском и английском;
- изменения формата Session требуют migration test и увеличения schema version.

Форматируйте C++ в существующем стиле: C++20, 4 пробела, `snake_case` для функций/переменных, `PascalCase` для типов. Предупреждения `-Wall -Wextra -Wpedantic -Wshadow -Wconversion` должны оставаться чистыми.

Отправляя код, вы соглашаетесь распространять свой вклад по GPL-3.0-or-later.

## EN

This is an early project; discuss large DSP or UI changes in an issue first. Small fixes, tests and platform profiles may go directly to a pull request.

Run `make -j2 && make test`. The audio callback may not allocate, touch files, lock or log. Every upstream import needs an exact commit, license/provenance records and a third-party notice. Product macros remain bounded, smoothed and modulatable. Feedback paths require delay, gain bounds and Panic reset. Add every user-facing string in Russian and English. Session format changes need migrations, tests and a schema bump.

Contributions are submitted under GPL-3.0-or-later.

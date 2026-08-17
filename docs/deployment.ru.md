# Self-hosted deployment Myldy

Cursed Drone следует единому стандарту развёртывания проектов `myldy.ru`.

## Роли инфраструктуры

Для Cursed Drone используются два класса self-hosted инфраструктуры с разными задачами.

### Домашняя build-машина

Тяжёлые сборки и тесты используют существующий selector домашнего runner:

```yaml
runs-on: [self-hosted, myldy-home]
```

Сюда относятся Android SDK/NDK/Gradle, C++ build trees, WebAssembly/Emscripten, npm dependencies, Playwright/Chromium и браузерные test artifacts.

### Production VPS Selectel

Production-facing операции используют только repo-specific VPS selector:

```yaml
runs-on: [self-hosted, myldy-vps, cursed-drone]
```

VPS — `135.106.183.22` (`myldysite`, Ubuntu 24.04) с маленьким root filesystem и рассматривается прежде всего как production host. Проектные пути:

- релизы: `/srv/www/cursed-drone/releases/`;
- активная версия: symlink `/srv/www/cursed-drone/current`;
- preview: `https://cursed-drone.myldy.ru/`.

Caddy, HTTPS, Basic Auth и доменные конфиги управляются централизованно вне репозитория. Workflow проекта не должен менять Caddyfile, получать TLS-сертификаты, менять firewall или systemd, перенастраивать другие runners, менять глобальные права сервера или трогать каталоги других проектов.

GitHub Pages остаётся включённым до отдельного подтверждения, что preview на `myldy.ru` работает корректно.

## Модель доверия

Каждая существенная job перед работой выводит `runner.name`, `runner.environment`, `runner.os`, `runner.arch`, `github.ref` и точный `github.sha`.

Репозиторий публичный, поэтому self-hosted runners не исполняют код из недоверенных fork PR. `pull_request` CI допускается только для PR, head-репозиторий которого — `myldy20/cursed-drone`. `pull_request_target` не используется для checkout или исполнения PR-кода.

GitHub-hosted fallback запрещён. Если домашний runner недоступен, тяжёлая job ждёт или падает, а не устанавливает многогигабайтные toolchains на Selectel VPS.

## Контроль диска и cleanup

Перед тяжёлой операцией домашний runner проверяет filesystem, на котором находится Actions workspace. При 80% usage выводится warning; при 85% и выше тяжёлая сборка/toolchain setup не стартует; при 90% и выше сначала требуется разбираться с диском.

Selectel VPS проверяет `/` перед deploy/rollback. При 80% usage выводится warning; при 90% deployment work не начинается. Тяжёлые сборки никогда не являются fallback-задачей VPS.

Cleanup является частью workflow contract, включая failure/cancellation paths. Через `trap` и/или `if: always()` удаляются project build directories, временные Android/Gradle/Emscripten/Playwright данные, browser profiles, deployment input, staging directories и временные symlinks. Browser/dev-server процессы завершаются по сохранённому PID, без широкого `pkill` по всему серверу.

Diagnostic Actions artifacts имеют короткий retention: browser/benchmark evidence — три дня, deployment bundle — один день.

## Preview deployment

`.github/workflows/deploy-mydly.yml` — основной workflow публикации preview.

Предпочтительный процесс:

```text
trusted commit
→ обновление ветки preview
→ disk guard домашнего runner
→ native core tests на домашнем runner
→ WebAssembly build на домашнем runner
→ Playwright/browser smoke на домашнем runner
→ компактный checksummed tar.gz artifact (retention 1 день)
→ disk guard VPS
→ только download/verify на VPS
→ staging validation и лёгкий HTTP smoke
→ immutable release directory
→ atomic current symlink switch
```

Также доступен ручной `workflow_dispatch`. Обычный push в `main` сам по себе не публикует новую Myldy preview-версию.

В текущей архитектуре короткоживущий GitHub Actions artifact используется только для передачи уже собранного компактного web bundle с домашней build-машины на VPS. Старое правило «не использовать artifact для передачи web-build» относилось к схеме, где сам build runner физически находился на VPS; теперь пересборка приложения на production host прямо запрещена. SSH deployment keys не используются.

Ошибка теста или сборки не запускает VPS activation job и поэтому не может изменить `current`.

## Immutable releases и build.json

Успешный deploy создаёт новый каталог в `/srv/www/cursed-drone/releases/`. Существующие release directories не изменяются.

Retention: **current + максимум два предыдущих rollback releases**. После успешной активации более старые project releases удаляются автоматически.

В каждом релизе есть `build.json` как минимум с полями:

- `project`;
- `version`;
- полный 40-символьный `commit_sha`;
- `built_at_utc`, записанный на домашней build-машине;
- `deployed_at_utc`, записанный во время VPS activation;
- `content_hash`.

`content_hash` — детерминированный SHA-256 deployed static payload без самого `build.json`. Сам deployment bundle также имеет SHA-256 checksum, который проверяется до распаковки.

## Rollback

Запусти Myldy deploy workflow вручную с `action=rollback` и точным именем существующего immutable release directory. Workflow проверит, что каталог находится внутри release root Cursed Drone, и атомарно переключит `current`.

Rollback не выполняет rebuild и не делает git revert.

## GitHub Pages

GitHub Pages пока остаётся включённым. Тяжёлая WebAssembly-сборка Pages выполняется на домашнем runner, а маленькая `deploy-pages` activation job — на VPS selector. Pages artifact хранится один день.

## Замороженные нативные пакеты 1.0

Проверенные и уже опубликованные пакеты Cursed Drone 1.0.0 для macOS, PortMaster/Knulli и NextUI остаются immutable в существующем GitHub Release. Обязательный maintenance CI покрывает portable Linux core, Android ARM64 и WebAssembly на self-hosted инфраструктуре.

Если когда-нибудь понадобится новый нативный maintenance-релиз для архитектуры, которой нет на существующей build-машине, сначала потребуется явно одобренный self-hosted runner/toolchain соответствующей архитектуры. GitHub-hosted runners и production VPS Selectel не являются fallback build-машинами.

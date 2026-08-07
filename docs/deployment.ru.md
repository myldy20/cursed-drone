# Self-hosted deployment Myldy

Cursed Drone следует единому стандарту развёртывания проектов `myldy.ru`.

## Граница ответственности проекта

Проект публикует только собственную статическую веб-сборку на VPS Myldy:

- сервер: `135.106.183.22` (`myldysite`, Ubuntu 24.04);
- веб-сервер: Caddy, управляется централизованно вне репозитория;
- релизы: `/srv/www/cursed-drone/releases/`;
- активная версия: symlink `/srv/www/cursed-drone/current`;
- preview: `https://cursed-drone.myldy.ru/`.

Workflow проекта не должен менять Caddyfile, получать TLS-сертификаты, менять firewall или systemd, перенастраивать другие runners, менять глобальные права сервера или трогать каталоги других проектов.

GitHub Pages остаётся включённым до отдельного подтверждения, что preview на `myldy.ru` работает корректно.

## Runner и модель доверия

Все CI/deploy jobs репозитория используют только:

```yaml
runs-on: [self-hosted, myldy-vps, cursed-drone]
```

Каждая существенная job перед работой выводит `runner.name`, `runner.environment`, `runner.os`, `runner.arch`, `github.ref` и точный `github.sha`.

Репозиторий публичный, поэтому self-hosted runner не исполняет код из недоверенных fork PR. Jobs на событии `pull_request` допускаются к VPS только если head-репозиторий самого PR — `myldy20/cursed-drone`. `pull_request_target` не используется.

Runner не должен донастраивать хост через `sudo`, Docker, systemd или глобальную установку пакетов. Portable SDK/toolchain допускаются только внутри рабочего каталога runner.

## Preview deployment

`.github/workflows/deploy-mydly.yml` — основной workflow публикации preview на Myldy VPS.

Предпочтительный процесс:

```text
trusted commit
→ обновление ветки preview
→ portable native core tests
→ WebAssembly build
→ browser interaction smoke tests
→ staging directory
→ validation
→ immutable release directory
→ atomic current symlink switch
```

Также доступен ручной `workflow_dispatch`. Обычный push в `main` сам по себе не публикует новую Myldy preview-версию.

Веб-сборка создаётся непосредственно на VPS-runner. SSH deployment keys и GitHub Artifacts для передачи веб-сборки на сервер не используются.

## Immutable releases и build.json

Успешный deploy создаёт новый каталог в `/srv/www/cursed-drone/releases/`. Существующие release directories не изменяются. По умолчанию сохраняются пять последних успешных релизов.

В каждом релизе есть `build.json` как минимум с полями:

- `project`;
- `version`;
- полный 40-символьный `commit_sha`;
- `built_at_utc`;
- `deployed_at_utc`;
- `content_hash`.

`content_hash` — детерминированный SHA-256 deployed static payload без самого `build.json`.

Тесты, сборка и проверка staging выполняются до изменения `current`. Ошибка теста или сборки не переключает активную версию.

## Rollback

Запусти Myldy deploy workflow вручную с `action=rollback` и точным именем существующего immutable release directory. Workflow проверит, что каталог находится внутри release root Cursed Drone, и атомарно переключит `current`.

Rollback не выполняет rebuild и не делает git revert.

## Замороженные нативные пакеты 1.0

Myldy runner — Linux/X64 VPS без Docker и без системной донастройки из project workflow. Единый стандарт также запрещает GitHub-hosted fallback. Поэтому обязательный maintenance CI после миграции покрывает portable Linux core, Android ARM64 и WebAssembly, но не пересобирает нативный macOS и AArch64 handheld packages.

Проверенные и уже опубликованные пакеты Cursed Drone 1.0.0 для macOS, PortMaster/Knulli и NextUI остаются неизменяемыми в GitHub Release. Это соответствует статусу feature-complete/maintenance-only. Если когда-нибудь понадобится новый нативный maintenance-релиз для macOS или handheld, сначала потребуется отдельно одобренный self-hosted runner/toolchain для соответствующей архитектуры; обход через GitHub-hosted runner не допускается.

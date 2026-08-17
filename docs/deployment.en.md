# Myldy self-hosted deployment

Cursed Drone follows the shared Myldy deployment standard for `myldy.ru` projects.

## Infrastructure roles

Cursed Drone uses two self-hosted infrastructure classes with different responsibilities.

### Home build machine

Heavy build/test work uses the existing home-runner selector:

```yaml
runs-on: [self-hosted, myldy-home]
```

This includes Android SDK/NDK/Gradle work, C++ build trees, WebAssembly/Emscripten builds, npm dependencies, Playwright/Chromium and browser evidence.

### Selectel production VPS

Production-facing work uses only the repo-specific VPS selector:

```yaml
runs-on: [self-hosted, myldy-vps, cursed-drone]
```

The VPS is `135.106.183.22` (`myldysite`, Ubuntu 24.04) with a small root filesystem and is treated primarily as a production host. Its project paths are:

- release root: `/srv/www/cursed-drone/releases/`;
- active symlink: `/srv/www/cursed-drone/current`;
- preview URL: `https://cursed-drone.myldy.ru/`.

Caddy, HTTPS, Basic Auth and domain configuration are managed centrally outside this repository. Project workflows must not edit Caddy configuration, obtain TLS certificates, change firewall or systemd settings, alter other runners, change global server permissions, or touch another project's directory.

GitHub Pages remains enabled until the Myldy-hosted preview is accepted separately.

## Trust model

Substantial jobs print `runner.name`, `runner.environment`, `runner.os`, `runner.arch`, `github.ref` and the exact `github.sha` before doing work.

This is a public repository. Self-hosted jobs must not execute untrusted fork code. `pull_request` CI is guarded so that only pull requests whose head repository is `myldy20/cursed-drone` can reach self-hosted runners. `pull_request_target` is not used to checkout or execute pull-request code.

No GitHub-hosted fallback is allowed. If the home runner is unavailable, heavy work waits/fails instead of installing multi-gigabyte toolchains on the Selectel VPS.

## Disk policy and cleanup

Before heavy work the home runner checks the filesystem containing the Actions workspace. At 80% usage the job warns; at 85% or more it refuses to start heavy build/toolchain setup; at 90% or more disk investigation is required before CI continues.

The Selectel VPS checks `/` before deploy/rollback operations. At 80% usage it warns; at 90% it refuses deployment work. Heavy build steps are never a VPS fallback.

Cleanup is part of the workflow contract, including failure/cancellation paths. Jobs use `trap` and/or `if: always()` to remove project build directories, temporary Android/Gradle/Emscripten/Playwright data, browser profiles, temporary deployment input, staging directories and temporary symlinks. Browser/dev-server processes are killed by their recorded PIDs rather than broad process matching.

Diagnostic Actions artifacts use short retention. Browser/benchmark evidence is retained for three days. Deployment bundles are retained for one day.

## Preview deployment

`.github/workflows/deploy-mydly.yml` is the authoritative Myldy preview deployment.

Preferred flow:

```text
trusted commit
→ move/update preview branch
→ home runner disk guard
→ native core tests on home runner
→ WebAssembly build on home runner
→ Playwright/browser smoke on home runner
→ compact checksummed tar.gz artifact (1-day retention)
→ VPS disk guard
→ download/verify only on VPS
→ staging validation and lightweight HTTP smoke
→ immutable release directory
→ atomic current symlink switch
```

A manual `workflow_dispatch` deploy is also supported. A normal push to `main` does not publish the Myldy preview.

The current topology deliberately uses a short-lived GitHub Actions artifact to transfer the already-built compact web bundle from the home build machine to the VPS. The earlier no-artifact transfer rule applied when the build runner itself lived on the VPS; rebuilding on the production host is now explicitly prohibited. SSH deployment keys are not used.

A failed test/build never starts the VPS activation job and therefore cannot change `current`.

## Immutable releases and build.json

A successful deploy creates a new directory under `/srv/www/cursed-drone/releases/`. Existing release directories are never modified in place.

Retention is **current + at most two previous rollback releases**. After successful activation, older project releases are automatically removed.

Each release includes `build.json` with at least:

- `project`;
- `version`;
- full 40-character `commit_sha`;
- `built_at_utc` from the home build stage;
- `deployed_at_utc` written during VPS activation;
- `content_hash`.

`content_hash` is a deterministic SHA-256 digest of the deployed static payload excluding `build.json` itself. The deployment bundle also carries its own SHA-256 checksum, verified before extraction.

## Rollback

Run the Myldy deploy workflow manually with `action=rollback` and the exact immutable release directory name. Rollback validates that the target belongs to the Cursed Drone release root and atomically points `current` at it.

Rollback does not rebuild anything and does not perform a Git revert.

## GitHub Pages

GitHub Pages remains enabled during the migration period. Its heavy WebAssembly build runs on the home runner; the small `deploy-pages` activation job runs on the VPS selector. The Pages artifact has one-day retention.

## Frozen native 1.0 packages

The verified Cursed Drone 1.0.0 macOS, PortMaster/Knulli and NextUI packages remain immutable and available in the existing GitHub Release. Mandatory maintenance CI covers the portable Linux core, Android ARM64 and WebAssembly on self-hosted infrastructure.

A future native maintenance release for an architecture not available on the existing build machine requires an explicitly approved matching self-hosted runner/toolchain. GitHub-hosted runners and the Selectel production VPS are not fallback build machines.

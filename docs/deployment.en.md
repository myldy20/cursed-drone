# Myldy self-hosted deployment

Cursed Drone follows the shared Myldy deployment standard for `myldy.ru` projects.

## Server boundary

The project deploys only its own static web build on the Myldy VPS:

- server: `135.106.183.22` (`myldysite`, Ubuntu 24.04);
- web server: Caddy, managed centrally outside this repository;
- release root: `/srv/www/cursed-drone/releases/`;
- active symlink: `/srv/www/cursed-drone/current`;
- preview URL: `https://cursed-drone.myldy.ru/`.

Project workflows must not edit Caddy configuration, obtain TLS certificates, change firewall or systemd settings, alter other runners, change global server permissions, or touch another project's directory.

GitHub Pages remains enabled until the Myldy-hosted preview has been accepted separately.

## Runner and trust model

All repository CI and deploy jobs use only:

```yaml
runs-on: [self-hosted, myldy-vps, cursed-drone]
```

Substantial jobs print `runner.name`, `runner.environment`, `runner.os`, `runner.arch`, `github.ref` and the exact `github.sha` before doing work.

This is a public repository. Self-hosted jobs are therefore not allowed to execute code from untrusted forks. `pull_request` CI jobs are guarded so that only pull requests whose head repository is `myldy20/cursed-drone` can reach the VPS runner. `pull_request_target` is not used.

The runner is not reconfigured by project workflows through `sudo`, Docker, systemd or global package installation. Portable SDK/toolchain setup is limited to the runner workspace.

## Preview deployment

`.github/workflows/deploy-mydly.yml` is the authoritative Myldy preview deployment.

Preferred flow:

```text
trusted commit
→ move/update preview branch
→ portable native core tests
→ WebAssembly build
→ browser interaction smoke tests
→ staging directory
→ validation
→ immutable release directory
→ atomic current symlink switch
```

A manual `workflow_dispatch` deploy is also supported. A normal push to `main` does not publish the Myldy preview.

The web build is produced directly on the VPS runner. No SSH deployment key and no GitHub Artifact transfer is used for the Myldy deployment.

## Immutable releases and build.json

A successful deploy creates a new directory under `/srv/www/cursed-drone/releases/`. Existing release directories are never modified in place. The five most recent successful releases are retained by default.

Each release includes `build.json` with at least:

- `project`;
- `version`;
- full 40-character `commit_sha`;
- `built_at_utc`;
- `deployed_at_utc`;
- `content_hash`.

`content_hash` is a deterministic SHA-256 digest of the deployed static payload excluding `build.json` itself.

Tests, build and staging validation happen before `current` is changed. A failed test or build therefore leaves the active version untouched.

## Rollback

Run the Myldy deploy workflow manually with `action=rollback` and the exact immutable release directory name. Rollback validates that the target belongs to the Cursed Drone release root and then atomically points `current` at it.

Rollback does not rebuild anything and does not perform a Git revert.

## Frozen native 1.0 packages

The approved Myldy runner is a Linux/X64 VPS without Docker and project workflows do not modify the server to add cross-platform infrastructure. GitHub-hosted fallback is also prohibited. Mandatory maintenance CI therefore covers the portable Linux core, Android ARM64 and WebAssembly, but it does not rebuild native macOS or AArch64 handheld packages.

The verified Cursed Drone 1.0.0 macOS, PortMaster/Knulli and NextUI packages remain immutable and available in the existing GitHub Release. This matches the feature-complete/maintenance-only status. A future native maintenance release for macOS or handhelds first requires an explicitly approved self-hosted runner/toolchain for that architecture; GitHub-hosted runners are not a fallback.

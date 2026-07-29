#!/usr/bin/env python3
"""Require a release version bump whenever shipped runtime code changes."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

SEMVER = re.compile(r"^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$")


def git(*args: str) -> str:
    return subprocess.check_output(["git", *args], text=True).strip()


def normalized_base(candidate: str) -> str:
    if candidate and set(candidate) != {"0"}:
        try:
            git("cat-file", "-e", f"{candidate}^{{commit}}")
            return candidate
        except subprocess.CalledProcessError:
            pass
    try:
        return git("rev-parse", "HEAD^")
    except subprocess.CalledProcessError:
        return git("rev-list", "--max-parents=0", "HEAD")


def is_shipped_runtime(path: str) -> bool:
    if path == "CMakeLists.txt":
        return True
    if path.startswith(("src/", "include/", "android/", "third_party/")):
        return True
    if path.startswith("web/") and not path.startswith("web/tests/"):
        return True
    if path.startswith("tools/package_"):
        return True
    return False


def parse_version(text: str, label: str) -> tuple[int, int, int]:
    match = SEMVER.fullmatch(text.strip())
    if not match:
        raise ValueError(f"{label} VERSION is not semantic x.y.z: {text!r}")
    return tuple(int(part) for part in match.groups())


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("base", nargs="?", default="")
    args = parser.parse_args()

    base = normalized_base(args.base)
    changed = [
        line for line in git("diff", "--name-only", f"{base}...HEAD").splitlines()
        if line
    ]
    runtime = [path for path in changed if is_shipped_runtime(path)]
    if not runtime:
        print("No shipped runtime changes; VERSION bump not required.")
        return 0

    head_text = Path("VERSION").read_text(encoding="utf-8").strip()
    try:
        base_text = git("show", f"{base}:VERSION")
        head_version = parse_version(head_text, "head")
        base_version = parse_version(base_text, "base")
    except (subprocess.CalledProcessError, ValueError) as error:
        print(f"Version discipline error: {error}", file=sys.stderr)
        return 1

    if head_version <= base_version:
        print("Shipped runtime files changed without a newer VERSION:", file=sys.stderr)
        for path in runtime:
            print(f"  - {path}", file=sys.stderr)
        print(f"Base VERSION: {base_text}; head VERSION: {head_text}", file=sys.stderr)
        return 1

    release_notes = Path("docs/releases") / f"v{head_text}.md"
    if not release_notes.is_file():
        print(f"Missing release notes: {release_notes}", file=sys.stderr)
        return 1

    print(f"Runtime change accepted: {base_text} -> {head_text}")
    print(f"Release notes: {release_notes}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

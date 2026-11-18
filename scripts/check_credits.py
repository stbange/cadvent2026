#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Checks that every asset file is listed in assets/CREDITS.md.

Each asset must appear as a backticked path relative to assets/, e.g.
``| `hub/tree.vox` | Jane Doe | CC BY 4.0 | ... |``. Entries for files that no
longer exist are reported as stale.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

IGNORED = {"CREDITS.md", ".gitkeep", "README.md"}
ENTRY_RE = re.compile(r"`([^`]+)`")


def asset_files(assets: Path) -> set[str]:
    return {
        f.relative_to(assets).as_posix()
        for f in assets.rglob("*")
        if f.is_file() and f.name not in IGNORED
    }


def credited(credits_file: Path) -> set[str]:
    if not credits_file.exists():
        return set()
    entries: set[str] = set()
    for line in credits_file.read_text(encoding="utf-8").splitlines():
        if line.lstrip().startswith("|"):
            entries.update(ENTRY_RE.findall(line))
    return entries


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    args = ap.parse_args(argv)
    assets = args.root / "assets"
    files = asset_files(assets) if assets.exists() else set()
    entries = credited(assets / "CREDITS.md")
    missing = sorted(files - entries)
    stale = sorted(e for e in entries - files if "/" in e or "." in e)
    print(f"credits: {len(files)} asset files, {len(missing)} uncredited, {len(stale)} stale entries")
    for m in missing:
        print(f"  UNCREDITED {m}")
    for s in stale:
        print(f"  STALE {s}")
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())

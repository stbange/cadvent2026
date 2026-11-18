#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Checks that every OpenSpec scenario is covered by an automated test.

A scenario "Portal sealed before its day" in capability "advent-calendar" maps
to the test id ``advent_calendar__portal_sealed_before_its_day``. The id must
appear somewhere in the test sources (a test function name, a ctest name, a
Playwright test title, ...).

Scenarios in openspec/specs/ (implemented, archived changes) MUST be covered;
missing ones fail the check. Scenarios in active changes under
openspec/changes/ are reported as pending unless --strict-changes is given.
"""
from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

SCENARIO_RE = re.compile(r"^####\s+Scenario:\s*(.+?)\s*$")
TEST_SUFFIXES = {".c", ".h", ".cmake", ".txt", ".ts", ".js", ".gts", ".py"}


@dataclass(frozen=True)
class Scenario:
    capability: str
    title: str
    spec_file: Path
    implemented: bool

    @property
    def test_id(self) -> str:
        return f"{slug(self.capability)}__{slug(self.title)}"


def slug(text: str) -> str:
    """Lower-case identifier made of [a-z0-9_]."""
    s = re.sub(r"[^a-z0-9]+", "_", text.lower())
    return s.strip("_")


def capability_of(spec_file: Path, root: Path) -> str:
    """openspec/specs/<cap>/spec.md or openspec/changes/<id>/specs/<cap>/spec.md."""
    parts = spec_file.relative_to(root).parts
    idx = len(parts) - 1 - parts[::-1].index("specs")
    return "-".join(parts[idx + 1 : -1])


def collect_scenarios(openspec_dir: Path) -> list[Scenario]:
    found: list[Scenario] = []
    for spec_file in sorted(openspec_dir.rglob("spec.md")):
        rel = spec_file.relative_to(openspec_dir).parts
        if "archive" in rel:
            continue  # archived deltas are merged into openspec/specs
        implemented = rel[0] == "specs"
        cap = capability_of(spec_file, openspec_dir)
        for line in spec_file.read_text(encoding="utf-8").splitlines():
            m = SCENARIO_RE.match(line)
            if m:
                found.append(Scenario(cap, m.group(1), spec_file, implemented))
    return found


def collect_test_text(test_dirs: list[Path]) -> str:
    chunks: list[str] = []
    for d in test_dirs:
        if not d.exists():
            continue
        for f in sorted(d.rglob("*")):
            if f.is_file() and f.suffix in TEST_SUFFIXES and "node_modules" not in f.parts:
                chunks.append(f.read_text(encoding="utf-8", errors="replace"))
    return "\n".join(chunks)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    ap.add_argument("--strict-changes", action="store_true",
                    help="also fail on uncovered scenarios of active changes")
    ap.add_argument("--verbose", action="store_true", help="list every pending scenario")
    args = ap.parse_args(argv)

    scenarios = collect_scenarios(args.root / "openspec")
    text = collect_test_text([args.root / "tests"])
    missing = [s for s in scenarios if s.test_id not in text]
    required = [s for s in missing if s.implemented or args.strict_changes]
    pending = [s for s in missing if s not in required]

    covered = len(scenarios) - len(missing)
    print(f"traceability: {covered}/{len(scenarios)} scenarios covered, "
          f"{len(required)} missing (required), {len(pending)} pending (active changes)")
    for s in required:
        print(f"  MISSING {s.test_id}  ({s.spec_file.relative_to(args.root)}: {s.title})")
    if args.verbose:
        for s in pending:
            print(f"  pending {s.test_id}")
    return 1 if required else 0


if __name__ == "__main__":
    sys.exit(main())

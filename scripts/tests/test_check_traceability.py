# SPDX-License-Identifier: MIT
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import check_traceability as ct  # noqa: E402

SPEC = """# Spec
## Requirements
### Requirement: Daily portal unlock
The system SHALL unlock.

#### Scenario: Portal sealed before its day
- **WHEN** x
- **THEN** y

#### Scenario: Time zone independence
- **WHEN** x
- **THEN** y
"""


def make_repo(tmp_path: Path, implemented: bool, test_src: str) -> Path:
    if implemented:
        spec = tmp_path / "openspec/specs/advent-calendar/spec.md"
    else:
        spec = tmp_path / "openspec/changes/bootstrap/specs/advent-calendar/spec.md"
    spec.parent.mkdir(parents=True)
    spec.write_text(SPEC, encoding="utf-8")
    archived = tmp_path / "openspec/changes/archive/old/specs/advent-calendar/spec.md"
    archived.parent.mkdir(parents=True)
    archived.write_text("#### Scenario: Ancient history\n", encoding="utf-8")
    t = tmp_path / "tests/unit/test_calendar.c"
    t.parent.mkdir(parents=True)
    t.write_text(test_src, encoding="utf-8")
    (tmp_path / "tests/node_modules").mkdir()
    (tmp_path / "tests/node_modules/x.js").write_text("advent_calendar__time_zone_independence")
    return tmp_path


def test_slug():
    assert ct.slug("Portal sealed before its day") == "portal_sealed_before_its_day"
    assert ct.slug("  Opens at 00:00 (local)! ") == "opens_at_00_00_local"
    assert ct.slug("advent-calendar") == "advent_calendar"


def test_capability_paths(tmp_path):
    root = tmp_path / "openspec"
    assert ct.capability_of(root / "specs/hub-hall/spec.md", root) == "hub-hall"
    assert ct.capability_of(root / "changes/c/specs/a/b/spec.md", root) == "a-b"


def test_implemented_scenario_missing_fails(tmp_path, capsys):
    root = make_repo(tmp_path, True, "void test_advent_calendar__portal_sealed_before_its_day(void);")
    assert ct.main(["--root", str(root)]) == 1
    out = capsys.readouterr().out
    assert "1/2 scenarios covered" in out
    assert "MISSING advent_calendar__time_zone_independence" in out


def test_all_implemented_covered_passes(tmp_path, capsys):
    src = "advent_calendar__portal_sealed_before_its_day\nadvent_calendar__time_zone_independence\n"
    root = make_repo(tmp_path, True, src)
    assert ct.main(["--root", str(root)]) == 0
    assert "2/2 scenarios covered" in capsys.readouterr().out


def test_active_change_is_pending_unless_strict(tmp_path, capsys):
    root = make_repo(tmp_path, False, "")
    assert ct.main(["--root", str(root), "--verbose"]) == 0
    out = capsys.readouterr().out
    assert "2 pending" in out
    assert "pending advent_calendar__portal_sealed_before_its_day" in out
    assert ct.main(["--root", str(root), "--strict-changes"]) == 1


def test_missing_tests_dir(tmp_path):
    assert ct.collect_test_text([tmp_path / "nope"]) == ""

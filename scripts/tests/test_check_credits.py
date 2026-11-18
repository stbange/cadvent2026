# SPDX-License-Identifier: MIT
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import check_credits as cc  # noqa: E402

CREDITS = """# Credits
| File | Author | Licence |
|---|---|---|
| `hub/tree.vox` | A. Author | CC BY 4.0 |
| `gone/old.png` | B. Author | CC BY 4.0 |
Mentioning `hub/ignored.png` outside a table does not count.
"""


def test_reports_uncredited_and_stale(tmp_path, capsys):
    a = tmp_path / "assets"
    (a / "hub").mkdir(parents=True)
    (a / "hub/tree.vox").write_bytes(b"VOX ")
    (a / "hub/ignored.png").write_bytes(b"x")
    (a / ".gitkeep").write_text("")
    (a / "CREDITS.md").write_text(CREDITS, encoding="utf-8")
    assert cc.main(["--root", str(tmp_path)]) == 1
    out = capsys.readouterr().out
    assert "UNCREDITED hub/ignored.png" in out
    assert "STALE gone/old.png" in out


def test_all_credited_passes(tmp_path):
    a = tmp_path / "assets/hub"
    a.mkdir(parents=True)
    (a / "tree.vox").write_bytes(b"VOX ")
    (tmp_path / "assets/CREDITS.md").write_text(CREDITS, encoding="utf-8")
    assert cc.main(["--root", str(tmp_path)]) == 0


def test_no_assets_dir_passes(tmp_path):
    assert cc.main(["--root", str(tmp_path)]) == 0
    assert cc.credited(tmp_path / "missing.md") == set()

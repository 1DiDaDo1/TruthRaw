from __future__ import annotations

import hashlib
from pathlib import Path
import subprocess
import tempfile
import unittest

from tools.recover_v19_historical_lineage_v01 import scan_history


def _run(repo: Path, *args: str) -> str:
    p = subprocess.run(
        ["git", "-C", str(repo), *args],
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    return p.stdout


def _init_repo(root: Path) -> None:
    _run(root, "init")
    _run(root, "config", "user.email", "truthraw-ci@example.invalid")
    _run(root, "config", "user.name", "TruthRaw CI")


def _commit_all(root: Path, message: str) -> None:
    _run(root, "add", "-A")
    _run(root, "commit", "-m", message)


class HistoricalLineageRecoveryV01Tests(unittest.TestCase):
    def test_deleted_exact_blob_is_recovered_from_history(self):
        with tempfile.TemporaryDirectory() as td:
            repo = Path(td)
            _init_repo(repo)
            payload = b"exact historical source bytes\nline two\n"
            p = repo / "staging" / "old" / "core.cpp"
            p.parent.mkdir(parents=True)
            p.write_bytes(payload)
            _commit_all(repo, "historical source")

            p.unlink()
            _commit_all(repo, "remove staging source")
            self.assertFalse(p.exists())

            expected = hashlib.sha256(payload).hexdigest()
            targets = {
                "canonical_v4_7i_core.cpp": {
                    "sha256": expected,
                    "required": True,
                    "path_match": lambda path: path.endswith("/core.cpp"),
                }
            }
            r = scan_history(repo, targets)
            rec = r["records"]["canonical_v4_7i_core.cpp"]
            self.assertTrue(r["pass"])
            self.assertTrue(rec["exact_recovered"])
            self.assertEqual(rec["status"], "RECOVERED_EXACT")
            self.assertEqual(rec["exact_occurrences"][0]["sha256"], expected)
            self.assertEqual(rec["exact_occurrences"][0]["bytes"], len(payload))

    def test_same_name_wrong_bytes_do_not_close_sha_gate(self):
        with tempfile.TemporaryDirectory() as td:
            repo = Path(td)
            _init_repo(repo)
            wrong = b"looks plausible but is not the frozen source\n"
            p = repo / "scientific_master_digest_v0_1.cpp"
            p.write_bytes(wrong)
            _commit_all(repo, "wrong candidate")

            expected = hashlib.sha256(b"different exact bytes\n").hexdigest()
            targets = {
                "scientific_master_digest_v0_1.cpp": {
                    "sha256": expected,
                    "required": True,
                    "path_match": lambda path: path.endswith("scientific_master_digest_v0_1.cpp"),
                }
            }
            r = scan_history(repo, targets)
            rec = r["records"]["scientific_master_digest_v0_1.cpp"]
            self.assertFalse(r["pass"])
            self.assertFalse(rec["exact_recovered"])
            self.assertEqual(rec["status"], "CANDIDATES_FOUND_NO_SHA256_MATCH")
            self.assertNotEqual(rec["all_candidates"][0]["sha256"], expected)

    def test_renamed_exact_blob_is_content_recovered_without_path_authority(self):
        with tempfile.TemporaryDirectory() as td:
            repo = Path(td)
            _init_repo(repo)
            payload = b"same immutable source payload\n"
            first = repo / "old" / "uncertainty_core_v5_0g.py"
            first.parent.mkdir(parents=True)
            first.write_bytes(payload)
            _commit_all(repo, "original extractor")

            moved = repo / "archive" / "uncertainty_core_v5_0g.py"
            moved.parent.mkdir(parents=True)
            _run(repo, "mv", str(first.relative_to(repo)), str(moved.relative_to(repo)))
            _commit_all(repo, "move extractor")

            expected = hashlib.sha256(payload).hexdigest()
            targets = {
                "uncertainty_core_v5_0g.py": {
                    "sha256": expected,
                    "required": True,
                    "path_match": lambda path: path.endswith("/uncertainty_core_v5_0g.py"),
                }
            }
            r = scan_history(repo, targets)
            rec = r["records"]["uncertainty_core_v5_0g.py"]
            self.assertTrue(r["pass"])
            self.assertTrue(rec["exact_recovered"])
            # Same bytes have the same Git blob SHA-1. Historical path count is
            # therefore intentionally deduplicated by object identity.
            self.assertEqual(len(rec["exact_occurrences"]), 1)

    def test_optional_missing_target_does_not_override_required_exact_pass(self):
        with tempfile.TemporaryDirectory() as td:
            repo = Path(td)
            _init_repo(repo)
            payload = b"required source\n"
            p = repo / "required.cpp"
            p.write_bytes(payload)
            _commit_all(repo, "required only")
            expected = hashlib.sha256(payload).hexdigest()
            targets = {
                "required.cpp": {
                    "sha256": expected,
                    "required": True,
                    "path_match": lambda path: path == "required.cpp",
                },
                "optional_probe.cpp": {
                    "sha256": "0" * 64,
                    "required": False,
                    "path_match": lambda path: path == "optional_probe.cpp",
                },
            }
            # scan_history's convenience summary names the production probe and
            # extractor, so custom-target tests should inspect records directly.
            # Add production-name optional/required aliases to exercise summary.
            targets["probe_dynamic_authority_v19.cpp"] = {
                "sha256": "0" * 64,
                "required": False,
                "path_match": lambda path: path == "never-probe.cpp",
            }
            targets["uncertainty_core_v5_0g.py"] = {
                "sha256": expected,
                "required": True,
                "path_match": lambda path: path == "required.cpp",
            }
            r = scan_history(repo, targets)
            self.assertTrue(r["pass"])
            self.assertFalse(r["v19_local_probe_recovered_exact"])


if __name__ == "__main__":
    unittest.main()

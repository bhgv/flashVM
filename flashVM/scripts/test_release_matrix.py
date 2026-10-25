#!/usr/bin/env python3
"""Tamper, relocation, missing-platform and CLI tests for native release bundles.

Reports are synthetic fixtures. No foreign-platform execution is claimed.
"""
import contextlib
import copy
import io
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch
from ctest_evidence import inventory, results
import release_matrix as matrix


COMMIT = "a" * 40
VERSION = "0.643"


class MatrixTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="release matrix ")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.path = self.root / "bundle" / "platform-evidence.json"
        self.directory = self.path.parent / "minimal"
        self.directory.mkdir(parents=True)
        self.key = ("Linux", "x86_64", "minimal")
        self.inv = {
            "version": {"major": 1},
            "tests": [{"name": "smoke", "command": ["/old/host/tool"]}],
        }
        self.xml = (
            '<testsuite tests="1" failures="0" disabled="0" skipped="0">'
            '<testcase name="smoke" status="run"/></testsuite>'
        )
        self.report = {
            "schema": 2,
            "status": "passed",
            "commit": COMMIT,
            "version": VERSION,
            "dirty": False,
            "started_utc": "2026-10-20T00:00:00+00:00",
            "finished_utc": "2026-10-20T00:01:00Z",
            "host": {"system": "Linux", "machine": "AMD64"},
            "source_start": {"commit": COMMIT, "sha256": "b" * 64, "files": 42},
            "source_end": {"commit": COMMIT, "sha256": "b" * 64, "files": 42},
            "profiles": [
                {
                    "name": "minimal",
                    "status": "passed",
                    "native_target": {"system": "Linux", "machine": "x86_64", "pointer_size": 8},
                    "toolchains": {
                        lang: {
                            "id": "Fixture",
                            "version": "1.2.3",
                            "architecture": "",
                            "pointer_size": 8,
                        }
                        for lang in ("C", "CXX")
                    },
                    "commands": [
                        {
                            "phase": phase,
                            "argv": ["never-execute-fixture"],
                            "status": "passed",
                            "returncode": 0,
                        }
                        for phase in matrix.PHASES
                    ],
                }
            ],
        }
        self.write_artifacts()
        self.save()

    def write_artifacts(self, skips=()):
        for name in ("test-inventory.log", "test-inventory-after.log"):
            (self.directory / name).write_text(json.dumps(self.inv))
        (self.directory / "ctest-results.xml").write_text(self.xml)
        self.report["profiles"][0]["tests"] = results(
            self.directory / "ctest-results.xml",
            inventory(self.directory / "test-inventory.log"),
            skips,
        )

    def save(self):
        self.path.write_text(json.dumps(self.report))

    def evaluate(self, required=None, paths=None, rules=None):
        return matrix.evaluate(
            paths or [self.path],
            COMMIT,
            VERSION,
            required if required is not None else [self.key],
            rules,
        )

    def test_complete_subset_is_explicit_and_default_blocks_missing_platforms(self):
        subset = self.evaluate()
        self.assertEqual(subset["status"], "passed")
        self.assertIn("not complete roadmap", subset["scope"])
        full = self.evaluate(matrix.DEFAULT_TARGETS)
        self.assertEqual(full["status"], "blocked")
        self.assertEqual(len(full["cells"]), 16)
        self.assertEqual(sum(c["status"] == "missing" for c in full["cells"]), 15)
        self.assertTrue(
            any(c["system"] == "Android" and c["status"] == "missing" for c in full["cells"])
        )

    def test_relocated_bundle_ignores_foreign_absolute_command_paths(self):
        copied = self.root / "moved bundle with spaces"
        shutil.copytree(self.path.parent, copied)
        result = self.evaluate(paths=[copied / self.path.name])
        self.assertEqual(result["status"], "passed")
        self.assertEqual(result["cells"][0]["tests"]["passed"], 1)

    def test_wrong_revision_version_dirty_schema_status_and_endpoints(self):
        original = copy.deepcopy(self.report)
        edits = [
            ("commit", "c" * 40),
            ("version", "9.0"),
            ("dirty", True),
            ("dirty", 0),
            ("schema", 1),
            ("status", "running"),
            ("source_end", {}),
            ("source_start", {"commit": COMMIT, "sha256": "", "files": True}),
            ("finished_utc", "2026-10-19T00:00:00Z"),
            ("started_utc", "2026-10-20T00:00:00"),
        ]
        for field, value in edits:
            self.report = copy.deepcopy(original)
            self.report[field] = value
            self.save()
            with self.subTest(field=field, value=value), self.assertRaises(ValueError):
                self.evaluate()

    def test_wrong_target_and_compiler_abi(self):
        original = copy.deepcopy(self.report)
        for field, value in [("machine", "aarch64"), ("system", "Windows"), ("pointer_size", 4)]:
            self.report = copy.deepcopy(original)
            self.report["profiles"][0]["native_target"][field] = value
            self.save()
            with self.subTest(field=field), self.assertRaises(ValueError):
                self.evaluate()
        self.report = copy.deepcopy(original)
        self.report["profiles"][0]["toolchains"]["CXX"]["pointer_size"] = 4
        self.save()
        with self.assertRaises(ValueError):
            self.evaluate()

    def test_command_reordering_failure_and_missing_phase(self):
        original = copy.deepcopy(self.report)
        for mode in ["reorder", "failed", "missing", "boolean"]:
            self.report = copy.deepcopy(original)
            commands = self.report["profiles"][0]["commands"]
            if mode == "reorder":
                commands.reverse()
            if mode == "failed":
                commands[0]["status"] = "failed"
            if mode == "missing":
                commands.pop()
            if mode == "boolean":
                commands[0]["returncode"] = False
            self.save()
            with self.subTest(mode=mode), self.assertRaises(ValueError):
                self.evaluate()

    def test_modified_junit_inventory_or_summary_is_rejected(self):
        (self.directory / "ctest-results.xml").write_text(self.xml.replace("smoke", "changed"))
        with self.assertRaises(ValueError):
            self.evaluate()
        self.write_artifacts()
        (self.directory / "test-inventory-after.log").write_text("{}")
        with self.assertRaises(ValueError):
            self.evaluate()
        self.write_artifacts()
        self.report["profiles"][0]["tests"]["passed"] = 500
        self.save()
        with self.assertRaises(ValueError):
            self.evaluate()

    def test_exact_skip_policy_is_independent_of_report(self):
        self.inv["tests"].append({"name": "unsupported", "command": ["/old/host/tool"]})
        self.xml = self.xml.replace('tests="1"', 'tests="2"').replace('skipped="0"', 'skipped="1"')
        self.xml = self.xml.replace(
            "</testsuite>",
            '<testcase name="unsupported" status="notrun"><skipped/></testcase></testsuite>',
        )
        self.write_artifacts(["unsupported"])
        self.save()
        with self.assertRaisesRegex(ValueError, "unacknowledged"):
            self.evaluate()
        accepted = self.evaluate(rules={self.key: ["unsupported"]})
        self.assertEqual(accepted["cells"][0]["tests"]["passed"], 1)
        self.assertEqual(accepted["cells"][0]["tests"]["skipped"], ["unsupported"])

    def test_duplicate_cells_invalid_requirements_and_android_desktop_claim(self):
        with self.assertRaisesRegex(ValueError, "duplicate evidence"):
            self.evaluate(paths=[self.path, self.path])
        for required in [[], [self.key, self.key]]:
            with self.assertRaises(ValueError):
                self.evaluate(required)
        self.report["host"]["system"] = "Android"
        self.save()
        with self.assertRaisesRegex(ValueError, "Android"):
            self.evaluate()

    @unittest.skipUnless(os.name == "posix", "POSIX symlink fixture")
    def test_bundle_symlink_cannot_import_external_evidence(self):
        path = self.directory / "ctest-results.xml"
        outside = self.root / "external.xml"
        shutil.copyfile(path, outside)
        path.unlink()
        path.symlink_to(outside)
        with self.assertRaisesRegex(ValueError, "escapes"):
            self.evaluate()

    def test_cli_replaces_previous_pass_with_blocked_and_invalid(self):
        output = self.root / "output" / "matrix.json"
        base = [
            "release_matrix.py",
            "--report",
            str(self.path),
            "--commit",
            COMMIT,
            "--version",
            VERSION,
            "--output",
            str(output),
        ]
        with patch.object(
            sys, "argv", base + ["--require", ":".join(self.key)]
        ), contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(matrix.main(), 0)
        with patch.object(sys, "argv", base), contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(matrix.main(), 1)
        self.assertEqual(json.loads(output.read_text())["status"], "blocked")
        self.path.write_text("not json")
        with patch.object(sys, "argv", base), contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(matrix.main(), 2)
        self.assertEqual(json.loads(output.read_text())["status"], "invalid")
        self.assertFalse(list(output.parent.glob(".matrix.json.*")))

    def test_no_reports_and_explicit_cell_parsing(self):
        result = matrix.evaluate([], COMMIT, VERSION)
        self.assertEqual(result["status"], "blocked")
        self.assertEqual(matrix.cell("Windows:AMD64:minimal"), ("Windows", "x86_64", "minimal"))
        self.assertEqual(matrix.skip_rule("Linux:x64:minimal:with:colon"), (self.key, "with:colon"))
        for value in ["Linux:x64:../outside", "Other:x64:minimal", "Linux:unknown:minimal"]:
            with self.assertRaises(ValueError):
                matrix.cell(value)


if __name__ == "__main__":
    unittest.main()

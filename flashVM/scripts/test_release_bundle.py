#!/usr/bin/env python3
"""Determinism, manifest and snapshot-validation tests for evidence archives."""
import hashlib
import json
from pathlib import Path
import zipfile
import unittest
from unittest.mock import patch
import release_bundle as bundle
import release_matrix as matrix
import test_release_matrix as fixtures

COMMIT, VERSION = fixtures.COMMIT, fixtures.VERSION


class BundleTests(unittest.TestCase):
    def setUp(self):
        self.fixture = fixtures.MatrixTests()
        self.fixture.setUp()
        self.addCleanup(self.fixture.doCleanups)
        self.output = self.fixture.root / "archives" / "native.zip"

    def create(self):
        return bundle.create_bundle(self.fixture.path, COMMIT, VERSION, self.output)

    def test_deterministic_content_manifest_and_relocated_acceptance(self):
        manifest = self.create()
        first = self.output.read_bytes()
        self.create()
        self.assertEqual(first, self.output.read_bytes())
        moved = self.fixture.root / "extracted bundle"
        with zipfile.ZipFile(self.output) as archive:
            self.assertIsNone(archive.testzip())
            self.assertEqual(archive.namelist(), sorted(archive.namelist()))
            stored = json.loads(archive.read("manifest.json"))
            self.assertEqual(stored, manifest)
            for member in stored["members"]:
                data = archive.read(member["name"])
                self.assertEqual(len(data), member["size"])
                self.assertEqual(hashlib.sha256(data).hexdigest(), member["sha256"])
            for info in archive.infolist():
                self.assertEqual(info.date_time, (1980, 1, 1, 0, 0, 0))
                self.assertEqual(info.external_attr >> 16, 0o100644)
                self.assertEqual(info.compress_type, zipfile.ZIP_STORED)
            # This test extracts only our own known, enumerated producer output.
            archive.extractall(moved)
        result = matrix.evaluate(
            [moved / "platform-evidence.json"], COMMIT, VERSION, [self.fixture.key]
        )
        self.assertEqual(result["status"], "passed")

    def test_unrelated_files_and_logs_are_not_exported(self):
        (self.fixture.path.parent / "unrelated.txt").write_text("not an artifact")
        (self.fixture.directory / "configure.log").write_text("not exported")
        self.create()
        with zipfile.ZipFile(self.output) as archive:
            self.assertNotIn("unrelated.txt", archive.namelist())
            self.assertNotIn("minimal/configure.log", archive.namelist())
            self.assertEqual(len(archive.namelist()), 5)

    def test_invalid_source_does_not_replace_previous_valid_archive(self):
        self.create()
        previous = self.output.read_bytes()
        (self.fixture.directory / "ctest-results.xml").write_text("invalid")
        with self.assertRaises(ValueError):
            self.create()
        self.assertEqual(previous, self.output.read_bytes())
        self.assertFalse(list(self.output.parent.glob(".native-evidence-*")))

    def test_edit_between_initial_validation_and_copying_is_rejected(self):
        real = bundle.report_cells
        calls = []

        def mutate(*args, **kwargs):
            value = real(*args, **kwargs)
            calls.append(args[0])
            if len(calls) == 1:
                (self.fixture.directory / "ctest-results.xml").write_text(
                    "changed after validation"
                )
            return value

        with patch.object(bundle, "report_cells", side_effect=mutate), self.assertRaises(
            ValueError
        ):
            self.create()
        self.assertFalse(self.output.exists())
        self.assertFalse(list(self.output.parent.glob(".native-evidence-*")))

    def test_export_cannot_overwrite_its_input_bundle(self):
        with self.assertRaises(ValueError):
            bundle.create_bundle(
                self.fixture.path, COMMIT, VERSION, self.fixture.directory / "archive.zip"
            )


if __name__ == "__main__":
    unittest.main()

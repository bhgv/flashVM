#!/usr/bin/env python3
"""Exercise automatic Coco/R SDK selection with real shell paths, including stale overrides."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]  # Workspace whose bundled SDK is selected.
SELECTOR = ROOT / "tools/coco-r-C/select-sdk.sh"  # Sourced Bash selector under test.
FRAMES = ("scan_c.frm", "scan_h.frm", "parser_c.frm", "parser_h.frm")  # Complete C frame contract.


class SdkSelectionTests(unittest.TestCase):
    """Check fallback, strict opt-in and path normalization without downloading tools."""

    def probe(self, environment, workspace=ROOT):
        """Invoke the actual selector; environment overrides inherited SDK values; return its result."""
        child = dict(os.environ)
        for name in ("COCOR_ROOT", "COCOR_EXECUTABLE", "COCOR_SOURCE"):
            child.pop(name, None)
        child.update(environment)
        return subprocess.run(
            [
                "bash",
                "-c",
                'set -euo pipefail; source "$1"; select_cocor_sdk "$2" "$3"; '
                'printf "RESULT=%s|%s|%s\\n" "$need_cocor" "$COCOR_ROOT" "$COCOR_EXECUTABLE"',
                "sdk-test",
                str(SELECTOR),
                str(workspace),
                "/tmp/sdk output",
            ],
            env=child,
            capture_output=True,
            text=True,
        )

    def make_sdk(self, root):
        """Create the four frame fixtures and an executable; return its generator path."""
        (root / "frames").mkdir(parents=True)
        for name in FRAMES:
            (root / "frames" / name).write_text("fixture\n")
        program = root / "generator"
        program.write_text("#!/bin/sh\nexit 0\n")
        program.chmod(0o755)
        return program

    def test_default_uses_owned_sources(self):
        """No external SDK or previously built generator is needed for default selection."""
        result = self.probe({})
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=1|" + str(ROOT / "tools/coco-r-C/upstream"), result.stdout)

    def test_stale_or_partial_environment_recovers(self):
        """Each historical failure mode selects a local source build and explains recovery."""
        for environment in (
            {"COCOR_ROOT": "/old/removed/source"},
            {"COCOR_EXECUTABLE": "/old/removed/generator"},
            {"COCOR_ROOT": "/old/removed/source", "COCOR_EXECUTABLE": "/old/tool"},
        ):
            with self.subTest(environment=environment):
                result = self.probe(environment)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn("RESULT=1|", result.stdout)
                self.assertIn("using bundled sources", result.stderr)

    def test_valid_external_pair_and_wrapper_path(self):
        """An owned tools wrapper resolves upstream frames, preserving paths containing spaces."""
        with tempfile.TemporaryDirectory(prefix="Coco R SDK ") as directory:
            wrapper = Path(directory)
            program = self.make_sdk(wrapper / "upstream")
            result = self.probe({"COCOR_ROOT": str(wrapper), "COCOR_EXECUTABLE": str(program)})
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("RESULT=0|" + str(wrapper / "upstream"), result.stdout)

    def test_missing_any_frame_recovers(self):
        """A scanner frame alone must not count as a complete SDK."""
        for name in FRAMES:
            with self.subTest(frame=name), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                program = self.make_sdk(root)
                (root / "frames" / name).unlink()
                result = self.probe({"COCOR_ROOT": str(root), "COCOR_EXECUTABLE": str(program)})
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn("RESULT=1|", result.stdout)

    def test_explicit_external_is_strict(self):
        """Only deliberate external mode rejects incomplete SDK overrides."""
        result = self.probe({"COCOR_SOURCE": "external", "COCOR_ROOT": "/missing"})
        self.assertEqual(result.returncode, 2)
        self.assertIn("external mode is strict", result.stderr)

    def test_bundled_ignores_valid_overrides(self):
        """Explicit bundled mode always selects the owned source build."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            program = self.make_sdk(root)
            result = self.probe(
                {
                    "COCOR_SOURCE": "bundled",
                    "COCOR_ROOT": str(root),
                    "COCOR_EXECUTABLE": str(program),
                }
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("RESULT=1|", result.stdout)

    def test_broken_bundled_snapshot_fails_clearly(self):
        """A missing tracked snapshot is reported instead of pretending a build can proceed."""
        with tempfile.TemporaryDirectory() as directory:
            result = self.probe({}, Path(directory))
            self.assertEqual(result.returncode, 2)
            self.assertIn("bundled SDK is incomplete", result.stderr)

    def test_unknown_mode_is_rejected(self):
        """Misspelled explicit policy is not silently interpreted as automatic mode."""
        result = self.probe({"COCOR_SOURCE": "unknown"})
        self.assertEqual(result.returncode, 2)


class ProseSpellingTests(unittest.TestCase):
    """Guard prose spelling while retaining executable and historical source identifiers."""

    def test_legacy_prose_is_reported(self):
        """The forbidden compact spelling must be found in prose and link labels."""
        from check_cocor_spelling import violations

        self.assertEqual(violations("Use CocoR here.\n[Cocor](tools/coco-r-C/README.md)"), [1, 2])

    def test_identifiers_and_examples_are_preserved(self):
        """Existing executable names, URLs, paths and inline/fenced code remain compatible."""
        from check_cocor_spelling import violations

        source = (
            "Coco/R uses `cocor`, COCOR_ROOT and tools/cocor/compiler.\n"
            "https://example.invalid/cocor\n```sh\ncocor input.atg\n```\n"
        )
        self.assertEqual(violations(source), [])

    def test_prose_after_fence_is_checked(self):
        """Closing a code block must resume prose checks."""
        from check_cocor_spelling import violations

        self.assertEqual(violations("~~~sh\ncocor file.atg\n~~~\nCocoR tool"), [4])


if __name__ == "__main__":
    unittest.main()

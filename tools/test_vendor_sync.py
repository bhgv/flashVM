"""Verify pinned Git imports, dirty source isolation and local-change protection."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().with_name("vendor-sync.py")


# Every test owns both its small Git source and destination snapshot.
class VendorSyncTest(unittest.TestCase):
    def setUp(self):
        self.owner = tempfile.TemporaryDirectory(prefix="vendor-sync-test-")
        self.addCleanup(self.owner.cleanup)
        root = Path(self.owner.name)
        self.source, self.target = root / "git source", root / "snapshot"
        self.source.mkdir()
        self.target.mkdir()
        subprocess.run(["git", "init", "-q", str(self.source)], check=True)
        self.write_source("first")
        self.commit()
        manifest = dict(
            repository="test-local",
            branch="fixture",
            revision=None,
            destination="upstream",
            include=["*.c"],
            exclude=[],
            files={},
        )
        (self.target / "UPSTREAM.json").write_text(json.dumps(manifest))

    # Store a source blob without modifying any production repository.
    def write_source(self, text):
        (self.source / "main.c").write_text(text)

    # Snapshot commits carry explicit test-only identities and no remote operations.
    def commit(self):
        subprocess.run(["git", "-C", str(self.source), "add", "."], check=True)
        subprocess.run(
            [
                "git",
                "-C",
                str(self.source),
                "-c",
                "user.name=fixture",
                "-c",
                "user.email=fixture@example.invalid",
                "commit",
                "-qm",
                "fixture",
            ],
            check=True,
        )

    # Run the same CLI used for real updates, never an in-process implementation mirror.
    def sync(self, *args, accepted=True):
        result = subprocess.run(
            [sys.executable, str(SCRIPT), str(self.target), *args], capture_output=True, text=True
        )
        self.assertEqual(result.returncode, 0 if accepted else 1, result.stderr)
        return result

    # Imports come from the resolved commit even when the checkout is dirty.
    def test_committed_blob_and_local_change_guard(self):
        self.write_source("dirty uncommitted")
        self.sync("--update", "--source", str(self.source))
        copied = self.target / "upstream/main.c"
        self.assertEqual(copied.read_text(), "first")
        self.sync()
        copied.write_text("local adaptation")
        self.sync(accepted=False)
        self.sync("--update", "--source", str(self.source), accepted=False)
        self.assertEqual(copied.read_text(), "local adaptation")

    # New upstream names must not overwrite unrecorded local files.
    def test_new_file_collision_and_update(self):
        self.sync("--update", "--source", str(self.source))
        (self.source / "extra.c").write_text("upstream new")
        self.write_source("second")
        self.commit()
        collision = self.target / "upstream/extra.c"
        collision.write_text("local file")
        self.sync("--update", "--source", str(self.source), accepted=False)
        self.assertEqual((self.target / "upstream/main.c").read_text(), "first")
        collision.unlink()
        self.sync("--update", "--source", str(self.source))
        self.assertEqual((self.target / "upstream/main.c").read_text(), "second")
        self.sync()

    # A symlink must never redirect writes out of the snapshot root.
    def test_parent_symlink_rejected(self):
        outside = self.target.parent / "outside"
        outside.mkdir()
        (self.target / "upstream").symlink_to(outside, target_is_directory=True)
        self.sync("--update", "--source", str(self.source), accepted=False)
        self.assertEqual(list(outside.iterdir()), [])


if __name__ == "__main__":
    unittest.main()

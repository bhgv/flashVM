#!/usr/bin/env python3
"""Check automatic SDL dependency recovery and header-only extraction without network access."""

import hashlib
import importlib.util
import io
from pathlib import Path
import subprocess
import tarfile
import tempfile
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[1]  # Workspace owning the bootstrap implementation.
SPEC = importlib.util.spec_from_file_location(
    "sdl_bootstrap", ROOT / "tools/sdl2/ensure_sdk.py"
)  # Isolated helper import.
BOOTSTRAP = importlib.util.module_from_spec(SPEC)  # Import does not fetch or build anything.
SPEC.loader.exec_module(BOOTSTRAP)


class BootstrapTests(unittest.TestCase):
    """Exercise cache recovery and member selection using locally generated fixtures."""

    def test_header_only_archive(self):
        """Only regular bounded headers/licenses are copied; unsafe/unrelated members are ignored."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive = root / "headers.tar.xz"
            with tarfile.open(archive, "w:xz") as output:
                for name, data in (
                    ("bundle/include/X11/Xlib.h", b"header"),
                    ("bundle/include/X11/../../escape.h", b"bad"),
                    ("bundle/src/engine.c", b"not selected"),
                    ("bundle/COPYING", b"license"),
                ):
                    member = tarfile.TarInfo(name)
                    member.size = len(data)
                    output.addfile(member, io.BytesIO(data))
                member = tarfile.TarInfo("bundle/include/X11/linked.h")
                member.type = tarfile.SYMTYPE
                member.linkname = "/outside"
                output.addfile(member)
            record = {
                "url": "https://example.invalid/headers.tar.xz",
                "root": "bundle",
                "sha256": hashlib.sha256(archive.read_bytes()).hexdigest(),
            }
            BOOTSTRAP.x11_headers(record, root, root / "sdk")
            self.assertEqual((root / "sdk/include/X11/Xlib.h").read_bytes(), b"header")
            self.assertEqual((root / "sdk/licenses/bundle/COPYING").read_bytes(), b"license")
            self.assertFalse((root / "sdk/src").exists())
            self.assertFalse((root / "sdk/include/X11/linked.h").exists())
            self.assertFalse((root / "sdk/escape.h").exists())

    def test_corrupt_cache_download_recovers(self):
        """A corrupt old download is automatically replaced only after checksum verification."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "headers.tar.xz").write_bytes(b"interrupted")
            data = b"complete verified archive fixture"
            response = io.BytesIO(data)
            response.geturl = lambda: "https://example.invalid/headers.tar.xz"
            record = {"url": response.geturl(), "sha256": hashlib.sha256(data).hexdigest()}
            with patch.object(BOOTSTRAP.urllib.request, "urlopen", return_value=response):
                result = BOOTSTRAP.archive(record, root)
            self.assertEqual(result.read_bytes(), data)
            self.assertEqual(list(root.glob("*.part-*")), [])

    def test_partial_git_cache_recovers(self):
        """A Git directory left by an interrupted fetch is automatically completed on retry."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "upstream"
            subprocess.run(["git", "init", "-q", str(source)], check=True)
            (source / "README").write_text("public fixture\n")
            subprocess.run(["git", "-C", str(source), "add", "README"], check=True)
            subprocess.run(
                [
                    "git",
                    "-C",
                    str(source),
                    "-c",
                    "user.name=bhgv",
                    "-c",
                    "user.email=bhgv@bhgv.bhgv",
                    "commit",
                    "-qm",
                    "fixture",
                ],
                check=True,
            )
            revision = subprocess.check_output(
                ["git", "-C", str(source), "rev-parse", "HEAD"], text=True
            ).strip()
            cache = root / "partial"
            subprocess.run(["git", "init", "-q", str(cache)], check=True)
            result = BOOTSTRAP.git_source(str(source), revision, cache)
            self.assertEqual((result / "README").read_text(), "public fixture\n")
            (result / "README").write_text("local edit\n")
            with self.assertRaises(RuntimeError):
                BOOTSTRAP.git_source(str(source), revision, cache)
            self.assertEqual((result / "README").read_text(), "local edit\n")


if __name__ == "__main__":
    unittest.main()

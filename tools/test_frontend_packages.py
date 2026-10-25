#!/usr/bin/env python3
"""Verify installed executable identities and default GUI selection, independent of SDK availability."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]  # Real packaging scripts under test.


class FrontendTests(unittest.TestCase):
    """Tool doubles inspect selection only; real SDL movie acceptance is a separate test."""

    def probe_builder(self, options):
        """Run the actual root selector in a private minimal source tree; return argv logs."""
        with tempfile.TemporaryDirectory(prefix="GUI selection ") as directory:
            root = Path(directory)
            (root / "flashVM").mkdir()
            (root / "flashVM/DEFS.txt").write_text("GRAPHICS_AGG\nHEADLESS\n")
            (root / "tools/sdl2").mkdir(parents=True)
            (root / "build-workspace.sh").write_bytes((ROOT / "build-workspace.sh").read_bytes())
            log = root / "calls"
            cmake = root / "cmake"
            cmake.write_text(
                '#!/bin/sh\nif [ "$1" = "--version" ]; then echo "cmake version 3.10.2"; exit 0; fi\n'
                'printf "%s\\n" "$@" >> "$PROBE_LOG"\n'
            )
            cmake.chmod(0o755)
            (root / "tools/sdl2/ensure_sdk.py").write_text(
                'import os\nfrom pathlib import Path\np=Path(os.environ["PROBE_LOG"])\n'
                'p.write_text(p.read_text()+"SDK_BOOTSTRAP\\n" if p.exists() else "SDK_BOOTSTRAP\\n")\n'
                'print("/private SDL SDK/lib/cmake/SDL2")\n'
            )
            environment = dict(os.environ, CMAKE=str(cmake), PROBE_LOG=str(log))
            result = subprocess.run(
                [
                    "bash",
                    str(root / "build-workspace.sh"),
                    "--projects",
                    "vm",
                    "--no-install",
                    "--build-root",
                    str(root / "output"),
                    *options,
                ],
                env=environment,
                capture_output=True,
                text=True,
            )
            return result, log.read_text() if log.exists() else ""

    def test_default_graphical_build_uses_player_and_audio(self):
        """Normal menu/root VM selection bootstraps the SDK and enables the actual player."""
        result, log = self.probe_builder([])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("SDK_BOOTSTRAP", log)
        self.assertIn("-DFVM_BUILD_DESKTOP_FRONTEND=ON", log)
        self.assertIn("-DFVM_ENABLE_SDL_AUDIO=ON", log)
        self.assertIn("-DSDL2_DIR=/private SDL SDK/lib/cmake/SDL2", log)

    def test_explicit_console_build_never_bootstraps_sdl(self):
        """Offline/no-player builds retain SDK-independent console behavior."""
        result, log = self.probe_builder(["--no-player"])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotIn("SDK_BOOTSTRAP", log)
        self.assertIn("-DFVM_BUILD_DESKTOP_FRONTEND=OFF", log)
        self.assertIn("-DFVM_ENABLE_SDL_AUDIO=OFF", log)

    def test_minimal_profile_never_bootstraps_sdl(self):
        """A no-graphics profile must never be silently presented as a GUI binary."""
        result, log = self.probe_builder(["--vm-profiles", "minimal"])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotIn("SDK_BOOTSTRAP", log)

    def test_installer_selects_actual_graphical_executable(self):
        """The generic flashvm command must copy the player, keeping a separate console command."""
        for graphical in (False, True):
            with self.subTest(graphical=graphical), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                build = root / "build/vm"
                build.mkdir(parents=True)
                profile = ROOT / "flashVM/cmake/profiles/player-jit.defs"
                (build / "CMakeCache.txt").write_text(
                    "FVM_DEFS_FILE:FILEPATH="
                    + str(profile)
                    + "\nFVM_BUILD_DESKTOP_FRONTEND:BOOL="
                    + ("ON" if graphical else "OFF")
                    + "\n"
                )
                for name, text in (
                    ("flashvm_reference_swf", "CONSOLE"),
                    ("flashvm_player", "WINDOW"),
                ):
                    (build / name).write_text("#!/bin/sh\nprintf '%s\\n' '" + text + "'\n")
                out = root / "out"
                result = subprocess.run(
                    [
                        "bash",
                        str(ROOT / "tools/install-workspace.sh"),
                        "--projects",
                        "vm",
                        "--build-root",
                        str(root / "build"),
                        "--out",
                        str(out),
                    ],
                    capture_output=True,
                    text=True,
                )
                self.assertEqual(result.returncode, 0, result.stderr)
                package = out / "flashvm" / ("agg-sdl2" if graphical else "agg-headless")
                actual = subprocess.check_output([str(package / "flashvm")], text=True).strip()
                console = subprocess.check_output(
                    [str(package / "flashvm-console")], text=True
                ).strip()
                self.assertEqual(actual, "WINDOW" if graphical else "CONSOLE")
                self.assertEqual(console, "CONSOLE")
                if graphical:
                    self.assertTrue((out / "flashvm/run-demo.sh").is_file())
                    self.assertTrue((package / "run-demo.sh").is_file())


if __name__ == "__main__":
    unittest.main()

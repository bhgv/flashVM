"""Exercise engine discovery, five CLI choices and interactive selection."""

import contextlib
import importlib.util
import io
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("workspace_menu", ROOT / "build-menu.py")
menu = importlib.util.module_from_spec(spec)
spec.loader.exec_module(menu)


class MenuTests(unittest.TestCase):
    """A menu probe is read-only; actual engine acceptance uses separate builds."""

    def test_list_backends(self):
        """Discovery must work even when optional Meson/make binaries are absent."""
        result = subprocess.run(
            ["python3", str(ROOT / "build-menu.py"), "--list-backends"],
            capture_output=True,
            universal_newlines=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        for name in ["cmake", "meson", "make", "mk", "auto"]:
            self.assertIn(name, result.stdout)

    def test_all_five_choices_reach_the_launcher(self):
        """Dry-run preserves the explicit choice without needing to install that tool."""
        for name in ["cmake", "meson", "make", "mk", "auto"]:
            result = subprocess.run(
                [
                    "python3",
                    str(ROOT / "build-menu.py"),
                    "--projects",
                    "linker",
                    "--backend",
                    name,
                    "--dry-run",
                ],
                capture_output=True,
                universal_newlines=True,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("--backend " + name, result.stdout)

    def test_interactive_choice_and_default(self):
        """Displayed numbering maps mk to 4 and empty input to auto."""
        with contextlib.redirect_stdout(io.StringIO()), patch("builtins.input", return_value="4"):
            self.assertEqual(menu.choose_backend(), "mk")
        with contextlib.redirect_stdout(io.StringIO()), patch("builtins.input", return_value=""):
            self.assertEqual(menu.choose_backend(), "auto")

    def test_incompatible_selection_reprompts(self):
        """The screenshot's all/mk choice must never reach the builder."""
        projects = menu.selection("all")
        with contextlib.redirect_stdout(io.StringIO()), patch(
            "builtins.input", side_effect=["4", "1", "1"]
        ):
            selected, backend = menu.choose_compatible_backend(projects)
        self.assertEqual(selected, projects)
        self.assertEqual(backend, "cmake")

    def test_native_reselection_is_explicit(self):
        """Switching components requires an explicit user choice."""
        with contextlib.redirect_stdout(io.StringIO()), patch(
            "builtins.input", side_effect=["4", "2", "2,4,15,16"]
        ):
            selected, backend = menu.choose_compatible_backend(menu.selection("all"))
        self.assertEqual(backend, "mk")
        self.assertEqual(selected, ["compilers", "linker", "mk", "iyacc"])

    def test_native_menu_skips_unavailable_options(self):
        """A valid mk build asks neither for tests nor for an IDE example."""
        with contextlib.redirect_stdout(io.StringIO()), patch.object(
            menu.sys.stdin, "isatty", return_value=True
        ), patch("builtins.input", side_effect=["2,4", "4", ""]), patch.object(
            menu.subprocess, "call", return_value=0
        ) as run:
            self.assertEqual(menu.main([]), 0)
        command = run.call_args[0][0]
        self.assertIn("mk", command)
        self.assertNotIn("--tests", command)
        self.assertNotIn("--demo", command)

    def test_invalid_cli_rejected_before_launch(self):
        """CLI errors identify every unsupported part without spawning Bash."""
        with contextlib.redirect_stderr(io.StringIO()) as errors, patch.object(
            menu.subprocess, "call"
        ) as run:
            with self.assertRaises(SystemExit) as failure:
                menu.main(["--projects", "all", "--backend", "mk", "--tests", "--demo"])
        self.assertEqual(failure.exception.code, 2)
        self.assertIn("vm", errors.getvalue())
        self.assertIn("--tests/--demo", errors.getvalue())
        run.assert_not_called()

    def test_native_tool_tests_are_supported(self):
        """All three source-built tools can be tested through mk without CMake."""
        menu.validate_backend("mk", ["mk", "iyacc", "limbo-bootstrap"], tests=True)
        with contextlib.redirect_stdout(io.StringIO()), patch.object(
            menu.sys.stdin, "isatty", return_value=True
        ), patch("builtins.input", side_effect=["14,15,16", "4", "", "y"]), patch.object(
            menu.subprocess, "call", return_value=0
        ) as run:
            self.assertEqual(menu.main([]), 0)
        self.assertIn("--tests", run.call_args[0][0])

    def test_no_emu_does_not_offer_demo(self):
        """A full interactive selection must not invite an unavailable demo."""
        with patch.dict(os.environ, {"EMU_EXECUTABLE": ""}), contextlib.redirect_stdout(
            io.StringIO()
        ) as output, patch.object(menu.sys.stdin, "isatty", return_value=True), patch(
            "builtins.input", side_effect=["all", "", "", "", "y"]
        ), patch.object(
            menu.subprocess, "call", return_value=0
        ) as run:
            self.assertEqual(menu.main([]), 0)
        self.assertNotIn("--demo", run.call_args[0][0])
        self.assertIn("Dis/.sbl", output.getvalue())

    def test_all_defaults_builds_actual_windowed_jit(self):
        """Typing all and Enter must request a real player, not only a console library."""
        with contextlib.redirect_stdout(io.StringIO()), patch.object(
            menu.sys.stdin, "isatty", return_value=True
        ), patch("builtins.input", side_effect=["all", "", "", "", ""]), patch.object(
            menu.subprocess, "call", return_value=0
        ) as run, patch.object(
            menu, "show_products", return_value=0
        ):
            self.assertEqual(menu.main([]), 0)
        command = run.call_args[0][0]
        self.assertIn("--player", command)
        self.assertIn("disvm-runtime", command[command.index("--projects") + 1])
        self.assertEqual(command[command.index("--vm-profiles") + 1], "player")
        self.assertNotIn("--no-install", command)
        self.assertNotIn("--no-player", command)

    def test_menu_exposes_both_windowed_variants(self):
        """A menu selection builds both interpreter and JIT instead of overwriting one."""
        args = menu.argparse.Namespace(player=False, no_player=False, vm_profiles=None)
        with contextlib.redirect_stdout(io.StringIO()), patch("builtins.input", return_value="3"):
            menu.choose_presentation(args)
        self.assertEqual(args.vm_profiles, "agg,player")
        self.assertTrue(args.player)

    def test_advanced_menu_settings_need_no_switches(self):
        """Jobs, paths with spaces, config and installation are editable interactively."""
        args = menu.argparse.Namespace(
            jobs=2,
            config="Release",
            build_root=None,
            out=None,
            no_install=False,
            vm_profiles="player",
            dry_run=False,
        )
        answers = [
            "y",
            "1",
            "8",
            "2",
            "Debug",
            "3",
            "build with spaces",
            "4",
            "products with spaces",
            "5",
            "y",
            "",
        ]
        with contextlib.redirect_stdout(io.StringIO()), patch(
            "builtins.input", side_effect=answers
        ):
            menu.choose_settings(args)
        self.assertEqual(args.jobs, 8)
        self.assertEqual(args.config, "Debug")
        self.assertEqual(args.build_root, "build with spaces")
        self.assertEqual(args.out, "products with spaces")
        self.assertFalse(args.no_install)

    def test_success_menu_launches_checks_then_window(self):
        """The installed launchers are directly selectable without typing paths or switches."""
        with tempfile.TemporaryDirectory() as temporary:
            prefix = Path(temporary)
            (prefix / "flashvm").mkdir()
            (prefix / "run-checks.sh").write_text("#!/bin/sh\nexit 0\n")
            (prefix / "flashvm/run-demo.sh").write_text("#!/bin/sh\nexit 0\n")
            args = menu.argparse.Namespace(out=temporary, no_install=False)
            with contextlib.redirect_stdout(io.StringIO()), patch(
                "builtins.input", return_value="3"
            ), patch.object(menu.subprocess, "call", return_value=0) as run:
                self.assertEqual(menu.show_products(args, "/bin/bash", True), 0)
            self.assertEqual(
                run.call_args_list[0].args[0], ["/bin/bash", str(prefix / "run-checks.sh")]
            )
            self.assertEqual(
                run.call_args_list[1].args[0],
                ["/bin/bash", str(prefix / "run-checks.sh"), "--window"],
            )

    def test_failed_build_does_not_offer_stale_products(self):
        """An error must propagate without presenting older installed products as a success."""
        with contextlib.redirect_stdout(io.StringIO()), patch.object(
            menu.subprocess, "call", return_value=7
        ), patch.object(menu, "show_products") as products:
            self.assertEqual(menu.main(["--projects", "vm"]), 7)
        products.assert_not_called()

    def test_menu_launches_native_ide_without_switches(self):
        """Native Dis and IDE examples are real post-build menu choices."""
        with tempfile.TemporaryDirectory() as temporary:
            prefix = Path(temporary)
            (prefix / "disvm-runtime").mkdir()
            (prefix / "run-checks.sh").write_text("#!/bin/sh\nexit 0\n")
            (prefix / "disvm-runtime/run-editor.sh").write_text("#!/bin/sh\nexit 0\n")
            args = menu.argparse.Namespace(out=temporary, no_install=False)
            with contextlib.redirect_stdout(io.StringIO()), patch(
                "builtins.input", return_value="5"
            ), patch.object(menu.subprocess, "call", return_value=0) as run:
                self.assertEqual(menu.show_products(args, "/bin/bash", True), 0)
            self.assertEqual(
                run.call_args[0][0], ["/bin/bash", str(prefix / "disvm-runtime/run-editor.sh")]
            )

    def test_menu_launches_standard_modules_without_switches(self):
        """The retained-library tutorial is available from the post-build menu."""
        with tempfile.TemporaryDirectory() as temporary:
            prefix = Path(temporary)
            (prefix / "disvm-runtime").mkdir()
            (prefix / "run-checks.sh").write_text("#!/bin/sh\nexit 0\n")
            (prefix / "disvm-runtime/run-modules.sh").write_text("#!/bin/sh\nexit 0\n")
            args = menu.argparse.Namespace(out=temporary, no_install=False)
            with contextlib.redirect_stdout(io.StringIO()), patch(
                "builtins.input", return_value="7"
            ), patch.object(menu.subprocess, "call", return_value=0) as run:
                status = menu.show_products(args, "/bin/bash", True)
            if status != 0 or run.call_args[0][0] != [
                "/bin/bash",
                str(prefix / "disvm-runtime/run-modules.sh"),
            ]:
                raise RuntimeError("menu did not launch the retained-library tutorial")

    def test_backend_number_six_is_not_a_launch_menu_choice(self):
        """The six post-build actions must not extend the separate five-engine selector."""
        with contextlib.redirect_stdout(io.StringIO()), patch(
            "builtins.input", side_effect=["6", ""]
        ):
            self.assertEqual(menu.choose_backend(), "auto")

    def test_missing_override_is_reported(self):
        """An invalid CMAKE override is not displayed as a detected system CMake."""
        with patch.dict(os.environ, {"CMAKE": "/missing-test-path/cmake"}):
            self.assertIn("не найден", dict(menu.backend_status())["cmake"])

    def test_numeric_component_selection(self):
        """Keep ASCII number selection compatible with Python before str.isascii."""
        self.assertEqual(menu.selection("2,4"), ["compilers", "linker"])


if __name__ == "__main__":
    unittest.main()

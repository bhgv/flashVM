"""Exercise engine discovery, five CLI choices and interactive selection."""
import contextlib
import importlib.util
import io
import os
from pathlib import Path
import subprocess
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('workspace_menu', ROOT / 'build-menu.py')
menu = importlib.util.module_from_spec(spec)
spec.loader.exec_module(menu)


class MenuTests(unittest.TestCase):
    """A menu probe is read-only; actual engine acceptance uses separate builds."""

    def test_list_backends(self):
        """Discovery must work even when optional Meson/make binaries are absent."""
        result = subprocess.run(['python3', str(ROOT / 'build-menu.py'), '--list-backends'],
                                capture_output=True, universal_newlines=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        for name in ['cmake', 'meson', 'make', 'mk', 'auto']:
            self.assertIn(name, result.stdout)

    def test_all_five_choices_reach_the_launcher(self):
        """Dry-run preserves the explicit choice without needing to install that tool."""
        for name in ['cmake', 'meson', 'make', 'mk', 'auto']:
            result = subprocess.run(['python3', str(ROOT / 'build-menu.py'), '--projects', 'linker',
                                     '--backend', name, '--dry-run'],
                                    capture_output=True, universal_newlines=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn('--backend ' + name, result.stdout)

    def test_interactive_choice_and_default(self):
        """Displayed numbering maps mk to 4 and empty input to auto."""
        with contextlib.redirect_stdout(io.StringIO()), patch('builtins.input', return_value='4'):
            self.assertEqual(menu.choose_backend(), 'mk')
        with contextlib.redirect_stdout(io.StringIO()), patch('builtins.input', return_value=''):
            self.assertEqual(menu.choose_backend(), 'auto')

    def test_incompatible_selection_reprompts(self):
        """The screenshot's all/mk choice must never reach the builder."""
        projects = menu.selection('all')
        with contextlib.redirect_stdout(io.StringIO()), patch(
                'builtins.input', side_effect=['4', '1', '1']):
            selected, backend = menu.choose_compatible_backend(projects)
        self.assertEqual(selected, projects)
        self.assertEqual(backend, 'cmake')

    def test_native_reselection_is_explicit(self):
        """Switching components requires an explicit user choice."""
        with contextlib.redirect_stdout(io.StringIO()), patch(
                'builtins.input', side_effect=['4', '2', '2,4,15,16']):
            selected, backend = menu.choose_compatible_backend(menu.selection('all'))
        self.assertEqual(backend, 'mk')
        self.assertEqual(selected, ['compilers', 'linker', 'mk', 'iyacc'])

    def test_native_menu_skips_unavailable_options(self):
        """A valid mk build asks neither for tests nor for an IDE example."""
        with contextlib.redirect_stdout(io.StringIO()), patch.object(
                menu.sys.stdin, 'isatty', return_value=True), patch(
                'builtins.input', side_effect=['2,4', '4']), patch.object(
                menu.subprocess, 'call', return_value=0) as run:
            self.assertEqual(menu.main([]), 0)
        command = run.call_args[0][0]
        self.assertIn('mk', command)
        self.assertNotIn('--tests', command)
        self.assertNotIn('--demo', command)

    def test_invalid_cli_rejected_before_launch(self):
        """CLI errors identify every unsupported part without spawning Bash."""
        with contextlib.redirect_stderr(io.StringIO()) as errors, patch.object(
                menu.subprocess, 'call') as run:
            with self.assertRaises(SystemExit) as failure:
                menu.main(['--projects', 'all', '--backend', 'mk', '--tests', '--demo'])
        self.assertEqual(failure.exception.code, 2)
        self.assertIn('vm', errors.getvalue())
        self.assertIn('--tests/--demo', errors.getvalue())
        run.assert_not_called()

    def test_missing_override_is_reported(self):
        """An invalid CMAKE override is not displayed as a detected system CMake."""
        with patch.dict(os.environ, {'CMAKE': '/missing-test-path/cmake'}):
            self.assertIn('не найден', dict(menu.backend_status())['cmake'])

    def test_numeric_component_selection(self):
        """Keep ASCII number selection compatible with Python before str.isascii."""
        self.assertEqual(menu.selection('2,4'), ['compilers', 'linker'])


if __name__ == '__main__':
    unittest.main()

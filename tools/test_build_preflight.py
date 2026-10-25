"""Check root invocation syntax and fail-closed selection with isolated tool doubles.

Real CMake 3.10.2 acceptance is recorded separately; these tests inspect argv/cwd.
"""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class BuildInvocationTests(unittest.TestCase):
    """Legacy-compatible invocations must preserve paths and failed test status."""

    def probe(self, empty=False, extra=()):
        """Record actual shell arguments without invoking real compilers in these fixtures."""
        with tempfile.TemporaryDirectory(prefix="legacy build ") as directory:
            root = Path(directory)
            log = root / "calls"
            cmake = root / "cmake"
            ctest = root / "ctest"
            cmake.write_text('#!/bin/sh\nif [ "$1" = "--version" ]; then echo "cmake version 3.10.2"; exit 0; fi\nprintf "CALL %s\\n" "$PWD" >> "$PROBE_LOG"\n'
                             'printf "%s\\n" "$@" >> "$PROBE_LOG"\n')
            ctest.write_text('#!/bin/sh\nprintf "%s\\n" "$@" >> "$PROBE_LOG"\n'
                             'case " $* " in *" -N "*) echo "Total Tests: ' +
                             ('0' if empty else '1') + '" ;; esac\n')
            cmake.chmod(0o755)
            ctest.chmod(0o755)
            environment = os.environ.copy()
            environment.update(CMAKE=str(cmake), CTEST=str(ctest), PROBE_LOG=str(log))
            output = root / "output with spaces"
            result = subprocess.run(['bash', str(ROOT / 'build-workspace.sh'), '--no-install', '--projects', 'linker',
                                     '--build-root', str(output), '--tests', *extra],
                                    capture_output=True, text=True, env=environment)
            return result, log.read_text() if log.exists() else '', str(output)

    def test_legacy_flags_and_working_directory(self):
        """Configure uses a positional source path and CTest uses the build cwd."""
        result, log, output = self.probe()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('CALL ' + output + '/linker', log)
        self.assertIn(str(ROOT / 'linker'), log)
        self.assertIn('-j2', log)
        for option in ['-S\n', '-B\n', '--parallel', '--test-dir', '--no-tests']:
            self.assertNotIn(option, log)

    def test_zero_tests_is_still_an_error(self):
        """Compatibility must not turn an empty suite into successful acceptance."""
        result, _, _ = self.probe(empty=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn('No tests found', result.stderr)

    def test_demo_without_runtime_builds_before_reporting_unavailable(self):
        """Missing emu must not stop compilation or masquerade as demo execution."""
        with tempfile.TemporaryDirectory(prefix='no runtime ') as directory:
            root = Path(directory)
            log = root / 'calls'
            cmake = root / 'cmake'
            cmake.write_text('#!/bin/sh\nif [ "$1" = "--version" ]; then echo "cmake version 3.10.2"; exit 0; fi\n'
                             'printf "%s\\n" "$@" >> "$PROBE_LOG"\n')
            cmake.chmod(0o755)
            environment = os.environ.copy()
            environment.update(CMAKE=str(cmake), PROBE_LOG=str(log),
                               LIMBO_EXECUTABLE=str(cmake), EMU_EXECUTABLE='')
            result = subprocess.run(['bash', str(ROOT / 'build-workspace.sh'), '--no-install', '--projects', 'ide',
                                     '--demo', '--build-root', str(root / 'output')],
                                    capture_output=True, text=True, env=environment)
            self.assertEqual(result.returncode, 3, result.stderr)
            self.assertIn('--build', log.read_text())
            self.assertIn('-DEMU_EXECUTABLE=', log.read_text())
            self.assertIn('Build complete', result.stdout)
            self.assertIn('NOT RUN', result.stderr)

    def test_unknown_backend_is_rejected(self):
        """Validate backend before configuring any component."""
        result, log, _ = self.probe(extra=['--backend', 'unknown'])
        self.assertEqual(result.returncode, 2)
        self.assertEqual(log, '')

    def test_native_unsupported_component_is_rejected(self):
        """Never silently omit a requested component from a native fallback."""
        result = subprocess.run(['bash', str(ROOT / 'build-workspace.sh'), '--backend', 'mk',
                                 '--projects', 'vm'], capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn('does not yet build vm', result.stderr)


if __name__ == '__main__':
    unittest.main()

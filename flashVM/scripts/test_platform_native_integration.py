#!/usr/bin/env python3
"""Run real CMake/C89/C++98/CTest through native reporting and portable export.

The isolated miniature Git project is a tooling integration fixture, not a
replacement for building/testing the complete VM on any target platform.
"""
import argparse
import contextlib
import io
import json
from pathlib import Path
import platform
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import validate_platform as runner
from native_target import architecture
from release_bundle import create_bundle
from release_matrix import evaluate


CMAKE, CTEST, GENERATOR = 'cmake', 'ctest', None


class NativeIntegrationTests(unittest.TestCase):
    def test_actual_native_build_skip_policy_relocation_and_failure_recovery(self):
        with tempfile.TemporaryDirectory(prefix='native pipeline ') as temporary:
            root = Path(temporary) / 'component'
            root.mkdir()
            (root / '.gitignore').write_text('build*/\n')
            (root / 'VERSION.txt').write_text('0.644\n')
            (root / 'cmake' / 'profiles').mkdir(parents=True)
            (root / 'cmake' / 'profiles' / 'minimal.defs').write_text('# fixture\n')
            (root / 'value.c').write_text('int fixture_value(void) { return 42; }\n')
            (root / 'main.cpp').write_text('extern "C" int fixture_value(void);\n'
                                           'int main() { return fixture_value() - 42; }\n')
            python = Path(sys.executable).as_posix()
            (root / 'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.26)
project(native_evidence_fixture LANGUAGES C CXX)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
enable_testing()
add_executable(fixture main.cpp value.c)
add_test(NAME fixture_c89_cpp98 COMMAND fixture)
add_test(NAME fixture_skip COMMAND "''' + python + '''" -c "raise SystemExit(77)")
set_tests_properties(fixture_skip PROPERTIES SKIP_RETURN_CODE 77)
''')
            def git(*args):
                return subprocess.check_output(['git', '-C', str(root)] + list(args),
                                               stderr=subprocess.STDOUT, text=True).strip()
            git('init', '-q')
            git('add', '.')
            git('-c', 'user.name=Fixture', '-c', 'user.email=fixture@example.invalid',
                'commit', '-qm', 'native evidence fixture')
            commit = git('rev-parse', 'HEAD')
            argv = ['validate_platform.py', '--profile', 'minimal', '--cmake', CMAKE,
                    '--ctest', CTEST, '--jobs', '1']
            if GENERATOR:
                argv += ['--generator', GENERATOR]
            def run(extra):
                with patch.object(runner, '__file__', str(root / 'scripts' / 'validate_platform.py')), \
                        patch.object(sys, 'argv', argv + extra), \
                        contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                    return runner.main()
            self.assertEqual(run([]), 1)  # Actual CTest exit zero, rejected unacknowledged skip.
            report_path = root / 'build-platform' / 'platform-evidence.json'
            failed = json.loads(report_path.read_text())
            self.assertEqual(failed['status'], 'failed')
            self.assertIn('unacknowledged', failed['error'])
            self.assertEqual(run(['--allow-skip', 'fixture_skip']), 0)
            passed = json.loads(report_path.read_text())
            self.assertFalse(passed['dirty'])
            self.assertEqual(passed['source_start'], passed['source_end'])
            self.assertEqual(passed['profiles'][0]['tests']['registered'], 2)
            self.assertEqual(passed['profiles'][0]['tests']['passed'], 1)
            key = platform.system(), architecture(platform.machine()), 'minimal'
            rules = {key: ['fixture_skip']}
            self.assertEqual(evaluate([report_path], commit, '0.644', [key], rules)['status'], 'passed')
            create_bundle(report_path, commit, '0.644', Path(temporary) / 'exports' / 'evidence.zip', rules)
            # No old XML success can survive a failed complete rerun.
            (root / 'build-platform' / 'minimal' / 'ctest-results.xml').unlink()
            self.assertEqual(run([]), 1)
            self.assertEqual(json.loads(report_path.read_text())['status'], 'failed')
            # Exercise the production timeout with an actual sleeping child.
            with (root / 'CMakeLists.txt').open('a') as stream:
                stream.write('add_test(NAME fixture_timeout COMMAND "' + python +
                             '" -c "import time; time.sleep(30)")\n')
            git('add', 'CMakeLists.txt')
            git('-c', 'user.name=Fixture', '-c', 'user.email=fixture@example.invalid',
                'commit', '-qm', 'timeout fixture')
            self.assertEqual(run(['--allow-skip', 'fixture_skip', '--test-timeout', '1']), 1)
            timed_out = json.loads(report_path.read_text())
            self.assertEqual(timed_out['status'], 'failed')
            self.assertEqual(timed_out['profiles'][0]['test_timeout_default_seconds'], 1)
            log = (root / 'build-platform' / 'minimal' / 'test.log').read_text()
            self.assertIn('Timeout', log)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--cmake', default=CMAKE)
    parser.add_argument('--ctest', default=CTEST)
    parser.add_argument('--generator')
    args, rest = parser.parse_known_args()
    CMAKE, CTEST, GENERATOR = args.cmake, args.ctest, args.generator
    unittest.main(argv=[sys.argv[0]] + rest)

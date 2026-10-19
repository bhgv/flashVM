#!/usr/bin/env python3
"""Exercise release source provenance against real isolated Git repositories."""
import contextlib
import io
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import validate_platform as runner


class SourceEvidenceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='source evidence ')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / 'component'
        self.root.mkdir()
        self.git('init', '-q')
        self.git('config', 'user.name', 'Fixture')
        self.git('config', 'user.email', 'fixture@example.invalid')
        (self.root / '.gitignore').write_text('build*/\n', encoding='utf-8')
        (self.root / 'VERSION.txt').write_text('0.633\n', encoding='utf-8')
        (self.root / 'input file.cpp').write_bytes(b'original\x00content')
        self.git('add', '.')
        self.git('commit', '-qm', 'fixture')

    def git(self, *args):
        return subprocess.check_output(['git', '-C', str(self.root)] + list(args),
                                       stderr=subprocess.STDOUT)

    def identity(self):
        return runner.source_identity(self.root)

    def test_stable_ignored_build_output_and_component_scope(self):
        original = self.identity()
        build = self.root / 'build-results'
        build.mkdir()
        (build / 'report').write_text('generated')
        (self.root.parent / 'sibling').write_text('independent project')
        self.assertEqual(original, self.identity())
        self.assertEqual(original['files'], 3)

    def test_dirty_to_dirty_changes_and_untracked_files(self):
        path = self.root / 'input file.cpp'
        path.write_bytes(b'first dirty')
        original = self.identity()
        path.write_bytes(b'second dirty')
        self.assertNotEqual(original, self.identity())
        original = self.identity()
        added = self.root / 'new\ninput'
        added.write_bytes(b'new')
        self.assertNotEqual(original, self.identity())
        added.unlink()
        self.assertEqual(original, self.identity())

    def test_deletion_index_only_and_commit_changes(self):
        path = self.root / 'input file.cpp'
        original = self.identity()
        path.unlink()
        self.assertNotEqual(original, self.identity())
        path.write_bytes(b'original\x00content')
        self.assertEqual(original, self.identity())
        path.write_bytes(b'staged')
        self.git('add', 'input file.cpp')
        path.write_bytes(b'original\x00content')
        self.assertNotEqual(original, self.identity())
        self.git('reset', '-q', 'HEAD', '--', 'input file.cpp')
        self.git('commit', '--allow-empty', '-qm', 'new head')
        self.assertNotEqual(original, self.identity())

    @unittest.skipUnless(os.name == 'posix', 'POSIX symlink and executable metadata')
    def test_symlink_target_not_external_content_and_executable_mode(self):
        target = self.root.parent / 'external'
        target.write_text('external data')
        link = self.root / 'link'
        link.symlink_to(target)
        original = self.identity()
        target.write_text('external change outside declared inputs')
        self.assertEqual(original, self.identity())
        link.unlink()
        link.symlink_to('missing target')
        self.assertNotEqual(original, self.identity())
        original = self.identity()
        path = self.root / 'input file.cpp'
        path.chmod(path.stat().st_mode | 0o100)
        self.assertNotEqual(original, self.identity())

    def exercise_main(self, mutate=False, fail=False):
        output = self.root / 'build-platform'
        system = output / 'minimal' / 'CMakeFiles' / '4.0'
        system.mkdir(parents=True, exist_ok=True)
        (system / 'CMakeSystem.cmake').write_text('set(CMAKE_CROSSCOMPILING "FALSE")')
        phases = []
        real_run = subprocess.run

        def execute(command, **kwargs):
            if command[0] == 'git':
                return real_run(command, **kwargs)
            phases.append(command)
            if command[0] == 'ctest':
                if mutate:
                    (self.root / 'input file.cpp').write_bytes(b'changed during tests')
                return subprocess.CompletedProcess(command, 1 if fail else 0)
            return subprocess.CompletedProcess(command, 0)

        with patch.object(runner, '__file__', str(self.root / 'scripts' / 'validate_platform.py')), \
                patch.object(runner, 'tool_version', return_value='fixture 4.0.0'), \
                patch.object(runner, 'cache_values', return_value={}), \
                patch.object(runner, 'compiler_identities', return_value={}), \
                patch.object(runner.subprocess, 'run', side_effect=execute), \
                patch.object(runner.sys, 'argv', ['validate_platform.py', '--profile', 'minimal']), \
                patch.dict(os.environ, {}, clear=True), \
                contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            result = runner.main()
        return result, json.loads((output / 'platform-evidence.json').read_text()), phases

    def test_main_success_records_matching_endpoints(self):
        result, report, phases = self.exercise_main()
        self.assertEqual(result, 0)
        self.assertEqual(report['status'], 'passed')
        self.assertEqual(report['source_start'], report['source_end'])
        self.assertEqual(len(phases), 3)

    def test_main_successful_tools_cannot_mask_source_drift(self):
        result, report, _ = self.exercise_main(mutate=True)
        self.assertEqual(result, 1)
        self.assertEqual(report['status'], 'failed')
        self.assertNotEqual(report['source_start'], report['source_end'])
        self.assertIn('source inputs changed', report['error'])
        self.assertEqual(report['profiles'][0]['status'], 'passed')

    def test_failed_rerun_overwrites_previous_pass(self):
        result, _, _ = self.exercise_main()
        self.assertEqual(result, 0)
        result, report, _ = self.exercise_main(fail=True)
        self.assertEqual(result, 1)
        self.assertEqual(report['status'], 'failed')
        self.assertNotIn('source_end', report)


if __name__ == '__main__':
    unittest.main()

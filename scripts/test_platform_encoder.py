#!/usr/bin/env python3
"""Verify optional encoder validation rejects missing evidence and never reuses fixtures."""
import json
from pathlib import Path
import tempfile
import unittest
from validate_platform import encoder_command, validate_encoder


def discovery(command=None):
    return {'version': {'major': 1, 'minor': 0}, 'tests': [
        {'name': 'flashvm_tests_jit_backends', 'command': command or ['path with spaces/backend']}]}


class EncoderEvidenceTests(unittest.TestCase):
    def test_authoritative_command_keeps_arguments(self):
        command = ['launcher with spaces', '--flag', 'generator with spaces']
        result = encoder_command(discovery(command))
        self.assertEqual(result, command)
        result.append('fixture directory')
        self.assertEqual(len(command), 3)

    def test_missing_duplicate_malformed_test_is_rejected(self):
        documents = [[], {}, {'version': {'major': 2}, 'tests': []},
                     {'version': {'major': 1}, 'tests': [None]},
                     {'version': {'major': 1}, 'tests': []}, discovery()]
        documents[-1]['tests'] *= 2
        for document in documents:
            with self.subTest(document=document), self.assertRaises(ValueError):
                encoder_command(document)
        for command in [None, [], '', [''], [42]]:
            document = discovery()
            document['tests'][0]['command'] = command
            with self.subTest(command=command), self.assertRaises(ValueError):
                encoder_command(document)

    def exercise(self, directory, fail=None, document=None, version='2.1.4\n'):
        build = Path(directory)
        phases = []
        record = {}

        def execute(label, command):
            phases.append((label, command))
            if label == fail:
                raise RuntimeError('injected command failure')
            if label == 'jit-discover':
                (build / (label + '.log')).write_text(json.dumps(document or discovery()), encoding='utf-8')
            if label == 'jit-emulator-version':
                (build / (label + '.log')).write_text(version, encoding='utf-8')
            if label == 'jit-fixtures':
                fixture = Path(command[-1])
                self.assertTrue(fixture.is_dir())
                self.assertEqual(list(fixture.iterdir()), [])
                (fixture / 'current.bin').write_bytes(b'current')
            if label == 'jit-emulation':
                self.assertEqual((Path(command[-1]) / 'current.bin').read_bytes(), b'current')

        try:
            validate_encoder(build, build, 'ctest with spaces', execute, record)
        except (RuntimeError, ValueError):
            return record, phases, False
        return record, phases, True

    def test_success_requires_all_phases_and_new_fixture_directory(self):
        with tempfile.TemporaryDirectory(prefix='encoder spaces ') as directory:
            first, phases, passed = self.exercise(directory)
            second, _, passed_again = self.exercise(directory)
            self.assertTrue(passed and passed_again)
            self.assertEqual(first['jit_encoder']['status'], 'passed')
            self.assertEqual(first['jit_encoder']['unicorn_version'], '2.1.4')
            self.assertNotEqual(first['jit_encoder']['fixtures'], second['jit_encoder']['fixtures'])
            self.assertEqual([label for label, _ in phases],
                             ['jit-discover', 'jit-emulator-version', 'jit-fixtures', 'jit-emulation'])

    def test_each_failed_phase_remains_failed(self):
        for phase in ['jit-discover', 'jit-emulator-version', 'jit-fixtures', 'jit-emulation']:
            with self.subTest(phase=phase), tempfile.TemporaryDirectory() as directory:
                record, phases, passed = self.exercise(directory, fail=phase)
                self.assertFalse(passed)
                self.assertEqual(record['jit_encoder']['status'], 'failed')
                self.assertEqual(phases[-1][0], phase)

    def test_empty_version_and_missing_generator_cannot_pass(self):
        for options in [{'version': ''}, {'document': {'version': {'major': 1}, 'tests': []}}]:
            with self.subTest(options=options), tempfile.TemporaryDirectory() as directory:
                record, _, passed = self.exercise(directory, **options)
                self.assertFalse(passed)
                self.assertEqual(record['jit_encoder']['status'], 'failed')


if __name__ == '__main__':
    unittest.main()

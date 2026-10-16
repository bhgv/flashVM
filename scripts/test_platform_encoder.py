#!/usr/bin/env python3
"""Verify optional encoder validation rejects missing evidence and never reuses fixtures."""
import json
from pathlib import Path
import tempfile
import unittest
from validate_platform import encoder_command, validate_encoder, compiler_identities


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


class CompilerIdentityTests(unittest.TestCase):
    def fixture(self, root, language='C', version='13.3.0', size='8', vendor='GNU'):
        directory = Path(root) / 'CMakeFiles' / '4.4.4'
        directory.mkdir(parents=True, exist_ok=True)
        path = directory / ('CMake' + language + 'Compiler.cmake')
        path.write_text('\n'.join('set(CMAKE_%s_%s "%s")' % (language, suffix, value)
                                  for suffix, value in [('COMPILER_ID', vendor),
                                      ('COMPILER_VERSION', version), ('SIZEOF_DATA_PTR', size),
                                      ('COMPILER_ARCHITECTURE_ID', 'x86_64')]) + '\n', encoding='utf-8')
        return path

    def cache(self):
        return {'CMAKE_CACHE_MAJOR_VERSION': '4', 'CMAKE_CACHE_MINOR_VERSION': '4',
                'CMAKE_CACHE_PATCH_VERSION': '4'}

    def test_active_version_and_independent_languages(self):
        with tempfile.TemporaryDirectory() as root:
            self.fixture(root)
            self.fixture(root, 'CXX', '19.44.35221.0', vendor='MSVC')
            stale = Path(root) / 'CMakeFiles' / '0.0.0'
            stale.mkdir(); (stale / 'CMakeCCompiler.cmake').write_text('invalid stale metadata')
            result = compiler_identities(Path(root), self.cache())
            self.assertEqual(result['C']['id'], 'GNU')
            self.assertEqual(result['CXX']['version'], '19.44.35221.0')
            self.assertEqual(result['C']['pointer_size'], 8)
            self.assertEqual(result['C']['target'], '')
            self.assertIn('4.4.4', result['C']['metadata'])

    def test_missing_language_is_rejected(self):
        with tempfile.TemporaryDirectory() as root:
            self.fixture(root)
            with self.assertRaises(OSError):
                compiler_identities(Path(root), self.cache())

    def test_incomplete_or_computed_identity_is_rejected(self):
        for version, size, vendor in [('', '8', 'GNU'), ('${VERSION}', '8', 'GNU'),
                                       ('13.3.0', '0', 'GNU'), ('13.3.0', '8', '')]:
            with self.subTest(version=version, size=size, vendor=vendor), tempfile.TemporaryDirectory() as root:
                self.fixture(root, version=version, size=size, vendor=vendor)
                self.fixture(root, 'CXX')
                with self.assertRaises(ValueError):
                    compiler_identities(Path(root), self.cache())

    def test_mixed_pointer_abi_is_rejected(self):
        with tempfile.TemporaryDirectory() as root:
            self.fixture(root); self.fixture(root, 'CXX', size='4')
            with self.assertRaisesRegex(ValueError, 'pointer sizes differ'):
                compiler_identities(Path(root), self.cache())

    def test_cache_version_cannot_select_an_arbitrary_path(self):
        for cache in [{}, dict(self.cache(), CMAKE_CACHE_PATCH_VERSION='../other')]:
            with self.subTest(cache=cache), self.assertRaises(ValueError):
                compiler_identities(Path('/unused'), cache)


if __name__ == '__main__':
    unittest.main()

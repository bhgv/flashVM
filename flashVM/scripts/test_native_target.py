#!/usr/bin/env python3
"""Exercise native target policy with isolated CMake system metadata fixtures."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import native_target as target


class NativeTargetTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='native target ')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.cache = {'CMAKE_CACHE_MAJOR_VERSION': '4', 'CMAKE_CACHE_MINOR_VERSION': '4',
                      'CMAKE_CACHE_PATCH_VERSION': '4'}
        self.compilers = {lang: {'pointer_size': 8, 'architecture': ''} for lang in ('C', 'CXX')}
        self.values = {'CMAKE_CROSSCOMPILING': 'FALSE', 'CMAKE_SYSTEM_NAME': 'Linux',
                       'CMAKE_HOST_SYSTEM_NAME': 'Linux', 'CMAKE_SYSTEM_PROCESSOR': 'x86_64',
                       'CMAKE_HOST_SYSTEM_PROCESSOR': 'x86_64'}

    def run_target(self, system='Linux', machine='AMD64', width=8, suffix=''):
        directory = target.active_directory(self.root, self.cache)
        directory.mkdir(parents=True, exist_ok=True)
        (directory / 'CMakeSystem.cmake').write_text(''.join(
            '  set( ' + k + ' "' + v + '" )\n' for k, v in self.values.items()) + suffix)
        with patch.object(target.platform, 'system', return_value=system), \
                patch.object(target.platform, 'machine', return_value=machine), \
                patch.object(target.struct, 'calcsize', return_value=width):
            return target.validate_native_target(self.root, self.cache, self.compilers)

    def test_literal_native_aliases_and_stale_cross_metadata(self):
        stale = self.root / 'CMakeFiles' / '0.0.0'
        stale.mkdir(parents=True)
        (stale / 'CMakeSystem.cmake').write_text('set(CMAKE_CROSSCOMPILING "TRUE")')
        result = self.run_target()
        self.assertEqual(result['machine'], 'x86_64')
        self.assertEqual(result['pointer_size'], 8)
        self.assertIn('4.4.4', result['metadata'])

    def test_cross_flags_missing_and_computed_fields(self):
        for value in ['TRUE', 'true', '${CROSS}', '', 'OFF']:
            self.values['CMAKE_CROSSCOMPILING'] = value
            with self.subTest(value=value), self.assertRaises(ValueError):
                self.run_target()
        self.values.pop('CMAKE_CROSSCOMPILING')
        with self.assertRaises(ValueError):
            self.run_target()

    def test_wrong_host_target_system_and_machine(self):
        for key, value in [('CMAKE_HOST_SYSTEM_NAME', 'Windows'),
                           ('CMAKE_SYSTEM_NAME', 'Android'),
                           ('CMAKE_SYSTEM_PROCESSOR', 'aarch64'),
                           ('CMAKE_HOST_SYSTEM_PROCESSOR', '${PROCESSOR}')]:
            original = self.values[key]
            self.values[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                self.run_target()
            self.values[key] = original

    def test_duplicate_system_and_invalid_cache_path(self):
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            self.run_target(suffix='set(CMAKE_SYSTEM_NAME "Linux")\n')
        self.cache['CMAKE_CACHE_PATCH_VERSION'] = '../other'
        with self.assertRaises(ValueError):
            self.run_target()

    def test_c_and_cpp_pointer_or_architecture_mismatch(self):
        for lang in ('C', 'CXX'):
            self.compilers[lang]['pointer_size'] = 4
            with self.subTest(lang=lang), self.assertRaisesRegex(ValueError, 'pointer ABI'):
                self.run_target()
            self.compilers[lang]['pointer_size'] = 8
            self.compilers[lang]['architecture'] = 'arm64'
            with self.assertRaisesRegex(ValueError, 'compiler architecture'):
                self.run_target()
            self.compilers[lang]['architecture'] = ''
        with self.assertRaisesRegex(ValueError, 'reporter pointer ABI'):
            self.run_target(width=4)

    def test_supported_desktop_policies_are_fixtures_not_foreign_runs(self):
        for system, machine in [('Windows', 'x64'), ('Darwin', 'arm64'),
                                ('Linux', 'riscv64')]:
            for key in ('CMAKE_HOST_SYSTEM_NAME', 'CMAKE_SYSTEM_NAME'):
                self.values[key] = system
            for key in ('CMAKE_HOST_SYSTEM_PROCESSOR', 'CMAKE_SYSTEM_PROCESSOR'):
                self.values[key] = machine
            result = self.run_target(system=system, machine=machine)
            self.assertEqual(result['system'], system)
        with self.assertRaises(ValueError):
            self.run_target(system='Android')

    def test_32bit_policy_requires_a_matching_reporter_and_both_compilers(self):
        for key in ('CMAKE_HOST_SYSTEM_PROCESSOR', 'CMAKE_SYSTEM_PROCESSOR'):
            self.values[key] = 'i686'
        for identity in self.compilers.values():
            identity['pointer_size'] = 4
            identity['architecture'] = 'x86'
        result = self.run_target(machine='i386', width=4)
        self.assertEqual(result['pointer_size'], 4)


if __name__ == '__main__':
    unittest.main()

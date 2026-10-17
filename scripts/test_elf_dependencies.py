#!/usr/bin/env python3
"""Check ELF discovery, dependency policy and fail-first platform evidence."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from elf_dependencies import elf_artifacts, needed_libraries, validate_elf_dependencies


def executable(path, kind=2, endian=1):
    header = bytearray(18)
    header[:4] = b'\x7fELF'
    header[4:7] = bytes([2, endian, 1])
    header[16:18] = kind.to_bytes(2, 'little' if endian == 1 else 'big')
    path.write_bytes(header)
    return path


def discovery(command):
    return {'version': {'major': 1}, 'tests': [{'command': command}]}


class ElfDependencyTests(unittest.TestCase):
    def test_discovery_checks_headers_paths_and_duplicates(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            build = root / 'build with spaces'
            build.mkdir()
            target = executable(build / 'program with spaces')
            shared = executable(build / 'module', 3, 2)
            relocatable = executable(build / 'object.o', 1)
            outside = executable(root / 'outside')
            malformed = build / 'short'; malformed.write_bytes(b'\x7fELF')
            doc = discovery(['/usr/bin/python3', str(target), str(shared), str(target),
                             str(relocatable), str(outside), str(malformed), str(build)])
            self.assertEqual(elf_artifacts(doc, build), sorted([target, shared]))

    def test_missing_or_malformed_discovery_fails(self):
        with tempfile.TemporaryDirectory() as directory:
            build = Path(directory)
            documents = [None, [], {}, {'version': {'major': 2}, 'tests': []},
                         {'version': {'major': 1}, 'tests': [None]}, discovery([]),
                         discovery([42]), discovery(['']), discovery(['/nonexistent'])]
            for doc in documents:
                with self.subTest(doc=doc), self.assertRaises(ValueError):
                    elf_artifacts(doc, build)

    def test_system_dependencies_and_static_artifacts(self):
        output = (' 0x1 (NEEDED) Shared library: [libstdc++.so.6]\n'
                  ' 0x1 (NEEDED) Shared library: [libc.so.6]\n'
                  ' 0x1 (NEEDED) Shared library: [libc.so.6]\n 0x0 (NULL) 0x0\n')
        self.assertEqual(needed_libraries(output), ['libc.so.6', 'libstdc++.so.6'])
        self.assertEqual(needed_libraries('There is no dynamic section in this file.\n'), [])
        self.assertEqual(needed_libraries('0x0 (NULL) 0x0\n'), [])
        self.assertEqual(needed_libraries('0x1 (NEEDED) Shared library: [libzstd.so.1]\n0x0 (NULL) 0'), ['libzstd.so.1'])

    def test_bundled_sonames_are_rejected(self):
        names = ['libflashvm.so', 'libfvm_zlib.so', 'libagg.so.2', 'libnanosvg.so', 'libnanosvg.so.1', 'libcairo.so.2',
                 'libcairo-gobject.so.2', 'libfreetype.so.6', 'libfribidi.so.0',
                 'libgif.so.7', 'libharfbuzz-subset.so.0', 'libjpeg.so.8',
                 'libturbojpeg.so.0', 'libpng16.so.16', 'libunibreak.so.5',
                 'libpixman-1.so.0', 'libsqlite3.so.0', 'libz.so.1', 'libzlib.so',
                 '/path with spaces/libcairo.so.2', '/path with spaces/libnanosvg.so.2']
        for name in names:
            with self.subTest(name=name), self.assertRaises(ValueError):
                needed_libraries('0x1 (NEEDED) Shared library: [' + name + ']\n0x0 (NULL) 0')

    def test_bad_dynamic_evidence_is_rejected(self):
        for output in ['', 'garbage', '0x1 (NEEDED) missing name\n',
                       '0x1 (NEEDED) Shared library: [libc.so.6]\n']:
            with self.subTest(output=output), self.assertRaises(ValueError):
                needed_libraries(output)

    def exercise(self, directory, fail=None, libraries=None, version=None):
        build = Path(directory)
        binary = executable(build / 'artifact')
        record = {'elf_dependencies': {'status': 'passed'}}
        phases = []

        def execute(label, command):
            phases.append((label, command))
            if label == fail:
                raise RuntimeError('injected command failure')
            if label == 'elf-tool':
                output = 'GNU readelf 2.40\n' if version is None else version
            elif label == 'elf-discover':
                output = json.dumps(discovery([str(binary)]))
            else:
                output = ('0x1 (NEEDED) Shared library: [libc.so.6]\n0x0 (NULL) 0\n'
                          if libraries is None else libraries)
            (build / (label + '.log')).write_text(output)

        with patch('elf_dependencies.platform.system', return_value='Linux'):
            try:
                validate_elf_dependencies(build, 'ctest with spaces', 'readelf with spaces', execute, record)
            except (ValueError, RuntimeError):
                self.assertEqual(record['elf_dependencies']['status'], 'failed')
                raise
        return record['elf_dependencies'], phases

    def test_command_arguments_and_success_evidence(self):
        with tempfile.TemporaryDirectory() as directory:
            evidence, phases = self.exercise(directory)
            self.assertEqual(evidence['status'], 'passed')
            self.assertEqual(evidence['readelf_version'], 'GNU readelf 2.40')
            self.assertEqual(evidence['artifacts'][0]['needed'], ['libc.so.6'])
            self.assertEqual(phases[0][1], ['readelf with spaces', '--version'])
            self.assertEqual(phases[-1][1], ['readelf with spaces', '--wide', '--dynamic', str(Path(directory) / 'artifact')])

    def test_failures_replace_stale_pass(self):
        for phase in ['elf-tool', 'elf-discover', 'elf-dynamic-0000']:
            with tempfile.TemporaryDirectory() as directory, self.subTest(phase=phase):
                with self.assertRaises(RuntimeError):
                    self.exercise(directory, fail=phase)
        for kwargs in [{'version': ''}, {'version': 'LLVM tool\n'},
                       {'libraries': 'garbage'},
                       {'libraries': '0x1 (NEEDED) Shared library: [libpng16.so.16]\n0x0 (NULL) 0'},
                       {'libraries': '0x1 (NEEDED) Shared library: [libnanosvg.so.1]\n0x0 (NULL) 0'}]:
            with tempfile.TemporaryDirectory() as directory, self.subTest(kwargs=kwargs):
                with self.assertRaises(ValueError):
                    self.exercise(directory, **kwargs)

    def test_foreign_hosts_fail_without_launching_tools(self):
        record = {}
        with patch('elf_dependencies.platform.system', return_value='Darwin'):
            with self.assertRaises(RuntimeError):
                validate_elf_dependencies(Path('.'), 'ctest', 'readelf',
                                          lambda *args: self.fail('foreign tool launched'), record)
        self.assertEqual(record['elf_dependencies']['status'], 'failed')


if __name__ == '__main__':
    unittest.main()

#!/usr/bin/env python3
"""Exercise explicit SDK selection with actual isolated CMake configure probes.

Host compiler paths deliberately stand in for an SDK here. These are configure
policy fixtures, not foreign-CPU compilation or execution evidence.
"""
import argparse
import platform
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
CMAKE, CC, CXX = 'cmake', 'cc', 'c++'


class ToolchainTests(unittest.TestCase):
    def configure(self, languages, provide_cxx=False, processor=None):
        if processor is None:
            processor = platform.machine()
        with tempfile.TemporaryDirectory(prefix='explicit SDK ') as temporary:
            root = Path(temporary)
            (root / 'CMakeLists.txt').write_text(
                'cmake_minimum_required(VERSION 3.16)\n'
                'project(sdk_fixture LANGUAGES ' + languages + ')\n', encoding='utf-8')
            command = [CMAKE, '-S', str(root), '-B', str(root / 'build'),
                       '-DCMAKE_TOOLCHAIN_FILE=' + str(ROOT / 'toolchains/explicit.cmake'),
                       '-DFVM_TARGET_SYSTEM=' + platform.system(), '-DFVM_TARGET_PROCESSOR=' + processor,
                       '-DFVM_TARGET_CC=' + str(Path(CC).resolve())]
            if provide_cxx:
                command += ['-DFVM_TARGET_CXX=' + str(Path(CXX).resolve())]
            return subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                  text=True)

    def test_c_only_needs_no_cpp_compiler(self):
        result = self.configure('C')
        self.assertEqual(result.returncode, 0, result.stdout)

    def test_cpp_cannot_fall_back_to_host_compiler(self):
        result = self.configure('C CXX')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('FVM_TARGET_CXX_REQUIRED', result.stdout)

    def test_explicit_cpp_reaches_real_compiler_link_probes(self):
        result = self.configure('C CXX', provide_cxx=True)
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn('Detecting CXX compiler ABI info - done', result.stdout)

    def test_missing_processor_is_not_inferred_from_host(self):
        result = self.configure('C', processor='')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('FVM_TARGET_PROCESSOR', result.stdout)


if __name__ == '__main__':
    import shutil
    import sys
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--cmake', default=CMAKE)
    parser.add_argument('--cc', default=CC)
    parser.add_argument('--cxx', default=CXX)
    args, remaining = parser.parse_known_args()
    CMAKE = args.cmake
    CC = shutil.which(args.cc) or args.cc
    CXX = shutil.which(args.cxx) or args.cxx
    unittest.main(argv=[sys.argv[0]] + remaining)

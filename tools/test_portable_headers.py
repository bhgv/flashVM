#!/usr/bin/env python3
"""Compile public C99 headers and reject unsupported target representation.

Negative cases deliberately override already-included limit macros. They test
clear diagnostics, not execution on an actual exotic CPU. Positive cross builds
are recorded separately in toolchains/VALIDATION.md.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
CC = 'cc'
HEADERS = ['compiler/' + lang + '/frontend/frontend.h' for lang in ('as1', 'as2', 'as3', 'limbo')]
HEADERS += ['fir/v1/frontend/firsyn.h', 'fir/v1/include/fir.h', 'linker/include/swl.h']


class PortableHeaderTests(unittest.TestCase):
    def compile(self, headers, override=''):
        source = '#include <limits.h>\n#include <float.h>\n' + override
        source += ''.join('#include "' + (ROOT / path).as_posix() + '"\n' for path in headers)
        source += 'int main(void) { return 0; }\n'
        with tempfile.TemporaryDirectory(prefix='header ABI ') as temporary:
            path = Path(temporary) / 'probe.c'
            path.write_text(source, encoding='utf-8')
            return subprocess.run([CC, '-std=c99', '-pedantic-errors', '-fsyntax-only', str(path)],
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    def test_all_public_headers_coexist_in_c99(self):
        result = self.compile(HEADERS)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_wide_char_target_is_rejected_per_detached_component(self):
        for header in HEADERS:
            with self.subTest(header=header):
                result = self.compile([header], '#undef CHAR_BIT\n#define CHAR_BIT 16\n')
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('8-bit bytes', result.stderr)

    def test_sixteen_bit_int_is_rejected_per_component(self):
        for header in HEADERS:
            with self.subTest(header=header):
                result = self.compile([header], '#undef INT_MAX\n#define INT_MAX 32767\n')
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('32-bit int', result.stderr)

    def test_non_binary64_target_is_rejected_before_payload_copying(self):
        for name, value in [('DBL_MANT_DIG', 24), ('DBL_MAX_EXP', 128), ('FLT_RADIX', 10)]:
            with self.subTest(macro=name):
                result = self.compile(['fir/v1/include/fir.h'],
                                      '#undef ' + name + '\n#define ' + name + ' ' + str(value) + '\n')
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('binary64', result.stderr)

    def test_known_mixed_float_word_order_is_rejected(self):
        result = self.compile(['fir/v1/include/fir.h'],
                              '#undef __FLOAT_WORD_ORDER__\n#define __FLOAT_WORD_ORDER__ 9876\n'
                              '#undef __BYTE_ORDER__\n#define __BYTE_ORDER__ 1234\n')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('word order', result.stderr)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--cc', default=CC)
    args, remaining = parser.parse_known_args()
    CC = args.cc
    import sys
    unittest.main(argv=[sys.argv[0]] + remaining)

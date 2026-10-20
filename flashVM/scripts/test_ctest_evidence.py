#!/usr/bin/env python3
"""CTest inventory/JUnit validation, including actual CTest skip and filter runs."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import ctest_evidence as evidence


CTEST = 'ctest'


class CTestEvidenceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='ctest evidence ')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def inventory(self, tests=None):
        path = self.root / 'inventory.json'
        if tests is None:
            tests = [{'name': 'a', 'command': ['tool']}, {'name': 'b', 'command': ['tool']}]
        path.write_text(json.dumps({'version': {'major': 1}, 'tests': tests}))
        return evidence.inventory(path)

    def junit(self, cases, attrs='tests="2" failures="0" disabled="0" skipped="0"'):
        path = self.root / 'results.xml'
        path.write_text('<testsuite ' + attrs + '>' + cases + '</testsuite>')
        return path

    def test_matching_names_and_content_fingerprints(self):
        expected = self.inventory()
        path = self.junit('<testcase name="b" status="run"/><testcase name="a" status="run"/>')
        result = evidence.results(path, expected)
        self.assertEqual(result['registered'], 2)
        self.assertEqual(result['passed'], 2)
        self.assertEqual(result['skipped'], [])
        self.assertEqual(result['inventory_sha256'], expected['sha256'])
        changed = self.inventory([{'name': 'a', 'command': ['other']},
                                  {'name': 'b', 'command': ['tool']}])
        self.assertNotEqual(expected['sha256'], changed['sha256'])

    def test_invalid_inventories(self):
        for tests in [[], [None], [{'name': 'a'}], [{'name': 'a', 'command': []}],
                      [{'name': 'a', 'command': ['tool']}] * 2,
                      [{'name': '', 'command': ['tool']}]]:
            with self.subTest(tests=tests), self.assertRaises(ValueError):
                self.inventory(tests)

    def test_missing_extra_duplicate_and_statusless_cases(self):
        for cases in ['<testcase name="a" status="run"/>',
                      '<testcase name="a" status="run"/><testcase name="c" status="run"/>',
                      '<testcase name="a" status="run"/><testcase name="a" status="run"/>',
                      '<testcase name="a"/><testcase name="b" status="run"/>']:
            with self.subTest(cases=cases), self.assertRaises(ValueError):
                evidence.results(self.junit(cases), self.inventory())

    def test_skip_needs_exact_acknowledgement_and_an_executed_case(self):
        path = self.junit('<testcase name="a" status="run"/>'
                          '<testcase name="b" status="notrun"><skipped/></testcase>',
                          'tests="2" failures="0" disabled="0" skipped="1"')
        with self.assertRaisesRegex(ValueError, 'unacknowledged'):
            evidence.results(path, self.inventory(), ['b*'])
        result = evidence.results(path, self.inventory(), ['b'])
        self.assertEqual(result['passed'], 1)
        self.assertEqual(result['skipped'], ['b'])
        path = self.junit('<testcase name="a" status="notrun"><skipped/></testcase>',
                          'tests="1" failures="0" disabled="0" skipped="1"')
        expected = self.inventory([{'name': 'a', 'command': ['tool']}])
        with self.assertRaisesRegex(ValueError, 'actually executed'):
            evidence.results(path, expected, ['a'])

    def test_failed_disabled_conflicting_counts_and_entities(self):
        expected = self.inventory([{'name': 'a', 'command': ['tool']}])
        cases = [('<testcase name="a" status="disabled"/>', 'disabled="1" skipped="0" failures="0"'),
                 ('<testcase name="a" status="run"><failure/></testcase>',
                  'disabled="0" skipped="0" failures="1"'),
                 ('<testcase name="a" status="run"><failure/><skipped/></testcase>',
                  'disabled="0" skipped="0" failures="1"'),
                 ('<testcase name="a" status="run"/>', 'disabled="0" skipped="0" failures="1"')]
        for case, attrs in cases:
            with self.subTest(case=case), self.assertRaises(ValueError):
                evidence.results(self.junit(case, 'tests="1" ' + attrs), expected)
        path = self.root / 'entities.xml'
        path.write_text('<!DOCTYPE x [<!ENTITY a "x">]><testsuite/>')
        with self.assertRaisesRegex(ValueError, 'entities'):
            evidence.results(path, expected)

    def test_oversized_inputs(self):
        path = self.root / 'large'
        path.write_bytes(b'x' * 100)
        with patch.object(evidence, 'MAX_EVIDENCE_BYTES', 99), self.assertRaises(ValueError):
            evidence.read_bounded(path)

    def test_alternate_encoding_cannot_hide_entity_declarations(self):
        expected = self.inventory()
        path = self.root / 'encoded.xml'
        xml = '<!DOCTYPE x [<!ENTITY hidden "expanded">]><testsuite/>'
        for encoding in ('utf-16', 'utf-16-le', 'utf-16-be', 'utf-32'):
            path.write_bytes(xml.encode(encoding))
            with self.subTest(encoding=encoding), self.assertRaises(ValueError):
                evidence.results(path, expected)

    def real_ctest(self, extra=(), disabled=False):
        python = Path(sys.executable).as_posix()
        (self.root / 'CTestTestfile.cmake').write_text(
            'add_test(pass "' + python + '" "-c" "pass")\n'
            'add_test(skip "' + python + '" "-c" "raise SystemExit(77)")\n'
            'set_tests_properties(skip PROPERTIES SKIP_RETURN_CODE 77)\n' +
            ('set_tests_properties(pass PROPERTIES DISABLED TRUE)\n' if disabled else ''))
        command = [CTEST, '--test-dir', str(self.root)]
        raw = subprocess.check_output(command + ['--show-only=json-v1'])
        (self.root / 'inventory.json').write_bytes(raw)
        expected = evidence.inventory(self.root / 'inventory.json')
        path = self.root / 'results.xml'
        subprocess.run(command + ['--output-junit', str(path)] + list(extra),
                       check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return path, expected

    def test_actual_ctest_skip_is_not_a_pass(self):
        path, expected = self.real_ctest()
        result = evidence.results(path, expected, ['skip'])
        self.assertEqual((result['registered'], result['passed'], result['skipped']), (2, 1, ['skip']))

    def test_actual_ctest_filtered_success_is_rejected(self):
        path, expected = self.real_ctest(['-R', '^pass$'])
        with self.assertRaisesRegex(ValueError, 'inventory'):
            evidence.results(path, expected)

    def test_actual_ctest_disabled_success_is_rejected(self):
        path, expected = self.real_ctest(disabled=True)
        with self.assertRaisesRegex(ValueError, 'disabled'):
            evidence.results(path, expected, ['skip', 'pass'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--ctest', default=CTEST)
    args, rest = parser.parse_known_args()
    CTEST = args.ctest
    unittest.main(argv=[sys.argv[0]] + rest)

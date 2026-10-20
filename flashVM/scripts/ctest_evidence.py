"""Validate complete native CTest execution, including skipped/disabled tests.

CTest exit zero alone is insufficient: all tests can have been skipped. Bind
JUnit cases to the configured inventory and reject incomplete or stale output.
No code is executed by this module; the platform runner owns tool invocation.
"""
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET


MAX_EVIDENCE_BYTES = 32 * 1024 * 1024


def read_bounded(path):
    """Bound allocation before parsing locally produced tool evidence."""
    with Path(path).open('rb') as stream:
        data = stream.read(MAX_EVIDENCE_BYTES + 1)
    if len(data) > MAX_EVIDENCE_BYTES:
        raise ValueError('CTest evidence exceeds 32 MiB')
    return data


def inventory(path):
    """Require unique named executable tests and fingerprint their definitions."""
    document = json.loads(read_bounded(path))
    if (not isinstance(document, dict) or not isinstance(document.get('version'), dict)
            or document['version'].get('major') != 1
            or not isinstance(document.get('tests'), list) or not document['tests']):
        raise ValueError('empty or invalid CTest inventory')
    names = set()
    for test in document['tests']:
        if not isinstance(test, dict):
            raise ValueError('invalid CTest test definition')
        name, command = test.get('name'), test.get('command')
        if (not isinstance(name, str) or not name or name in names
                or not isinstance(command, list) or not command
                or any(not isinstance(part, str) or not part for part in command)):
            raise ValueError('missing/duplicate test name or unresolved executable')
        names.add(name)
    canonical = json.dumps(document, sort_keys=True, separators=(',', ':'),
                           ensure_ascii=True).encode('ascii')
    return {'names': sorted(names), 'sha256': hashlib.sha256(canonical).hexdigest()}


def results(path, expected, allowed_skips=()):
    """Match every case once, enforce statuses/counts and explicit skip policy.

    A listed skip is an acknowledged limitation, not an executed passing test.
    Disabled tests, failures, unknown statuses and zero executed cases fail.
    """
    data = read_bounded(path)
    # CTest emits UTF-8. Reject alternate encodings rather than letting UTF-16
    # hide forbidden declarations behind interleaved zero bytes.
    text = data.decode('utf-8-sig')
    if '\x00' in text:
        raise ValueError('CTest JUnit must be UTF-8 without NUL')
    if '<!DOCTYPE' in text.upper() or '<!ENTITY' in text.upper():
        raise ValueError('XML document types/entities are forbidden')
    try:
        suite = ET.fromstring(text)
    except ET.ParseError as error:
        raise ValueError('malformed CTest JUnit: ' + str(error)) from error
    if suite.tag != 'testsuite':
        raise ValueError('expected one CTest JUnit testsuite')
    counts = dict(tests=0, failures=0, disabled=0, skipped=0)
    seen, passed, skipped, disabled, failed = set(), [], [], [], []
    for case in suite:
        if case.tag != 'testcase':
            raise ValueError('unexpected CTest JUnit suite element')
        name = case.get('name')
        if not name or name in seen:
            raise ValueError('missing or duplicate JUnit test name')
        seen.add(name)
        counts['tests'] += 1
        status = case.get('status')
        failures = case.findall('failure') + case.findall('error')
        omissions = case.findall('skipped')
        if len(failures) > 1 or len(omissions) > 1 or (failures and omissions):
            raise ValueError('conflicting JUnit outcome')
        if status == 'disabled' and not failures and not omissions:
            counts['disabled'] += 1
            disabled.append(name)
        elif status == 'notrun' and len(omissions) == 1:
            counts['skipped'] += 1
            skipped.append(name)
        elif status == 'run' and not omissions:
            if failures:
                counts['failures'] += 1
                failed.append(name)
            else:
                passed.append(name)
        else:
            raise ValueError('unknown or inconsistent JUnit status for ' + name)
    if seen != set(expected['names']):
        raise ValueError('JUnit test names differ from configured CTest inventory')
    for key, count in counts.items():
        value = suite.get(key)
        if value is None or not value.isascii() or not value.isdigit() or int(value) != count:
            raise ValueError('inconsistent JUnit ' + key + ' count')
    if failed or disabled:
        raise ValueError('failed/disabled tests: ' + ', '.join(sorted(failed + disabled)))
    unexpected = set(skipped) - set(allowed_skips)
    if unexpected:
        raise ValueError('unacknowledged skips: ' + ', '.join(sorted(unexpected)))
    if not passed:
        raise ValueError('no CTest cases actually executed successfully')
    return {'registered': counts['tests'], 'passed': len(passed),
            'skipped': sorted(skipped), 'disabled': [], 'failed': [],
            'inventory_sha256': expected['sha256'],
            'junit_sha256': hashlib.sha256(data).hexdigest()}

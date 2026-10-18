#!/usr/bin/env python3
"""Verify every bundled upstream file against the checked-in SHA-256 manifest."""
import hashlib
from pathlib import Path

root = Path(__file__).resolve().parents[1]
external = root / 'external'
expected = set()
for line in (external / 'SOURCES.sha256').read_text().splitlines():
    digest, name = line.split('  ', 1)
    path = external / name
    assert path.is_file(), 'Missing vendor source: ' + name
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, 'Changed vendor source: ' + name
    expected.add(name)
actual = {str(path.relative_to(external)) for path in external.rglob('*')
          if path.is_file() and ('upstream' in path.parts or 'mini_regex' in path.parts)}
assert actual == expected, 'Vendor manifest differs: ' + str(actual ^ expected)
assert not (root / 'extern').exists(), 'Obsolete extern directory remains'
print('PASS:', len(expected), 'vendored files; unified external directory')

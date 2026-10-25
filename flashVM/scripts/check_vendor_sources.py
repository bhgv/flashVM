#!/usr/bin/env python3
"""Verify every bundled upstream file against the checked-in SHA-256 manifest."""
import hashlib
from pathlib import Path

root = Path(__file__).resolve().parents[1]
external = root / "external"
expected = set()
for line in (external / "SOURCES.sha256").read_text().splitlines():
    digest, name = line.split("  ", 1)
    path = external / name
    if not (path.is_file()):
        raise AssertionError("Missing vendor source: " + name)
    if not (hashlib.sha256(path.read_bytes()).hexdigest() == digest):
        raise AssertionError("Changed vendor source: " + name)
    expected.add(name)
actual = {
    str(path.relative_to(external))
    for path in external.rglob("*")
    if path.is_file() and ("upstream" in path.parts or "mini_regex" in path.parts)
}
if not (actual == expected):
    raise AssertionError("Vendor manifest differs: " + str(actual ^ expected))
if not (not (root / "extern").exists()):
    raise AssertionError("Obsolete extern directory remains")
print("PASS:", len(expected), "vendored files; unified external directory")

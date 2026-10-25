"""Compare debugger queries with independent tuple-based selection policies."""

import os
from pathlib import Path
import random
import subprocess
import tempfile
import unittest
from support import prefix


# Exercise executable boundaries and an independent randomized coordinate oracle.
class Queries(unittest.TestCase):
    # Execute one query against a caller-supplied map; require exact output and exit status.
    def query(self, path, args, expected, status=None):
        result = subprocess.run(
            [os.environ["SBLQUERY_TOOL"], str(path), *map(str, args)],
            capture_output=True,
            timeout=15,
        )
        if status is None:
            status = 3 if expected is None else 0
        self.assertEqual(result.returncode, status, result.stderr)
        self.assertNotIn(b"runtime error:", result.stderr)
        self.assertNotIn(b"ERROR: AddressSanitizer", result.stderr)
        self.assertEqual(result.stdout, b"" if expected is None else f"{expected}\n".encode())

    # Use unrelated start/stop files and reversed spans to catch accidental range assumptions.
    def test_random_oracle(self):
        rng = random.Random(823174)
        entries = [tuple(rng.randrange(n) for n in (3, 30, 20, 3, 30, 20, 5)) for _ in range(160)]
        records = [f"{a}:{b}.{c},{d}:{e}.{f} {g}" for a, b, c, d, e, f, g in entries]
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "map.sbl"
            path.write_bytes(prefix(records, files=("a/main.b", "b/main.b", "other.b")))
            for _ in range(80):
                file, line, column, first = (
                    rng.randrange(3),
                    rng.randrange(1, 30),
                    rng.randrange(20),
                    rng.randrange(162),
                )
                found = next(
                    (
                        i
                        for i in range(first, len(entries))
                        if entries[i][0] == entries[i][3] == file
                        and entries[i][1]
                        and entries[i][4]
                        and entries[i][1:3] <= (line, column) <= entries[i][4:6]
                    ),
                    None,
                )
                self.query(path, ["position", file, line, column, first], found)
                last = rng.randrange(line, 40)
                candidates = [
                    (e[1], i)
                    for i, e in enumerate(entries)
                    if i >= first and e[0] == file and line <= e[1] <= last
                ]
                found = min(candidates)[1] if candidates else None
                self.query(path, ["breakpoint", file, line, last, first], found)
                found = (
                    next(
                        (
                            i
                            for i in range(first + 1, len(entries))
                            if entries[i][6] != entries[first][6]
                        ),
                        None,
                    )
                    if first < len(entries)
                    else None
                )
                self.query(path, ["statement", first], found)

    # Keep malformed arguments separate from a valid but absent map query.
    def test_argument_contract(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "map.sbl"
            path.write_bytes(prefix(["1.0,1.1 0"]))
            for value in ("-1", "+1", "", "1x", "4294967296", " 1"):
                self.query(path, ["statement", value], None, 2)
            self.query(path, ["statement", "4294967295"], None)
            self.query(path, ["position", 0, 0, 0, 0], None, 1)
            self.query(path, ["breakpoint", 0, 3, 2, 0], None, 1)
            self.query(path, ["unknown"], None, 2)
            path.write_bytes(b"invalid\n")
            self.query(path, ["statement", 0], None, 1)

"""Regenerate the actual Inferno Limbo parser with the source-built iyacc."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile


# Verify generation from .y, complete output and compiler-token compatibility.
def main():
    executable, skeleton_root, grammar, expected_header = map(Path, sys.argv[1:])
    executable = executable.resolve()
    environment = os.environ.copy()
    environment["ROOT"] = str(skeleton_root.resolve())
    with tempfile.TemporaryDirectory(prefix="inferno iyacc ") as directory:
        root = Path(directory)
        (root / "limbo.y").write_bytes(grammar.read_bytes())
        subprocess.run([str(executable), "-d", "limbo.y"], cwd=root, env=environment, check=True)
        if not ((root / "y.tab.c").stat().st_size > 1000):
            raise AssertionError("condition check failed: tools/iyacc/tests/smoke.py:20")
        if not ((root / "y.tab.h").read_bytes() == expected_header.read_bytes()):
            raise AssertionError("condition check failed: tools/iyacc/tests/smoke.py:21")
        if not (not list(root.glob("y.tmp.*")) and (not list(root.glob("y.acts.*")))):
            raise AssertionError("condition check failed: tools/iyacc/tests/smoke.py:22")
    print("PASS Inferno iyacc: real limbo.y generation and exact token-header compatibility")


if __name__ == "__main__":
    main()

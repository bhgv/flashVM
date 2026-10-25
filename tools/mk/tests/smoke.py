"""Execute the actual source-built mk in private build directories."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile


# Real recipes verify ordering, pattern expansion, freshness and error propagation.
def main():
    executable = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="inferno mk ") as directory:
        root = Path(directory)
        (root / "input.c").write_text("input\n")
        (root / "mkfile").write_text(
            "all:V: input.out\n\n" "%.out: %.c\n\tcp $prereq $target\n\n" "failure:V:\n\texit 7\n"
        )
        subprocess.run([executable, "all"], cwd=root, check=True)
        output = root / "input.out"
        if not (output.read_text() == "input\n"):
            raise AssertionError("condition check failed: tools/mk/tests/smoke.py:21")
        # Upstream mk compares whole seconds and rebuilds equal timestamps.
        # Give the prerequisite an explicitly older timestamp, without sleeping.
        os.utime(root / "input.c", (1, 1))
        stamp = output.stat().st_mtime_ns
        subprocess.run([executable, "all"], cwd=root, check=True)
        if not (output.stat().st_mtime_ns == stamp):
            raise AssertionError("condition check failed: tools/mk/tests/smoke.py:27")
        failed = subprocess.run([executable, "failure"], cwd=root, capture_output=True)
        if not (failed.returncode != 0):
            raise AssertionError("condition check failed: tools/mk/tests/smoke.py:29")
    print("PASS Inferno mk: real recipes, pattern targets, freshness and failed child status")


if __name__ == "__main__":
    main()

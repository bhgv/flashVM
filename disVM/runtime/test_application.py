"""Run unchanged IDE service tests on the actual native Dis host, checking output and status."""

from pathlib import Path
import subprocess
import re
import sys


# Run one isolated application; the explicit test-control node matches existing halt clients.
# Arguments are runtime, compiled module, jit/interpreter. Fail on exceptions, FAIL text or missing PASS.
def main():
    runtime, program, mode = sys.argv[1:]
    result = subprocess.run(
        [
            str(Path(runtime).resolve()),
            "--test-control",
            "--stats",
            "--jit" if mode == "jit" else "--no-jit",
            Path(program).name,
        ],
        cwd=Path(program).parent,
        capture_output=True,
        text=True,
        timeout=50,
    )
    print(result.stdout, end="")
    print(result.stderr, end="", file=sys.stderr)
    counters = re.search(r"instructions=(\d+) native=(\d+)", result.stderr)
    if (
        not counters
        or (mode == "jit" and int(counters[2]) == 0)
        or (mode != "jit" and int(counters[2]) != 0)
    ):
        raise SystemExit("incorrect native/interpreter dispatch counters")
    if result.returncode or "FAIL" in result.stdout or "PASS" not in result.stdout:
        raise SystemExit("native Dis application failed: " + str(result.returncode))


if __name__ == "__main__":
    main()

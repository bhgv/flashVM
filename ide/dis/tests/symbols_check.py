"""Check the real emitted .sbl PC prefix through an explicitly selected C99 symbol reader."""

from pathlib import Path
import subprocess
import sys


# Keep producer and reader implementations independent; no local/type/JIT debug claim is made.
def main():
    reader, symbols = sys.argv[1:]
    result = subprocess.run([reader, symbols], capture_output=True, timeout=15)
    if not (result.returncode == 0):
        raise AssertionError(result.stderr)
    if not (b"ERROR: AddressSanitizer" not in result.stderr):
        raise AssertionError("condition check failed: ide/dis/tests/symbols_check.py:13")
    if not (b"runtime error:" not in result.stderr):
        raise AssertionError("condition check failed: ide/dis/tests/symbols_check.py:14")
    lines = result.stdout.decode().splitlines()
    header = dict(line.split("\t", 1) for line in lines[:4])
    if not (header["module"] == "Idecore"):
        raise AssertionError("condition check failed: ide/dis/tests/symbols_check.py:17")
    count = int(header["pc"])
    if not (count > 0 and int(header["files"]) > 0):
        raise AssertionError("condition check failed: ide/dis/tests/symbols_check.py:19")
    if not (0 < int(header["prefix"]) <= Path(symbols).stat().st_size):
        raise AssertionError("condition check failed: ide/dis/tests/symbols_check.py:20")
    entries = [line.split("\t") for line in lines[4:] if line.startswith("entry\t")]
    if not (len(entries) == count):
        raise AssertionError("condition check failed: ide/dis/tests/symbols_check.py:22")
    if not ([int(row[1]) for row in entries] == list(range(count))):
        raise AssertionError("condition check failed: ide/dis/tests/symbols_check.py:23")
    print(f"Idecore .sbl source prefix accepted: {count} logical PCs")


if __name__ == "__main__":
    main()

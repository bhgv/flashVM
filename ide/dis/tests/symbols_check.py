"""Check the real emitted .sbl PC prefix through an explicitly selected C99 symbol reader."""

from pathlib import Path
import subprocess
import sys


# Keep producer and reader implementations independent; no local/type/JIT debug claim is made.
def main():
    reader, symbols = sys.argv[1:]
    result = subprocess.run([reader, symbols], capture_output=True, timeout=15)
    assert result.returncode == 0, result.stderr
    assert b"ERROR: AddressSanitizer" not in result.stderr
    assert b"runtime error:" not in result.stderr
    lines = result.stdout.decode().splitlines()
    header = dict(line.split("\t", 1) for line in lines[:4])
    assert header["module"] == "Idecore"
    count = int(header["pc"])
    assert count > 0 and int(header["files"]) > 0
    assert 0 < int(header["prefix"]) <= Path(symbols).stat().st_size
    entries = [line.split("\t") for line in lines[4:] if line.startswith("entry\t")]
    assert len(entries) == count
    assert [int(row[1]) for row in entries] == list(range(count))
    print(f"Idecore .sbl source prefix accepted: {count} logical PCs")


if __name__ == "__main__":
    main()

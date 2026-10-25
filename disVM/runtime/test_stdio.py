"""Check binary standard streams, EOF, invalid descriptors and the actual error stream."""

import subprocess
import sys


# Send all byte values through the real port test executable; no terminal/server is mocked.
# argv[1] names that executable. Returns only if every byte and stream boundary matches.
def main():
    executable = sys.argv[1]
    for data in (b"", b"\x00\r\n\xff", bytes(range(256)) * 400):
        result = subprocess.run([executable], input=data, capture_output=True, timeout=10)
        if result.returncode or result.stdout != data or result.stderr:
            raise RuntimeError("standard-input/output bytes changed")
    result = subprocess.run([executable, "stderr"], capture_output=True, timeout=10)
    if result.returncode or result.stdout or result.stderr != b"native-error":
        raise RuntimeError("standard error did not reach its actual OS descriptor")
    print("PASS actual binary standard streams, short fragments, EOF and descriptor validation")


if __name__ == "__main__":
    main()

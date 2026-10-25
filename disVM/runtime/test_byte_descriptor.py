"""Run the published upstream production string/UTF descriptor regression unchanged."""

from pathlib import Path
import subprocess
import sys


# CLI supplies the pinned upstream root. Execute both WORD models under UBSan;
# no production source is rewritten and failures preserve their nonzero status.
def main():
    root = Path(sys.argv[1]).resolve()
    subprocess.run([sys.executable, str(root / "libinterp/tests/module-byte-descriptors.py")], check=True)


if __name__ == "__main__":
    main()

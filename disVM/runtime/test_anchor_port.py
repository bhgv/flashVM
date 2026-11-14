# Copyright (c) 2026 bhgv
# EN/RU: Isolated native filesystem fixture; require actual child exit and marker.
# Отдельный native filesystem fixture; требовать настоящий exit и marker.
from pathlib import Path
import subprocess
import sys
import tempfile


# EN/RU: Own temporary cwd and subprocess through complete native fixture retirement.
# Владеть temporary cwd и subprocess до полного native fixture retirement.
def main():
    binary = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="anchor port ") as work:
        result = subprocess.run([str(binary)], cwd=work, capture_output=True, text=True, timeout=45)
        output = result.stdout + result.stderr
        if result.returncode or "PASS anchor rename," not in output or "ThreadSanitizer:" in output:
            raise RuntimeError("native anchor port failed\n" + output)
        print(output.strip())


if __name__ == "__main__":
    main()

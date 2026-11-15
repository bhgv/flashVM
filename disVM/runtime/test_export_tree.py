# Copyright (c) 2026 bhgv
# EN: Own a native export fixture with real escape symlinks and independent source-byte checks.
# RU: Native export fixture с реальными escape symlinks и независимой проверкой source bytes.
from pathlib import Path
import subprocess
import sys
import tempfile


# EN/RU: Create source/private/mount trees, run the native SDK test, verify no private writes.
# Создать source/private/mount trees, выполнить native SDK test и проверить source bytes.
def main():
    binary = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="export tree ") as temporary:
        root = Path(temporary)
        (root / "data").mkdir()
        (root / "mount").mkdir()
        (root / "outer").mkdir()
        for path, value in {"data/a":"A", "data/b":"B", "data/secret":"S", "outside":"O", "outer/private":"PRIVATE"}.items():
            (root / path).write_text(value)
        (root / "data/link").symlink_to("../outside")
        (root / "data/remove-link").symlink_to("../outside")
        (root / "data/escape").symlink_to("../outer", target_is_directory=True)
        result = subprocess.run([str(binary), str(root)], capture_output=True, text=True, timeout=30)
        if result.returncode or "PASS prepared ordinary tree" not in result.stdout:
            raise RuntimeError(f"returncode={result.returncode}\n" + result.stdout + result.stderr)
        for path, value in {"data/a":"A", "data/b":"B", "data/secret":"S", "outside":"O", "outer/private":"PRIVATE"}.items():
            if (root / path).read_text() != value:
                raise RuntimeError("export changed original/private source: " + path)
        print(result.stdout.strip())


if __name__ == "__main__":
    main()

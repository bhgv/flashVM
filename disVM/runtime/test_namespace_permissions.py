# Copyright (c) 2026 bhgv
# EN: Independent chmod fixtures verify host rwx through actual guest namespace I/O.
# RU: Независимые chmod fixtures проверяют host rwx через реальные guest namespace I/O.
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
from test_owner_location import compile_image


# EN: Own an isolated fixture and compiler/runtime children until joined. Require
# non-root host identity: root permission bypass is not a guest credential test.
# RU: Владеть fixture и compiler/runtime children до join. Требовать non-root:
# root bypass не является проверкой гостевых credentials.
def main():
    if os.geteuid() == 0:
        raise RuntimeError("host rwx acceptance requires a non-root OS user")
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="namespace rwx ") as temporary:
        work = Path(temporary)
        image = work / "main.dis"
        compile_image(compiler, include, root, root / "fixtures/namespace-permissions/main.b", image)
        for name, text in (("readonly", "R"), ("nosearch", "N"), ("visible", "V")):
            (work / name).mkdir()
            (work / name / "token").write_text(text)
        (work / "readonly/token").chmod(0o444)
        (work / "nosearch").chmod(0o600)
        try:
            for mode in ("--no-jit", "--jit"):
                for locked in (False, True):
                    writable = work / "writeonly"
                    if writable.exists():
                        writable.chmod(0o600)
                    writable.write_text("ABCDE")
                    writable.chmod(0o222)
                    result = subprocess.run(
                        [str(runtime), mode, "--stats", *(["--gc-locked-poll"] if locked else []), str(image)],
                        cwd=work, capture_output=True, text=True, timeout=45,
                    )
                    output = result.stdout + result.stderr
                    if result.returncode or "PASS namespace host rwx:" not in output or "ThreadSanitizer:" in output:
                        raise RuntimeError(output[-12000:])
                    files = re.search(r"NATIVEFILES created=(\d+) closed=(\d+) live=(\d+)", output)
                    if not files or files[1] != files[2] or files[3] != "0":
                        raise RuntimeError("native file owners did not drain\n" + output[-12000:])
                    writable.chmod(0o600)
                    if writable.read_text() != "XBCZE" or (work / "missing").exists():
                        raise RuntimeError("write-only open truncated, appended, required reads or created a file")
                    if (work / "readonly/token").read_text() != "R":
                        raise RuntimeError("denied create modified readonly contents")
                    print(f"PASS namespace host rwx {mode} locked={locked}")
        finally:
            (work / "nosearch").chmod(0o700)
            (work / "readonly/token").chmod(0o600)


if __name__ == "__main__":
    main()

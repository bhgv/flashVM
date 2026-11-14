# Copyright (c) 2026 bhgv
# EN: Exercise live native registry pages from actual concurrent guest threads.
# RU: Проверить live native registry pages из actual concurrent guest threads.
from pathlib import Path
import subprocess
import sys
import tempfile

from relink_private import relink
from resident_evidence import evidence


# EN/RU: Borrow matched runtime/compiler/include; own private wrapper and actual VM until all joins.
# Borrow matched runtime/compiler/include; владеть private wrapper и actual VM до всех joins.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="live registry ") as temporary:
        work = Path(temporary)
        image = work / "main.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(image),
                str(root / "fixtures/owner-monitor/main.b"),
            ],
            check=True,
            capture_output=True,
        )
        binary = relink(runtime, work, root / "test_live_registry_probe.c", 0, ["Sys_werrstr"])
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                result = subprocess.run(
                    [
                        str(binary),
                        "--stats",
                        mode,
                        *(["--gc-locked-poll"] if locked else []),
                        str(image),
                    ],
                    capture_output=True,
                    text=True,
                    timeout=90,
                )
                output = result.stdout + result.stderr
                if (
                    result.returncode
                    or "ThreadSanitizer:" in output
                    or "runtime error:" in output
                    or "NATIVELIVE_REGISTRY observed=1 bounded=1" not in output
                ):
                    raise RuntimeError("live registry failed\n" + output[-10000:])
                evidence(output, children=8)
                print("PASS live registry " + mode + " locked=" + str(locked), flush=True)


if __name__ == "__main__":
    main()

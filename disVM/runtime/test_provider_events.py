# Copyright (c) 2026 bhgv
# EN: Verify change hints from real guest I/O reach only the installed provider.
# RU: Проверять routing hints реального guest I/O в установленный provider.
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink
from test_owner_location import compile_image


# EN/RU: Compile real guest and relinked lifecycle observer; own all through exit.
# Require callback counts, state reread and recovery evidence in every mode.
# Собрать guest и observer, владеть до exit; требовать reread/recovery evidence.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="provider events ") as temporary:
        work = Path(temporary)
        (work / "view").mkdir()
        image = work / "main.dis"
        compile_image(compiler, include, root, root / "fixtures/provider-events/main.b", image)
        binary = relink(runtime, work, root / "test_provider_events.c", 0,
                        ["dvm_namespace_boot_initialize", "dvm_close_worker_stop"])
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                result = subprocess.run([str(binary), mode, "--stats",
                                         *(["--gc-locked-poll"] if locked else []), str(image)],
                                        cwd=work, text=True, capture_output=True, timeout=45)
                text = result.stdout + result.stderr
                if result.returncode or "PASS provider event guest" not in text or \
                        "PASS provider event bridge/reread" not in text or "ThreadSanitizer:" in text:
                    raise RuntimeError(text[-18000:])
                print(f"PASS provider events {mode} locked={locked}")


if __name__ == "__main__":
    main()

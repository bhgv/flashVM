# Copyright (c) 2026 bhgv
# EN: Execute real guest Sys calls through test-injected native provider handles.
# RU: Выполнять реальные guest Sys calls через тестовые native provider handles.
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink
from test_owner_location import compile_image


# EN/RU: Own isolated files and VM children through join; require exact guest and
# consuming-close evidence. Ordinary native files must remain byte-identical.
# Владеть файлами и VM children до join; требовать guest/close evidence и
# неизменность обычных native files.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="provider FD ") as temporary:
        work = Path(temporary)
        image = work / "main.dis"
        compile_image(compiler, include, root, root / "fixtures/provider-fd/main.b", image)
        binary = relink(runtime, work, root / "test_provider_fd.c", 0,
                        ["dvm_file_slots_publish", "dvm_close_worker_stop"])
        for name in ("provider-read", "provider-write", "provider-list"):
            (work / name).write_bytes(b"HOST!")
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                result = subprocess.run([str(binary), mode, "--stats",
                                         *(["--gc-locked-poll"] if locked else []), str(image)],
                                        cwd=work, text=True, capture_output=True, timeout=45)
                output = result.stdout + result.stderr
                if result.returncode or "PASS provider FD guest" not in output or \
                        "PASS provider lifecycle created=24 closed=24 payloads=24" not in output or \
                        "ThreadSanitizer:" in output:
                    raise RuntimeError(output[-18000:])
                if any((work / name).read_bytes() != b"HOST!" for name in ("provider-read", "provider-write", "provider-list")):
                    raise RuntimeError("provider bytes escaped into native fixture")
                print(f"PASS provider FD {mode} locked={locked}")


if __name__ == "__main__":
    main()

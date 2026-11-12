# Copyright (c) 2026 bhgv
# EN: Actual root index distribution, multiset retention and old-hash omission.
# RU: Настоящее распределение индекса корней, сохранение мультимножества и контроль старого хеша.
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile


# [GC:] EN: Own native builds/children from the local runtime sources and port.
# Both publication backends test actual keys/batches. Private shifted-address
# controls must fail with the exact distribution diagnostic, never a timeout.
# RU: Владеть native-сборками/процессами локального runtime/порта. Оба способа
# публикации проверяют настоящие ключи/пакеты. Частный старый хеш обязан дать
# точную ошибку распределения, не таймаут или ошибку компиляции.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    root = Path(__file__).resolve().parents[2]
    runtime = root / "disVM/runtime"
    ports = root / "flashVM/ports"
    source = (runtime / "root_registry.c").read_text()
    with tempfile.TemporaryDirectory(prefix="native root distribution ") as folder:
        work = Path(folder)
        for backend in ("atomic", "locked"):
            for omission in (False,) if positive_only else (False, True):
                code = source
                if omission:
                    start = code.index("    uintptr_t hash = (uintptr_t)key;")
                    end = code.index("\n}", start)
                    code = (
                        code[:start]
                        + "    return (unsigned int)(((uintptr_t)key >> 6) & 31);"
                        + code[end:]
                    )
                copied = work / "root_registry.c"
                copied.write_text(code)
                binary = work / "distribution"
                command = shlex.split(os.environ.get("CC", "cc"))
                command += shlex.split(os.environ.get("CFLAGS", "-O1 -g"))
                command += [
                    "-std=c99",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    "-pthread",
                    "-I" + str(runtime),
                    "-I" + str(ports),
                ]
                if backend == "locked":
                    command += ["-DDVM_ROOT_FORCE_LOCKED=1"]
                command += [
                    str(runtime / "test_root_distribution.c"),
                    str(copied),
                    str(runtime / "root_batch.c"),
                    str(ports / "linux/thread.c"),
                    str(ports / "linux/channel_sync.c"),
                    "-o",
                    str(binary),
                ]
                built = subprocess.run(command, capture_output=True, text=True)
                if built.returncode:
                    raise RuntimeError(
                        "root index compilation failed\n" + built.stdout + built.stderr
                    )
                result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=45)
                text = result.stdout + result.stderr
                if "WARNING: ThreadSanitizer" in text or "runtime error:" in text:
                    raise RuntimeError("root index sanitizer failure\n" + text)
                marker = (
                    "strided identities must not collapse the root index"
                    if omission
                    else "PASS root distribution"
                )
                if result.returncode != (12 if omission else 0) or marker not in text:
                    raise RuntimeError("root index semantic evidence missing\n" + text)
                if not omission:
                    print(backend + ": " + text, end="")


if __name__ == "__main__":
    main()

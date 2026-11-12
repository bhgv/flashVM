# Copyright (c) 2026 bhgv
# EN: Count actual arena inspections and detect restoration of repeated head search.
# RU: Считать настоящие просмотры арены и обнаруживать возврат повторного поиска с головы.
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile


# EN: Build actual pool/port sources with observer-only private instrumentation.
# Own children until exit. Require precise bounded-work omission failures and
# reject compiler errors, signals, timeout or sanitizer reports as evidence.
# RU: Собрать настоящие pool/порт с частным наблюдением без замены алгоритма.
# Владеть процессами до выхода. Контроль требует точной ошибки бюджета поиска;
# ошибки сборки, сигналы, таймауты и санитайзер не считаются доказательством.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    root = Path(__file__).resolve().parents[2]
    runtime = root / "disVM/runtime"
    ports = root / "flashVM/ports"
    source = (runtime / "heap_pool.c").read_text()
    anchor = "    for (block = start; (uchar *)block < end; block = B2NB(block))\n    {"
    if source.count(anchor) != 1:
        raise RuntimeError("pool search observer anchor changed")
    source = "void dvm_test_pool_search_visit(void);\n" + source.replace(
        anchor, anchor + "\n        dvm_test_pool_search_visit();"
    )
    with tempfile.TemporaryDirectory(prefix="actual pool search ") as folder:
        work = Path(folder)
        for omission in (False,) if positive_only else (False, True):
            code = source
            if omission:
                anchor = "    Bhdr *start = pool->cursor;"
                if code.count(anchor) != 1:
                    raise RuntimeError("pool cursor omission anchor changed")
                code = code.replace(anchor, "    Bhdr *start = B2NB(pool->arena);")
            copied = work / "heap_pool.c"
            copied.write_text(code)
            binary = work / "search"
            includes = [
                runtime,
                root / "disVM/include",
                root / "disVM/host/include",
                root / "disVM/upstream/Linux/arm/include",
                root / "disVM/upstream/include",
                ports,
            ]
            command = shlex.split(os.environ.get("CC", "cc"))
            command += shlex.split(os.environ.get("CFLAGS", "-O1 -g"))
            command += [
                "-std=gnu99",
                "-pthread",
                "-fno-strict-aliasing",
                *["-I" + str(p) for p in includes],
            ]
            command += [
                str(runtime / "test_pool_search.c"),
                str(copied),
                str(ports / "linux/thread.c"),
                str(ports / "linux/channel_sync.c"),
                "-o",
                str(binary),
            ]
            result = subprocess.run(command, capture_output=True, text=True)
            if result.returncode:
                raise RuntimeError(
                    "pool search compilation failed\n" + result.stdout + result.stderr
                )
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=45)
            text = result.stdout + result.stderr
            marker = (
                "live prefix must not be rescanned for every allocation"
                if omission
                else "PASS pool search"
            )
            if "WARNING: ThreadSanitizer" in text or "runtime error:" in text:
                raise RuntimeError("pool search sanitizer failure\n" + text)
            if result.returncode != (12 if omission else 0) or marker not in text:
                raise RuntimeError("pool cursor semantic check failed\n" + text)
            if not omission:
                print(text, end="")


if __name__ == "__main__":
    main()

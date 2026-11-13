# Copyright (c) 2026 bhgv
# EN: Reconcile independent UTF-8 cache observations against production counters.
# RU: Сверить независимые наблюдения UTF-8-кэша со счётчиками production.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from resident_evidence import evidence
from relink_private import relink


# EN: Build real bytecode using CLI runtime/compiler/include paths. Own temporary
# files and children until exit; require exact tables, thread identity and RC drain
# across all collector/dispatch/poll profiles. Raise on any failure or timeout.
# RU: Собрать байткод с CLI-путями runtime/compiler/include. Владеть временными
# файлами и детьми до выхода; проверить таблицы, идентичность потоков и очистку RC
# во всех профилях collector/dispatch/poll. При сбое или таймауте вызвать исключение.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="resident string views ") as temporary:
        work = Path(temporary)
        image = work / "counts.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(image),
                str(root / "fixtures/resident-string-view/main.b"),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        binary = relink(
            runtime,
            work,
            root / "test_string_view.c",
            0,
            ["Sys_werrstr", "malloc", "calloc", "free", "runetochar", "dvm_close_worker_stop"],
        )
        for collector in ((), ("--gc-no-cycles",), ("--gc-idle-cycles",)):
            for mode in ("--no-jit", "--jit"):
                for locked in (False, True):
                    result = subprocess.run(
                        [
                            str(binary),
                            "--native-spawn",
                            "--stats",
                            mode,
                            *collector,
                            *(["--gc-locked-poll"] if locked else []),
                            str(image),
                        ],
                        cwd=work,
                        capture_output=True,
                        text=True,
                        timeout=60,
                    )
                    text = result.stdout + result.stderr
                    if (
                        result.returncode
                        or "PASS resident string views owners=8 shared_reads=512 cow=8 refresh=8 grow=8"
                        not in text
                    ):
                        raise RuntimeError(f"{collector} {mode} locked={locked}\n{text[-12000:]}")
                    evidence(text, children=8)
                    if "UTF8_CACHE allocated=1 freed=1 published_writes=0" not in text:
                        raise RuntimeError(
                            "shared UTF-8 cache was not uniquely owned and immutable\n"
                            + text[-12000:]
                        )
                    growth = re.search(r"UTF8_PRIVATE cache_grows=(\d+)", text)
                    if not growth or int(growth[1]) < 8:
                        raise RuntimeError(
                            "real private cache resize was not observed\n" + text[-12000:]
                        )
                    if (
                        not re.search(
                            r"NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0",
                            text,
                        )
                        or "tables_live=0 keys_live=0" not in text
                    ):
                        raise RuntimeError(
                            "string view accounting did not retire native owners/RC\n"
                            + text[-12000:]
                        )
    print(
        "PASS resident string views: 96 permanent residents, 6144 shared reads, "
        "COW/cache refresh/growth, exact native cache freeing and no published writes"
    )


if __name__ == "__main__":
    main()

# Copyright (c) 2026 bhgv
# EN: Real shared-table primitives, nested leases and opposing native transactions.
# RU: Настоящие общие операции таблиц, вложенные lease и встречные транзакции.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from relink_private import relink
from resident_evidence import evidence


# EN: Compile matching bytecode and run both dispatch/poll modes with eight residents.
# CLI supplies runtime/compiler/includes; own private images and diagnostic binary.
# Require exact payloads, guard release, fixed Prog identity and complete native GC.
# RU: Собрать байткод и проверить оба dispatch/poll режима с восемью потоками.
# CLI задаёт runtime/compiler/includes; владеть образами и диагностической VM.
# Требовать точные данные, снятие guard, постоянство Prog и очистку native GC.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    root = Path(__file__).parent
    source = root / "fixtures/resident-tables"
    with tempfile.TemporaryDirectory(prefix="resident native tables ") as temporary:
        work = Path(temporary)
        controlled = relink(
            runtime,
            work,
            root / "test_table_preflight.c",
            0,
            ["dvm_bigtable_call_ready", "fvm_table_request", "fvm_owned_try_enter"],
        )
        for profile in ("scalar", "leased", "controlled", "unwind", "splice"):
            selected = work / "selected.b"
            text = (source / ("splice.b" if profile == "splice" else "main.b")).read_text()
            if profile == "unwind":
                text = text.replace("Faults: con 0;", "Faults: con 1;")
            if profile == "leased":
                text = text.replace("Leased: con 0;", "Leased: con 1;")
            selected.write_text(text)
            image = work / "tables.dis"
            subprocess.run(
                [
                    str(compiler),
                    "-I",
                    str(include),
                    "-I",
                    str(root.parent / "module"),
                    "-o",
                    str(image),
                    str(selected),
                ],
                check=True,
                capture_output=True,
                text=True,
            )
            for mode in ("--no-jit", "--jit"):
                for locked in (False, True):
                    binary = controlled if profile == "controlled" else runtime
                    result = subprocess.run(
                        [
                            str(binary),
                            "--native-spawn",
                            mode,
                            "--stats",
                            *(["--gc-locked-poll"] if locked else []),
                            str(image),
                        ],
                        cwd=work,
                        capture_output=True,
                        text=True,
                        timeout=60,
                    )
                    output = result.stdout + result.stderr
                    marker = (
                        "PASS resident table splices transactions=1536 snapshots=1024 owners=8"
                        if profile == "splice"
                        else f'PASS resident tables leased={int(profile == "leased")} keys=2048 owners=8'
                    )
                    if result.returncode or marker not in output:
                        raise RuntimeError(f"{profile} {mode} locked={locked}\n{output[-12000:]}")
                    evidence(output, children=8)
                    if profile == "controlled" and (
                        output.count("PASS table primitive admission overlap exercised") != 1
                        or output.count("TABLE_PREFLIGHT parked_history=1") != 1
                    ):
                        raise RuntimeError(
                            "controlled preflight/native overlap did not execute\n"
                            + output[-12000:]
                        )
                    required = (
                        r"NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0",
                        r"tables_live=0 keys_live=0",
                    )
                    if any(not re.search(pattern, output) for pattern in required):
                        raise RuntimeError(
                            "native table owners or RC did not retire\n" + output[-12000:]
                        )
    print(
        "PASS resident native tables: C/JIT, both polls, scalar contention, nested leases, "
        "controlled preflight overlap, native exception cleanup, UTF-8 keys, opposing/alias splices and immutable snapshots"
    )


if __name__ == "__main__":
    main()

# Copyright (c) 2026 bhgv
# EN: Real stale child replies during native execution of a surviving parent.
# RU: Настоящие старые ответы ребёнка при native-исполнении живого родителя.
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile

from relink_private import relink
from resident_evidence import evidence

SYMBOLS = [  # Private linker observations; production implementations still execute.
    "dvm_runtime_wait_request_finished",
    "dvm_runtime_channel_new",
    "dvm_runtime_guest_sleep",
]


# EN: Restore one old foreign-state read in a private main composition. root/work
# are borrowed source/output directories; return an owned source path. Reject
# drift before compiling. Production files and objects remain unchanged.
# RU: Вернуть одно старое чтение чужого state в частной композиции main.
# root/work — каталоги исходников/результатов; вернуть путь нового исходника.
# При расхождении отказать до сборки. Production-файлы/объекты не изменяются.
def old_reader(root, work):
    control = (root / "host_runtime/process_control.inc").read_text()
    anchor = "if (!parent)"
    if control.count(anchor) != 1:
        raise RuntimeError("late reply control anchor changed")
    old = work / "old-process-control.inc"
    old.write_text(control.replace(anchor, "if (!parent || parent->state != DVM_CHILD_EXIT_WAIT)"))
    main = (root / "main.c").read_text()
    include = '#include "host_runtime/process_control.inc"'
    if main.count(include) != 1:
        raise RuntimeError("late reply main composition changed")
    source = work / "old-main.c"
    source.write_text(main.replace(include, f'#include "{old}"'))
    return source


# EN: Execute borrowed binary/image/work with explicit mode/poll and optional
# TSan omission control. Own the child through exit; return output after exact
# completion and resident/GC checks. Raise on timeout or unexpected diagnostics.
# RU: Выполнить binary/image/work в mode/poll, необязательно со старым чтением
# под TSan. Владеть процессом до выхода; вернуть вывод после точных проверок
# завершения/потоков/GC. Таймаут и неверная диагностика дают исключение.
def execute(binary, image, work, mode, locked, control=False):
    environment = dict(os.environ)
    if control:
        environment["TSAN_OPTIONS"] = (
            environment.get("TSAN_OPTIONS", "") + ":halt_on_error=0:exitcode=66"
        )
    result = subprocess.run(
        [
            str(binary),
            "--native-spawn",
            "--stats",
            mode,
            *(["--gc-locked-poll"] if locked else []),
            str(image),
        ],
        cwd=work,
        env=environment,
        capture_output=True,
        text=True,
        timeout=45,
    )
    output = result.stdout + result.stderr
    race = "WARNING: ThreadSanitizer: data race" in output
    if control:
        warnings = re.findall(r"WARNING: ThreadSanitizer: ([^\n]+)", output)
        summaries = re.findall(r"SUMMARY: ThreadSanitizer: data race ([^\n]+)", output)
        valid = (
            result.returncode == 66
            and race
            and all(warning.startswith("data race") for warning in warnings)
            and len(summaries) == len(warnings)
            and all(
                "dvm_runtime_wait_request_finished" in summary
                or "dvm_runqueue_set_state" in summary
                for summary in summaries
            )
            and "dvm_runtime_wait_request_finished" in output
            and "dvm_runqueue_set_state" in output
        )
    else:
        valid = result.returncode == 0 and "ThreadSanitizer:" not in output
    if not valid:
        raise RuntimeError(f"late reply {mode} locked={locked} control={control}\n{output}")
    evidence(output, children=2)
    for marker in (
        "PASS stale reply discarded for live native parent",
        "PASS child reply returned after parent abandoned wait",
        "PASS live parent preserved Prog and REG across 256 native pairs",
    ):
        if output.count(marker) != 1:
            raise RuntimeError("late reply protocol evidence missing\n" + output)
    return output


# EN: CLI runtime/compiler/include select matching native products. Optional
# --race-control requires a TSan build and proves the old reader races. Compile
# real Limbo, preserve temporary artifacts until all C/JIT/poll children finish.
# RU: CLI runtime/compiler/include задают согласованные native-продукты.
# --race-control требует TSan и доказывает гонку старого чтения. Собрать настоящий
# Limbo и удерживать временные файлы до завершения всех C/JIT/poll процессов.
def main():
    if len(sys.argv) not in (4, 5) or (len(sys.argv) == 5 and sys.argv[4] != "--race-control"):
        raise RuntimeError("expected runtime compiler include [--race-control]")
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    control = len(sys.argv) == 5
    if (
        control
        and "-fsanitize=thread"
        not in (runtime.parent / "CMakeFiles/disvm.dir/flags.make").read_text()
    ):
        raise RuntimeError("old-reader control requires a fully instrumented TSan build")
    with tempfile.TemporaryDirectory(prefix="live parent late reply ") as temporary:
        work = Path(temporary)
        image = work / "late-reply.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(image),
                str(root / "fixtures/resident-processctl/late-reply.b"),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        binary = relink(runtime, work, root / "test_late_reply.c", 0, SYMBOLS)
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                execute(binary, image, work, mode, locked)
        if control:
            old = relink(
                runtime,
                work,
                root / "test_late_reply.c",
                1,
                SYMBOLS,
                runtime_main_source=old_reader(root, work),
            )
            for locked in (False, True):
                execute(old, image, work, "--no-jit", locked, control=True)
    print("PASS live-parent late replies: C/JIT, both polls, fixed Prog/REG and 256 native pairs")
    if control:
        print("PASS old foreign-state reader reproduced a TSan race with both polls")


if __name__ == "__main__":
    main()

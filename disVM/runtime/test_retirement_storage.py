# Copyright (c) 2026 bhgv
# EN: Independent child-retirement storage with explicit native mailbox debt.
# RU: Независимая память завершений детей при явно заданном долге mailbox.
from pathlib import Path
import subprocess
import sys
import tempfile

from relink_private import relink
from resident_evidence import evidence


# EN: Restore shared reply storage only in a private producer composition. Borrow
# root/work, return owned source path; reject drift before compiling production code.
# RU: Вернуть общую память ответа лишь в частной композиции отправителя. root/work
# заимствованы; вернуть новый исходник, при дрейфе отказать до компиляции.
def shared_producer(root, work):
    control = (root / "host_runtime/process_control.inc").read_text()
    anchor = "            DVM_OwnerMessage *reply = message;"
    if control.count(anchor) != 1:
        raise RuntimeError("retirement storage control anchor changed")
    replacement = work / "shared-retirement.inc"
    replacement.write_text(
        control.replace(
            anchor,
            """            static DVM_OwnerMessage shared_reply;
            free(message);
            message = &shared_reply;
            DVM_OwnerMessage *reply = message;""",
        )
    )
    main = (root / "main.c").read_text()
    include = '#include "host_runtime/process_control.inc"'
    if main.count(include) != 1:
        raise RuntimeError("retirement storage composition changed")
    source = work / "shared-main.c"
    source.write_text(main.replace(include, f'#include "{replacement}"'))
    return source


# EN: Build the real Limbo fixture and private native observer using matching CLI
# runtime/compiler/include. Own subprocesses and temporary files until exit;
# require precise results, owner identity and strict GC policy in C/JIT and both
# polls. Fail on timeout, diagnostics or stale completion acceptance.
# RU: Собрать настоящий Limbo и частный наблюдатель по CLI runtime/compiler/include.
# Владеть процессами и временными файлами до выхода; проверить точные результаты,
# владельцев и строгую GC-политику в C/JIT с обоими poll. Таймаут, диагностика
# и принятие старого ответа завершают тест с ошибкой.
def main():
    if len(sys.argv) not in (4, 5) or (len(sys.argv) == 5 and sys.argv[4] != "--positive-only"):
        raise RuntimeError("expected runtime compiler include [--positive-only]")
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="retirement storage ") as temporary:
        work = Path(temporary)
        image = work / "cancel.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(image),
                str(root / "fixtures/resident-processctl/retirement-storage.b"),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        for variant in (0, 2):
            binary = relink(
                runtime,
                work,
                root / "test_retirement_storage.c",
                variant,
                [
                    "dvm_runqueue_begin_retire",
                    "dvm_runtime_prepare_killwait",
                    "dvm_runtime_channel_new",
                    "dvm_messages_pending",
                    "dvm_messages_send",
                    "free",
                    "calloc",
                ],
            )
            for mode in ("--no-jit", "--jit"):
                for locked in (False, True):
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
                        capture_output=True,
                        text=True,
                        timeout=45,
                    )
                    output = result.stdout + result.stderr
                    if result.returncode or "ThreadSanitizer:" in output:
                        raise RuntimeError(f"retirement storage {mode} locked={locked}\n{output}")
                    evidence(output, children=3)
                    if (
                        variant == 2
                        and output.count(
                            "PASS reservation refusal preserved runnable parent before retry"
                        )
                        != 1
                    ):
                        raise RuntimeError("missing reservation refusal evidence\n" + output)
                    for marker in (
                        "PASS distinct retirement replies queued behind stale mailbox debt",
                        "PASS retirement A abandoned and B acknowledged",
                        "PASS retirement storage allocations=34 freed=34 live=0",
                    ):
                        if output.count(marker) != 1:
                            raise RuntimeError("missing retirement storage evidence\n" + output)
        if len(sys.argv) == 4:
            old = relink(
                runtime,
                work,
                root / "test_retirement_storage.c",
                1,
                [
                    "dvm_runqueue_begin_retire",
                    "dvm_runtime_prepare_killwait",
                    "dvm_runtime_channel_new",
                    "dvm_messages_pending",
                    "dvm_messages_send",
                    "free",
                    "calloc",
                ],
                runtime_main_source=shared_producer(root, work),
            )
            result = subprocess.run(
                [str(old), "--native-spawn", "--no-jit", str(image)],
                cwd=work,
                capture_output=True,
                text=True,
                timeout=45,
            )
            output = result.stdout + result.stderr
            if (
                result.returncode != 12
                or "B reused A's queued retirement node" not in output
                or "ThreadSanitizer:" in output
            ):
                raise RuntimeError("shared retirement control missed aliasing\n" + output)
            print("PASS shared retirement control rejected queued node reuse before corruption")
    print(
        "PASS retirement storage: real A/B exits, 32 explicit stale replies, C/JIT and both polls"
    )


if __name__ == "__main__":
    main()

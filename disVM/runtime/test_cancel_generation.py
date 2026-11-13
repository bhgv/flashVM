# Copyright (c) 2026 bhgv
# EN: Same-child cancellation generations with real resident owners and mailboxes.
# RU: Поколения отмены одного ребёнка на настоящих потоках и mailbox.
from pathlib import Path
import subprocess
import sys
import tempfile

from relink_private import relink
from resident_evidence import evidence


# EN: Remove only operation-ID validation in a private host composition. Borrow
# root/work, return an owned source path; reject drift, never change production.
# RU: Удалить лишь проверку ID операции в частной композиции хоста. root/work
# заимствованы; вернуть новый исходник. При расхождении отказать, production не менять.
def pid_only_reader(root, work):
    owner = (root / "host_runtime/owner_requests.inc").read_text()
    anchor = " &&\n        reply->request == process->cancel_wait_request"
    if owner.count(anchor) != 1:
        raise RuntimeError("cancellation generation control anchor changed")
    replacement = work / "pid-only-owner.inc"
    replacement.write_text(owner.replace(anchor, ""))
    main = (root / "main.c").read_text()
    anchor = '#include "host_runtime/owner_requests.inc"'
    if main.count(anchor) != 1:
        raise RuntimeError("cancellation generation main composition changed")
    source = work / "pid-only-main.c"
    source.write_text(main.replace(anchor, f'#include "{replacement}"'))
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
    with tempfile.TemporaryDirectory(prefix="cancel generation ") as temporary:
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
                str(root / "fixtures/resident-processctl/cancel-generation.b"),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        binary = relink(
            runtime,
            work,
            root / "test_cancel_generation.c",
            0,
            [
                "dvm_bigtable_cancel_wait",
                "dvm_runtime_prepare_cancelwait",
                "dvm_runtime_channel_new",
                "dvm_messages_take",
                "dvm_managed_workers_finish_reply",
                "dvm_messages_send",
                "free",
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
                    raise RuntimeError(f"cancel generation {mode} locked={locked}\n{output}")
                evidence(output, children=2)
                for marker in (
                    "PASS second cancellation posted before first completion",
                    "PASS cancellation generation rejected stale A and completed B",
                    "PASS cancellation storage sent=3 freed=3 live=0",
                ):
                    if output.count(marker) != 1:
                        raise RuntimeError("missing cancellation generation evidence\n" + output)
        if len(sys.argv) == 4:
            old = relink(
                runtime,
                work,
                root / "test_cancel_generation.c",
                1,
                [
                    "dvm_bigtable_cancel_wait",
                    "dvm_runtime_prepare_cancelwait",
                    "dvm_runtime_channel_new",
                    "dvm_messages_take",
                    "dvm_managed_workers_finish_reply",
                    "dvm_messages_send",
                    "free",
                ],
                runtime_main_source=pid_only_reader(root, work),
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
                not result.returncode
                or "stale cancellation A completed new wait B" not in output
                or "ThreadSanitizer:" in output
            ):
                raise RuntimeError("PID-only control did not reject the wrong result\n" + output)
            print("PASS PID-only control accepted stale A as the result of B")
    print("PASS cancellation generations: same child, interrupted A, distinct B, C/JIT, both polls")


if __name__ == "__main__":
    main()

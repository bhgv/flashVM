# Copyright (c) 2026 bhgv
# [GC:] EN: Verify strict native fallback diagnostics on real C/JIT applications.
# RU: Проверить диагностику запрета fallback на настоящих C/JIT-приложениях.
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink


# [GC:] EN: Borrow runtime/compiler/include paths; own bytecode and private
# binaries until exit. Unknown callbacks, direct tracing/drain and both stop
# APIs must exit exactly 70 with a specific production diagnostic. Never accept
# timeout, signal, compiler or sanitizer failure as proof.
# RU: Заимствовать runtime/compiler/include; владеть байткодом и частными
# бинарниками до выхода. Неизвестный callback, общий обход/drain и оба stop API
# должны дать ровно 70 с конкретной диагностикой. Таймаут, сигнал, ошибка сборки
# или санитайзера не являются подтверждением.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="strict native guards ") as folder:
        work = Path(folder)
        image = work / "peers.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-o",
                str(image),
                str(root / "fixtures/resident-managed-retirement/main.b"),
            ],
            check=True,
            capture_output=True,
        )
        for control, reason in enumerate(
            (
                "unsupported-finalizer",
                "retained-trace-forbidden",
                "retained-drain-forbidden",
                "exclusive-stop-forbidden",
                "exclusive-stop-forbidden",
            )
        ):
            binary = relink(
                runtime, work, root / "test_concurrent_guards.c", control, ["Sys_werrstr"]
            )
            for mode in ("--no-jit", "--jit"):
                for locked in (False, True):
                    result = subprocess.run(
                        [
                            str(binary),
                            "--native-spawn",
                            mode,
                            *(["--gc-locked-poll"] if locked else []),
                            str(image),
                        ],
                        cwd=work,
                        capture_output=True,
                        text=True,
                        timeout=15,
                    )
                    output = result.stdout + result.stderr
                    if (
                        result.returncode != 70
                        or "NATIVE_CONCURRENCY_ERROR" not in output
                        or f"reason={reason}" not in output
                        or "Sanitizer" in output
                        or "runtime error:" in output
                    ):
                        raise RuntimeError(
                            f"control={control} {mode} locked={locked} exit={result.returncode}\n{output[-8000:]}"
                        )
    print("PASS strict native guards: 20 exact diagnostic failures, no legacy access")


if __name__ == "__main__":
    main()

# Copyright (c) 2026 bhgv
# EN: Verify mandatory native defaults and reject removed execution modes.
# RU: Проверить обязательный native-профиль и отказ удалённым режимам исполнения.
from pathlib import Path
import subprocess
import sys
import tempfile

from resident_evidence import evidence


# EN: Borrow matching runtime/compiler/include paths; own bytecode and processes
# through exit. Default and diagnostic launches must retain dedicated owners and
# strict GC. Removed modes must fail before VM startup. Raise on any discrepancy.
# RU: Заимствовать согласованные runtime/compiler/include; владеть байткодом и
# процессами до выхода. Штатный запуск и диагностика сохраняют владельцев и строгий
# GC. Удалённые режимы должны отклоняться до VM. Расхождения дают исключение.
def main():
    runtime, compiler, include = (Path(arg).resolve() for arg in sys.argv[1:])
    root = Path(__file__).resolve().parent
    for removed in (
        ("--managed-workers", "1"),
        ("--managed-workers", "2"),
        ("--managed-workers", "auto"),
        ("--managed-workers-limit", "4"),
        ("--gc-inline",),
        ("--native-spawn", "--gc-inline"),
        ("--sequential-ui",),
    ):
        result = subprocess.run(
            [str(runtime), *removed, "--help"], capture_output=True, text=True, timeout=10
        )
        output = result.stdout + result.stderr
        if (
            result.returncode != 2
            or "native concurrency is mandatory" not in output
            or "NATIVEGUEST" in output
            or "ThreadSanitizer:" in output
        ):
            raise RuntimeError(f"removed profile {removed}\n{output}")
    with tempfile.TemporaryDirectory(prefix="native defaults ") as temporary:
        work = Path(temporary)
        image = work / "default.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-o",
                str(image),
                str(root / "fixtures/resident-processctl/native-default.b"),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        for mode in ("--jit", "--no-jit"):
            for options in (
                (),
                ("--gc-worker",),
                ("--guarded-copies",),
                ("--gc-locked-poll",),
                ("--native-spawn",),
            ):
                result = subprocess.run(
                    [str(runtime), mode, "--stats", *options, str(image)],
                    cwd=work,
                    capture_output=True,
                    text=True,
                    timeout=45,
                )
                output = result.stdout + result.stderr
                if (
                    result.returncode
                    or "PASS mandatory native default owners=4" not in output
                    or "ThreadSanitizer:" in output
                ):
                    raise RuntimeError(f"native default {mode} {options}\n{output}")
                evidence(output, children=4)
    print(
        "PASS mandatory native defaults: C/JIT, diagnostic options, four owners, seven removed profiles"
    )


if __name__ == "__main__":
    main()

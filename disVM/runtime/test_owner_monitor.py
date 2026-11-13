# Copyright (c) 2026 bhgv
# [GC:] EN: Verify real callback isolation and concurrent Fast under C/JIT.
# RU: Проверить изоляцию настоящих callbacks и параллельный Fast в C/JIT.
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink
from resident_evidence import evidence


# EN: CLI borrows VM/compiler/include; own bytecode, probe and children to exit.
# Test four positive native C/JIT/poll profiles and a real shared-selector control.
# --positive-only omits intentional failures. Compiler/sanitizer/timeouts fail.
# RU: CLI заимствует VM/компилятор/include; владеть файлами/процессами до выхода.
# Проверить четыре native C/JIT/poll профиля и контроль общего selector.
# --positive-only исключает намеренные отказы. Ошибка сборки/санитайзера/таймаут — отказ.
def main():
    positive = "--positive-only" in sys.argv[1:]
    paths = [value for value in sys.argv[1:] if value != "--positive-only"]
    runtime, compiler, include = (Path(value).resolve() for value in paths)
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="owner local monitors ") as directory:
        work = Path(directory)
        image = work / "monitors.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-o",
                str(image),
                str(root / "fixtures/owner-monitor/main.b"),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        for control in (0,) if positive else (0, 1):
            binary = relink(
                runtime, work, root / "test_owner_monitor.c", control, ["Sys_werrstr", "poolfree"]
            )
            for mode in ("--no-jit", "--jit"):
                for locked in (False, True):
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
                    if "Sanitizer" in output or "runtime error:" in output:
                        raise RuntimeError("observer sanitizer failure\n" + output[-12000:])
                    if control:
                        if (
                            result.returncode != 12
                            or "FAIL owner monitor: foreign callback selected" not in output
                        ):
                            raise RuntimeError(
                                "shared-selector control did not fail precisely\n" + output[-12000:]
                            )
                    else:
                        if (
                            result.returncode
                            or "PASS owner-local heap monitors owners=8" not in output
                            or output.count(
                                "OWNER_MONITOR parent=1 residents=8 physical_frees=8 isolated=1"
                            )
                            != 1
                        ):
                            raise RuntimeError(f"{mode} locked={locked}\n{output[-12000:]}")
                        evidence(output, children=8)
    print(
        "PASS owner-local heap monitors: callback install/clear/restore, noninheritance, "
        "eight real Fast frees with hooks installed, C/JIT and both polls"
        + ("; positive only" if positive else "; shared-selector omission controls")
    )


if __name__ == "__main__":
    main()

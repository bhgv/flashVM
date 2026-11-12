# Copyright (c) 2026 bhgv
# [GC:] EN: Verify strict native fallback diagnostics on real C/JIT applications.
# RU: Проверить диагностику запрета fallback на настоящих C/JIT-приложениях.
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink
from rc_test_source import read_rc_source
from resident_evidence import evidence


# [GC:] EN: Borrow runtime/compiler/include paths; own bytecode and private
# binaries until exit. Unknown callbacks, direct tracing/drain and both stop
# APIs must exit exactly 70 with a specific production diagnostic. Never accept
# timeout, signal, compiler or sanitizer failure as proof. --positive-only
# selects normal release/drain profiles without intentional termination.
# RU: Заимствовать runtime/compiler/include; владеть байткодом и частными
# бинарниками до выхода. Неизвестный callback, общий обход/drain и оба stop API
# должны дать ровно 70 с конкретной диагностикой. Таймаут, сигнал, ошибка сборки
# или санитайзера не являются подтверждением. --positive-only оставляет
# штатные release/drain-профили без намеренного завершения.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    arguments = [value for value in sys.argv[1:] if value != "--positive-only"]
    runtime, compiler, include = (Path(value).resolve() for value in arguments)
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
        closed_image = work / "single.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-o",
                str(closed_image),
                str(root / "fixtures/concurrent-retirement/main.b"),
            ],
            check=True,
            capture_output=True,
        )
        reasons = (
            "unsupported-finalizer",
            "retained-trace-forbidden",
            "retained-drain-forbidden",
            "exclusive-stop-forbidden",
            "exclusive-stop-forbidden",
            "retirement-not-admitted",
            "retirement-admission-closed",
            "retirement-not-admitted",
            "retirement-not-admitted",
            None,
            None,
        )
        controls = (9, 10) if positive_only else (*range(11), 100, 101, 102, 103, 104, 105)
        for control in controls:
            reason = reasons[control] if control < 100 else None
            rc = coordinator = None
            if control in (100, 101, 102, 103):
                source = read_rc_source(root)
                anchor = "        concurrent_retirement_refused(admitted_role);"
                if source.count(anchor) != 1:
                    raise RuntimeError("strict retirement rejection anchor changed")
                rc = work / "runtime_rc.c"
                rc.write_text(source.replace(anchor, "        (void)admitted_role;"))
            if control in (104, 105):
                source = (root / "gc/common/concurrent_runtime.c").read_text()
                begin = source.index(
                    "    if (!dvm_gc_runtime.world || context != dvm_gc_runtime.owner ||"
                )
                end = source.index("    int active =", begin)
                source = (
                    source[:begin]
                    + "    if (!dvm_native_owner_statistics(owner, &state))\n    {\n        return 0;\n    }\n"
                    + source[end:]
                )
                source = (
                    source.replace('"runtime_state.h"', '"gc/common/runtime_state.h"')
                    .replace('"../collector_passes.h"', '"gc/collector_passes.h"')
                    .replace('"../../', '"')
                )
                coordinator = work / "concurrent_runtime.c"
                coordinator.write_text(source)
            binary = relink(
                runtime,
                work,
                root / "test_concurrent_guards.c",
                control,
                [
                    "Sys_werrstr",
                    "_Exit",
                    "poolfree",
                    "dvm_runtime_guest_sleep",
                    "dvm_gc_workers_request",
                    "dvm_runtime_gc_stop",
                ],
                runtime_rc_source=rc,
                concurrent_runtime_source=coordinator,
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
                            str(closed_image if control in (6, 101) else image),
                        ],
                        cwd=work,
                        capture_output=True,
                        text=True,
                        timeout=15,
                    )
                    output = result.stdout + result.stderr
                    if "Sanitizer" in output or "runtime error:" in output:
                        raise RuntimeError("strict retirement sanitizer failure\n" + output[-8000:])
                    if control >= 100:
                        marker = (
                            "invalid drain must not queue collector work"
                            if control >= 104
                            else "FAIL strict guard: unknown callback executed"
                        )
                        valid = result.returncode == 12 and marker in output
                    elif control == 10:
                        valid = (
                            result.returncode == 0
                            and "PASS strict drain rejection shared=1 requests=0 phase=preserved"
                            in output
                        )
                        if valid:
                            evidence(output, children=2)
                    elif control == 9:
                        valid = result.returncode == 0 and "PASS strict release roundtrip" in output
                        if valid:
                            evidence(output, children=2)
                    else:
                        valid = (
                            result.returncode == 70
                            and "NATIVE_CONCURRENCY_ERROR" in output
                            and f"reason={reason}" in output
                        )
                        if control == 0 or control >= 5:
                            valid = (
                                valid and "GUARD_OWNERS_PRESERVED ref=1 roots=1 type=2" in output
                            )
                    if not valid:
                        raise RuntimeError(
                            f"control={control} {mode} locked={locked} exit={result.returncode}\n{output[-8000:]}"
                        )
    if positive_only:
        print("PASS strict native positive guards: eight shared release/drain profiles")
        return
    print(
        "PASS strict native guards: 36 guarded refusals, four shared roundtrips, four drain phase checks, 24 actual omission controls"
    )


if __name__ == "__main__":
    main()

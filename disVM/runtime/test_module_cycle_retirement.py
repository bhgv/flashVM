# Copyright (c) 2026 bhgv
# EN: Real module cycle retirement under permanent native peers, plus omission controls.
# RU: Освобождение настоящего цикла модуля при нативных соседях и контроли пропусков.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from rc_test_source import read_rc_source
from relink_private import relink
from resident_evidence import evidence


# EN: Own private relinks and real apps through exit. CLI supplies matching
# runtime/compiler/include paths; --positive-only excludes intentional violations.
# Require real pool/metadata frees, final epoch wait, balanced queues and actual
# stale-field rejection. Controls require exact semantic status, never timeout,
# compiler failure, signal or sanitizer warning.
# RU: Владеть частными бинарниками и приложениями до выхода. CLI задаёт пути
# runtime/compiler/include; --positive-only исключает намеренные нарушения.
# Требовать настоящие free, ожидание эпохи, баланс очередей и отказ устаревшим
# полям. Контроли требуют точного семантического кода, не таймаута/ошибки сборки.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    runtime, compiler, include = (
        Path(value).resolve() for value in sys.argv[1:] if value != "--positive-only"
    )
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="module cycle retirement ") as temporary:
        work = Path(temporary)
        image = work / "module-cycle.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(image),
                str(root / "fixtures/resident-packet-cycle/main.b"),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        for control in range(6 if positive_only else 10):
            replacement = None
            if control >= 7:
                source = read_rc_source(root)
                gate = "H2D(Modlink *, heap)->MP = H;\n                dvm_gc_fast_module_finalize(heap);"
                gates = {
                    7: (gate, "((void)0);\n                dvm_gc_fast_module_finalize(heap);"),
                    8: (gate, "H2D(Modlink *, heap)->MP = H;\n                ((void)0);"),
                    9: ("if (final_epoch)", "if (0)"),
                }
                gate, changed = gates[control]
                if source.count(gate) != 1:
                    raise RuntimeError("module cycle control anchor changed: " + str(control))
                replacement = work / "runtime_rc.c"
                replacement.write_text(source.replace(gate, changed))
            binary = relink(
                runtime,
                work,
                Path(__file__).with_suffix(".c"),
                control,
                [
                    "Sys_werrstr",
                    "dvm_runtime_guest_sleep",
                    "dvm_runtime_rc_defer",
                    "dvm_runtime_root_remove",
                    "poolfree",
                    "free",
                    "dvm_gc_fast_module_finalize",
                    "dvm_gc_fast_module_eligible",
                    "dvm_runtime_rc_shared_leaves",
                    "dvm_gc_fast_confirm_graph",
                ],
                runtime_rc_source=replacement,
            )
            profiles = (
                [("--no-jit", False)]
                if control >= 6
                else [(mode, locked) for mode in ("--no-jit", "--jit") for locked in (False, True)]
            )
            for mode, locked in profiles:
                result = subprocess.run(
                    [
                        str(binary),
                        "--native-spawn",
                        "--gc-idle-cycles",
                        "--stats",
                        mode,
                        *(["--gc-locked-poll"] if locked else []),
                        str(image),
                    ],
                    cwd=work,
                    capture_output=True,
                    text=True,
                    timeout=25,
                )
                text = result.stdout + result.stderr
                if "WARNING: ThreadSanitizer:" in text or "runtime error:" in text:
                    raise RuntimeError("module cycle sanitizer failure\n" + text[-12000:])
                if control >= 6:
                    marker = {
                        6: "module cycle still requires heap quiescence",
                        7: "claimed module MP edge must be detached once",
                        8: "module cycle must run native Module finalization",
                        9: "module finalizer waits for claim and reader epoch",
                    }[control]
                    if result.returncode != 12 or marker not in text:
                        raise RuntimeError(
                            f"module cycle control {control} missed\n{text[-12000:]}"
                        )
                    continue
                nodes = 2300 if control == 4 else 4 if control == 3 else 2
                if result.returncode or f"PASS module cycle nodes={nodes} " not in text:
                    raise RuntimeError(f"control={control} {mode} locked={locked}\n{text[-12000:]}")
                evidence(text, children=2)
                if not re.search(r"NATIVERC enqueued=(\d+) released=\1 pending=0", text):
                    raise RuntimeError("module cycle queue accounting failed\n" + text[-12000:])
                if control == 2 and "final_epoch=1 live=1" not in text:
                    raise RuntimeError("external MP survival not observed\n" + text[-12000:])
                if control == 5 and "stale=1" not in text:
                    raise RuntimeError(
                        "actual changed-field refusal not observed\n" + text[-12000:]
                    )
                if control == 4:
                    duke = re.search(
                        r"NATIVEDUKEGC promoted=(\d+) peak_fields=(\d+) resumed=(\d+) adopted_pins=(\d+)",
                        text,
                    )
                    if (
                        not duke
                        or int(duke[1]) < 1
                        or int(duke[2]) < nodes
                        or int(duke[3]) < 1
                        or int(duke[4]) < nodes - 1
                    ):
                        raise RuntimeError(
                            "module cycle did not resume Duke capture\n" + text[-12000:]
                        )
    print(
        "PASS actual module cycle retirement: physical graph/Module frees, external owners, two instances, "
        "final reader epochs, changed-field replay, 2300-node Duke; C/JIT, both polls"
        + ("; positive profiles only" if positive_only else "; four semantic omission controls")
    )


if __name__ == "__main__":
    main()

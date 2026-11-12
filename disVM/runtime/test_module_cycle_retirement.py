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
        positive = (*range(6), *range(10, 19), *range(20, 28), 31, 32, 33, 34)
        for control in positive if positive_only else (*positive, *range(6, 10), 19, 28, 29, 30):
            replacement = None
            module_graph = None
            if control in (29, 30):
                source = (root / "gc/common/module_graph.c").read_text()
                function = (
                    "void dvm_rc_mark_module_live("
                    if control == 29
                    else "int dvm_rc_keep_record_edge("
                )
                start = source.index(function)
                body = source.index("{", start) + 1
                source = (
                    source[:body]
                    + ("\n    return;" if control == 29 else "\n    return 1;")
                    + source[body:]
                )
                source = source.replace(
                    '#include "graph_snapshot.h"',
                    '#include "' + str(root / "gc/common/graph_snapshot.h") + '"',
                )
                module_graph = work / "module_graph.c"
                module_graph.write_text(source)
            if control in (7, 8, 9, 19):
                source = read_rc_source(root)
                gate = "H2D(Modlink *, heap)->MP = H;"
                gates = {
                    7: (gate, "((void)0);"),
                    8: (
                        "dvm_gc_fast_module_graph_finalize(heap, snapshot->native_modules[index]);",
                        "((void)0);",
                    ),
                    9: ("if (final_epoch)", "if (0)"),
                    19: (
                        "if (became_ready && notify_ready)",
                        "if (exclusive && became_ready && notify_ready)",
                    ),
                }
                gate, changed = gates[control]
                prefix = suffix = ""
                if control == 19:
                    first = source.index("static int defer_leaf(")
                    end = source.index("static void unlink_record_group", first)
                    prefix, source, suffix = source[:first], source[first:end], source[end:]
                if source.count(gate) != 1:
                    raise RuntimeError("module cycle control anchor changed: " + str(control))
                replacement = work / "runtime_rc.c"
                replacement.write_text(prefix + source.replace(gate, changed) + suffix)
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
                    "dvm_runtime_root_add",
                    "dvm_gc_workers_request",
                    "dvm_runtime_rc_shared_candidates",
                    "poolfree",
                    "free",
                    "dvm_gc_fast_module_finalize",
                    "dvm_gc_fast_module_graph_finalize",
                    "dvm_gc_fast_module_eligible",
                    "dvm_runtime_rc_shared_leaves",
                    "dvm_gc_fast_confirm_graph",
                    "dvm_refcount_compare_i",
                    "dvm_world_retire_epoch",
                    "dvm_module_registry_claim_counts",
                ],
                runtime_rc_source=replacement,
                module_graph_source=module_graph,
            )
            profiles = (
                [("--no-jit", False)]
                if control not in positive
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
                if control not in positive:
                    marker = {
                        6: "module cycle still requires heap quiescence",
                        7: "claimed module MP edge must be detached once",
                        8: "module cycle must run native Module finalization",
                        9: "module finalizer waits for claim and reader epoch",
                        19: "Cycles final-owner publication must notify idle Fast",
                        28: "module cycle still requires heap quiescence",
                        29: "live member and original MP cannot be reclaimed with dead subset",
                        30: "live member and original MP cannot be reclaimed with dead subset",
                    }[control]
                    if result.returncode != 12 or marker not in text:
                        raise RuntimeError(
                            f"module cycle control {control} missed\n{text[-12000:]}"
                        )
                    continue
                nodes = (
                    6
                    if control in (31, 32, 33)
                    else (
                        2300
                        if control in (4, 17, 25)
                        else 4 if control in (3, 16, 20, 21, 22, 23, 24, 26, 27, 34) else 2
                    )
                )
                if result.returncode or f"PASS module cycle nodes={nodes} " not in text:
                    raise RuntimeError(f"control={control} {mode} locked={locked}\n{text[-12000:]}")
                evidence(text, children=2)
                if not re.search(r"NATIVERC enqueued=(\d+) released=\1 pending=0", text):
                    raise RuntimeError("module cycle queue accounting failed\n" + text[-12000:])
                if control == 2 and "final_epoch=1 live=1" not in text:
                    raise RuntimeError("external MP survival not observed\n" + text[-12000:])
                if control in (14, 16, 21, 32) and "native_refusal=1" not in text:
                    raise RuntimeError("native claim refusal not observed\n" + text[-12000:])
                if control in (15, 22) and "epoch_refusal=1" not in text:
                    raise RuntimeError("epoch rollback not observed\n" + text[-12000:])
                if control in (13, 24) and "lookup_refusal=1" not in text:
                    raise RuntimeError("lookup race not observed\n" + text[-12000:])
                if control in (14, 15, 16, 21, 22, 32) and "rollback=1" not in text:
                    raise RuntimeError("full rollback not observed\n" + text[-12000:])
                if control in (12, 23, 26) and "live=1" not in text:
                    raise RuntimeError(
                        "external Module owner survival not observed\n" + text[-12000:]
                    )
                if control in (27, 33) and "member=1" not in text:
                    raise RuntimeError("live member preservation not observed\n" + text[-12000:])
                if control == 5 and "stale=1" not in text:
                    raise RuntimeError(
                        "actual changed-field refusal not observed\n" + text[-12000:]
                    )
                if control in (4, 17, 25):
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
        "aggregated 1/2/3 native owners, original-MP cycles, lookup races, partial native/epoch rollback, "
        "final reader epochs, "
        "changed-field replay, 2300-node Duke; C/JIT, both polls"
        + ("; positive profiles only" if positive_only else "; eight semantic omission controls")
    )


if __name__ == "__main__":
    main()

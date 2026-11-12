# Copyright (c) 2026 bhgv
# EN: Actual Duke abandonment, detached-cycle seed rescue and bounded FIFO refusal.
# RU: Настоящая отмена Duke, сохранение отделённого цикла и повтор отказа FIFO.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from rc_test_source import read_rc_source
from relink_private import relink
from resident_evidence import evidence


# EN: Resolve only existing local quoted includes for a private source copy.
# source/path are borrowed text and its production location. Return owned text;
# preserve system/build-provided include names and never change production files.
# RU: Разрешить существующие локальные include частной копии source по path.
# Вернуть личную строку; системные/сборочные include и исходники не меняются.
def private_includes(source, path):
    return re.sub(
        r'^#include "([^"\n]+)"$',
        lambda match: (
            '#include "' + str((path.parent / match[1]).resolve()) + '"'
            if (path.parent / match[1]).is_file()
            else match[0]
        ),
        source,
        flags=re.M,
    )


# EN: Own real C/JIT apps/private relinks using CLI runtime/compiler/include paths.
# Require a genuine old-edge seed to drain before Duke abandons its pins. The
# detached cycle frees before outside source release, then all real allocations
# retire with admitted peers. Controls require exact semantic failures, never
# build errors, signal, process timeout or sanitizer warning. --positive-only
# omits intentional violations.
# RU: Владеть C/JIT приложениями и частными бинарниками по CLI путям. Требовать
# снятия настоящего seed старого ребра до отмены pin Duke. Отдельный цикл
# освобождается до снятия внешнего source, затем все объекты при допущенных
# соседях. Контроли требуют точной семантической ошибки, не таймаута/санитайзера.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    runtime, compiler, include = (
        Path(value).resolve() for value in sys.argv[1:] if value != "--positive-only"
    )
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="Duke pin retirement ") as temporary:
        work = Path(temporary)
        image = work / "duke-pin.dis"
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
        for control in range(2 if positive_only else 6):
            rc = capture = None
            if control in (2, 3, 5):
                source = read_rc_source(root)
                gates = {
                    2: (
                        "if (!stopped && accepting && record_candidate(heap) && dvm_refcount_load_u(&heap->ref) > 1)",
                        "if (0)",
                    ),
                    3: (
                        "static int preserve_duke_pin(Heap *heap)\n{",
                        "static int preserve_duke_pin(Heap *heap)\n{\n    if (!dvm_runtime_root_remove(heap)) { abort(); }",
                    ),
                    5: (
                        "if (!preserve_duke_pin(heap))\n            {\n                return 0;",
                        "if (!preserve_duke_pin(heap))\n            {\n                duke_abandoned.pinned_nodes--;\n                return 0;",
                    ),
                }
                gate, changed = gates[control]
                if source.count(gate) != 1:
                    raise RuntimeError("Duke pin control anchor changed: " + str(control))
                rc = work / "runtime_rc.c"
                rc.write_text(source.replace(gate, changed))
            if control == 4:
                path = root / "gc/common/graph_capture.c"
                source = path.read_text()
                gate = "if (value != (target == UINT_MAX ? H : group->nodes[target]))"
                if source.count(gate) != 1:
                    raise RuntimeError("Duke guarded-field control anchor changed")
                capture = work / "graph_capture.c"
                capture.write_text(private_includes(source.replace(gate, "if (0)"), path))
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
                    "dvm_range_access_end",
                    "dvm_rc_duke_capture_slice",
                    "dvm_retirement_reserve",
                    "dvm_retirement_commit_ready",
                    "dvm_gc_duke_run",
                    "poolfree",
                    "dvm_world_try_quiescent",
                ],
                runtime_rc_source=rc,
                graph_capture_source=capture,
            )
            profiles = (
                [("--no-jit", False)]
                if control >= 2
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
                    raise RuntimeError("Duke pin sanitizer failure\n" + text[-12000:])
                if control >= 2:
                    marker = {
                        2: "detached cycle lost its Duke candidate pin",
                        3: "Duke pin root disappeared before mandatory commit",
                        4: "detached cycle lost its Duke candidate pin",
                        5: "refused Duke pin advanced journal cursor",
                    }[control]
                    if result.returncode != 12 or marker not in text:
                        raise RuntimeError(f"Duke pin control {control} missed\n{text[-12000:]}")
                    continue
                if (
                    result.returncode
                    or "PASS Duke pin retirement detached=64 targets=2366" not in text
                ):
                    raise RuntimeError(f"control={control} {mode} locked={locked}\n{text[-12000:]}")
                evidence(text, children=2)
                pins = re.search(r"NATIVEDUKEPINS seeds=(\d+) retries=(\d+)", text)
                if not pins or int(pins[1]) < 65 or (control == 1 and int(pins[2]) < 8):
                    raise RuntimeError("real Duke pin handoffs/retries missing\n" + text[-12000:])
                if not re.search(r"NATIVERC enqueued=(\d+) released=\1 pending=0", text):
                    raise RuntimeError("Duke pin accepted accounting failed\n" + text[-12000:])
    print(
        "PASS actual Duke pin retirement: real detached-cycle mutation, old seed consumption, "
        "pin rescue, FIFO retry, physical source/array/string frees; C/JIT, both polls"
        + ("; positive profiles only" if positive_only else "; four semantic omission controls")
    )


if __name__ == "__main__":
    main()

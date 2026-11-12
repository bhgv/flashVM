# Copyright (c) 2026 bhgv
# EN: Actual sparse/boundary/native-vector graph claims and allocation retries.
# RU: Настоящие захваты графов с редкими/граничными native-векторами и повторами.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from relink_private import relink
from resident_evidence import evidence
from test_duke_pin_retirement import private_includes


# EN: Own real VM applications/private confirmation relinks from CLI build paths.
# Positive cases use actual Modules and managed graph owners; omission controls
# require exact semantic failures, never build errors, signals or timeouts.
# --positive-only omits intentional violations for sanitizer repetitions.
# RU: Владеть VM-приложениями/частными подтверждениями по CLI путям сборки.
# Сценарии используют настоящие Module и владельцев графа; контроли требуют
# точных семантических ошибок, не ошибок сборки, сигналов или таймаутов.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    runtime, compiler, include = (
        Path(value).resolve() for value in sys.argv[1:] if value != "--positive-only"
    )
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="Fast claim storage ") as temporary:
        work = Path(temporary)
        image = work / "claim-storage.dis"
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
        for control in range(5 if positive_only else 9):
            confirmation = None
            if control >= 5:
                helper = root / "gc/fast/module_graph_claim.inc"
                text = helper.read_text()
                gates = {
                    5: (
                        "if (count > RECORD_NODE_CAPACITY)",
                        "if (group->nodes_used > RECORD_NODE_CAPACITY)",
                    ),
                    6: ("return DVM_CELL_BUSY;", "return DVM_CELL_REFUSED;"),
                    7: (
                        "count * (sizeof(*modules) + sizeof(*owners))",
                        "group->nodes_used * (sizeof(*modules) + sizeof(*owners))",
                    ),
                    8: ("        free(modules);", ""),
                }
                gate, changed = gates[control]
                if text.count(gate) != 1:
                    raise RuntimeError(
                        "native claim storage omission anchor changed: " + str(control)
                    )
                copied = work / "module_graph_claim.inc"
                copied.write_text(text.replace(gate, changed))
                path = root / "gc/fast/graph_confirmation.c"
                source = path.read_text().replace(
                    '#include "module_graph_claim.inc"', '#include "' + str(copied) + '"'
                )
                confirmation = work / "graph_confirmation.c"
                confirmation.write_text(private_includes(source, path))
            binary = relink(
                runtime,
                work,
                Path(__file__).with_suffix(".c"),
                control,
                [
                    "Sys_werrstr",
                    "dvm_runtime_guest_sleep",
                    "malloc",
                    "free",
                    "poolfree",
                    "dvm_cell_reservation_claim",
                    "dvm_gc_fast_confirm_graph",
                    "dvm_runtime_rc_shared_leaves",
                ],
                graph_confirmation_source=confirmation,
            )
            profiles = (
                [("--no-jit", False)]
                if control >= 5
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
                    raise RuntimeError("native claim storage sanitizer failure\n" + text[-12000:])
                if control >= 5:
                    marker = {
                        5: "claim vector allocation requires native owners",
                        6: "native vector allocation must retry original snapshot",
                        7: "native claim vector matches native owner count",
                        8: "native claim vector must be freed before confirmation returns",
                    }[control]
                    if result.returncode != 12 or marker not in text:
                        raise RuntimeError(
                            "native vector omission missed " + str(control) + "\n" + text[-12000:]
                        )
                    continue
                count = (0, 1, 513, 514, 520)[control]
                marker = f"PASS Fast claim storage nodes=1040 modules={count}"
                if result.returncode or marker not in text:
                    raise RuntimeError(f"control={control} {mode} locked={locked}\n{text[-12000:]}")
                evidence(text, children=2)
                if not re.search(r"NATIVERC enqueued=(\d+) released=\1 pending=0", text):
                    raise RuntimeError("native vector queue accounting failed\n" + text[-12000:])
    print(
        "PASS Fast claim storage: actual 0/1/513/514/520 Modules, native cardinality, "
        "eight allocation retries, same snapshot, physical frees and final epochs; C/JIT, both polls"
        + ("; positive profiles only" if positive_only else "; four semantic omission controls")
    )


if __name__ == "__main__":
    main()

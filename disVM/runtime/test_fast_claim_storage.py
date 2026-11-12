# Copyright (c) 2026 bhgv
# EN: Actual sparse/boundary/native-vector graph claims and allocation retries.
# RU: Настоящие захваты графов с редкими/граничными native-векторами и повторами.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from relink_private import relink
from rc_test_source import read_rc_source
from resident_evidence import evidence
from test_duke_pin_retirement import private_includes


# EN: Own real VM applications/private confirmation relinks from CLI build paths.
# Positive cases use actual Modules and managed graph owners; omission controls
# require exact semantic failures, never build errors, signals or timeouts.
# --positive-only omits intentional violations; --mixed-only selects retained
# backlog with vector BUSY, FULL/BYTES replay and quiescent resource recovery.
# --large-replay retains the separate 1040-node two-slot watchdog stress probe.
# RU: Владеть VM-приложениями/частными подтверждениями по CLI путям сборки.
# Сценарии используют настоящие Module и владельцев графа; контроли требуют
# точных семантических ошибок, не ошибок сборки, сигналов или таймаутов.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    mixed_only = "--mixed-only" in sys.argv[1:]
    large_replay = "--large-replay" in sys.argv[1:]
    runtime, compiler, include = (
        Path(value).resolve()
        for value in sys.argv[1:]
        if value not in ("--positive-only", "--mixed-only", "--large-replay")
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
        positive = (
            (17, 18)
            if large_replay
            else (9, 11, 13, 15) if mixed_only else (0, 1, 2, 3, 4, 9, 11, 13, 15)
        )
        controls = (
            positive
            if positive_only or mixed_only or large_replay
            else (*positive, 5, 6, 7, 8, 10, 12, 14, 16)
        )
        for control in controls:
            confirmation = rc = None
            if control in (10, 12, 14, 16):
                text = read_rc_source(root)
                gate = "retry |= snapshot_retry && dvm_world_has_active_guest(world);"
                if text.count(gate) != 1:
                    raise RuntimeError("snapshot retry propagation anchor changed")
                rc = work / "runtime_rc.c"
                rc.write_text(
                    text.replace(
                        gate,
                        "retry |= snapshot_retry;" if control == 16 else "(void)snapshot_retry;",
                    )
                )
            if control in (5, 6, 7, 8):
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
                    "dvm_retirement_reserve",
                    "dvm_retirement_commit",
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
                runtime_rc_source=rc,
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
                    raise RuntimeError("native claim storage sanitizer failure\n" + text[-12000:])
                if control not in positive:
                    marker = {
                        5: "claim vector allocation requires native owners",
                        6: "native vector allocation must retry original snapshot",
                        7: "native claim vector matches native owner count",
                        8: "native claim vector must be freed before confirmation returns",
                        10: "shared graph retry survives retained backlog",
                        12: "snapshot replay retry survives retained backlog",
                        14: "snapshot replay retry survives retained backlog",
                        16: "quiescent retained work must not starve behind shared retry",
                    }[control]
                    if result.returncode != 12 or marker not in text:
                        raise RuntimeError(
                            "native vector omission missed " + str(control) + "\n" + text[-12000:]
                        )
                    continue
                count = {
                    0: 0,
                    1: 1,
                    2: 513,
                    3: 514,
                    4: 520,
                    9: 514,
                    11: 0,
                    13: 0,
                    15: 514,
                    17: 0,
                    18: 0,
                }[control]
                nodes = 64 if control in (11, 13) else 1040
                marker = f"PASS Fast claim storage nodes={nodes} modules={count}"
                if result.returncode or marker not in text:
                    raise RuntimeError(f"control={control} {mode} locked={locked}\n{text[-12000:]}")
                evidence(text, children=2)
                if (
                    control in (9, 15)
                    and "MIXEDRETRY retained=1 replayed=0 refusals=0 passes=0" not in text
                ):
                    raise RuntimeError(
                        "mixed native retry and actual retained cleanup missing\n" + text[-12000:]
                    )
                if control in (11, 13, 17, 18):
                    if (
                        "MIXEDRETRY retained=1 replayed=1 refusals=8 passes=8" not in text
                        or not re.search(r"NATIVEPACKETCYCLE .*?replayed=[1-9]", text)
                    ):
                        raise RuntimeError(
                            "actual shared replay under retained backlog missing\n" + text[-12000:]
                        )
                if not re.search(r"NATIVERC enqueued=(\d+) released=\1 pending=0", text):
                    raise RuntimeError("native vector queue accounting failed\n" + text[-12000:])
    print(
        "PASS Fast claim storage: actual 0/1/513/514/520 Modules, native cardinality, "
        "vector/replay retries, mixed retained backlog, physical frees and final epochs; C/JIT, both polls"
        + (
            "; positive profiles only"
            if positive_only or mixed_only or large_replay
            else "; eight semantic omission controls"
        )
    )


if __name__ == "__main__":
    main()

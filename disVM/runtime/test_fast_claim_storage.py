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
from concurrent_claim_fixture import prepare_claim_fixture
from test_duke_pin_retirement import private_includes


# EN: Own real VM applications/private confirmation relinks from CLI build paths.
# Positive cases use actual Modules and managed graph owners; omission controls
# require exact semantic failures, never build errors, signals or timeouts.
# --positive-only omits intentional violations; --mixed-only selects vector BUSY
# and FULL/BYTES replay without an unknown callback. Retained-assist profiles and
# return-work controls dependent on retained admission stay outside native mode.
# --large-replay selects the default matrix's 1040-node two-slot replay cases.
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
            else (9, 11, 13) if mixed_only else (0, 1, 2, 3, 4, 9, 11, 13, 17, 18, 20)
        )
        controls = (
            positive if positive_only or mixed_only or large_replay else (*positive, 5, 6, 7, 8, 19)
        )
        fixture = prepare_claim_fixture(root, work)
        for control in controls:
            confirmation = rc = capture = None
            if control == 19:
                path = root / "gc/common/graph_capture.c"
                text = path.read_text()
                gate = "if (group->roots_pruned)"
                boundary = "    if (!classify_record_group(group, 1))"
                if text.count(gate) != 1 or text.count(boundary) != 1:
                    raise RuntimeError("Duke root observation restart anchor changed")
                # EN: Observe the actual classification boundary, never accept
                # the missing-seed watchdog as the negative test result.
                # RU: Проверить настоящую границу классификации; watchdog
                # потерянного seed не считается успехом отрицательного теста.
                observer = (
                    "    if (group->roots_pruned)\n"
                    "    {\n"
                    '        fputs("FAIL Fast claim storage: '
                    'pruned root observations cannot classify the last seed\\n", stderr);\n'
                    "        exit(12);\n"
                    "    }\n"
                )
                text = text.replace(gate, "if (0)").replace(boundary, observer + boundary)
                capture = work / "graph_capture.c"
                capture.write_text("#include <stdio.h>\n" + private_includes(text, path))
            if control in (19, 20):
                text = read_rc_source(root)
                begin = text.index("static unsigned int prune_rooted_duke_candidates(void *seed)")
                end = text.index("\n}\n", begin) + 3
                part = text[begin:end]
                lock = "    fvm_port_sync_lock(candidate_transfer);"
                if part.count(lock) != 1:
                    raise RuntimeError("actual prune publication boundary changed")
                part = part.replace(
                    lock,
                    lock + "\n    if (dvm_test_add_prune_owner(duke_candidates, seed))\n    {\n"
                    "        __atomic_fetch_add(&enqueued, 1, __ATOMIC_RELAXED);\n"
                    "        __atomic_fetch_add(&helper_enqueued, 1, __ATOMIC_RELAXED);\n    }",
                )
                text = (
                    text[:begin]
                    + "extern int dvm_test_add_prune_owner(DVM_RetirementQueue *, void *);\n\n"
                    + part
                    + text[end:]
                )
                rc = work / "runtime_rc.c"
                rc.write_text(text)
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
                fixture,
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
                graph_capture_source=capture,
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
                        19: "pruned root observations cannot classify the last seed",
                    }[control]
                    if result.returncode != 12 or marker not in text:
                        raise RuntimeError(
                            "native vector omission missed " + str(control) + "\n" + text[-12000:]
                        )
                    if control == 19 and "PRUNE_OWNER_PUBLISHED actual=1" not in text:
                        raise RuntimeError(
                            "pruning control lacked a real queue-owner publication\n"
                            + text[-12000:]
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
                    20: 0,
                }[control]
                nodes = 64 if control in (11, 13) else 1040
                marker = f"PASS Fast claim storage nodes={nodes} modules={count}"
                if result.returncode or marker not in text:
                    raise RuntimeError(f"control={control} {mode} locked={locked}\n{text[-12000:]}")
                evidence(text, children=2)
                if control == 20 and "PRUNE_OWNER_PUBLISHED actual=1" not in text:
                    raise RuntimeError(
                        "positive restart lacked a real queue-owner publication\n" + text[-12000:]
                    )
                if (
                    control in (9, 15)
                    and "MIXEDRETRY retained=0 replayed=0 refusals=0 passes=0" not in text
                ):
                    raise RuntimeError(
                        "mixed native retry and actual retained cleanup missing\n" + text[-12000:]
                    )
                if control in (11, 13, 17, 18, 20):
                    if (
                        "MIXEDRETRY retained=0 replayed=1 refusals=8 passes=8" not in text
                        or not re.search(r"NATIVEPACKETCYCLE .*?replayed=[1-9]", text)
                    ):
                        raise RuntimeError(
                            "actual shared replay under retained backlog missing\n" + text[-12000:]
                        )
                if not re.search(r"NATIVERC enqueued=(\d+) released=\1 pending=0", text):
                    raise RuntimeError("native vector queue accounting failed\n" + text[-12000:])
    print(
        "PASS Fast claim storage: actual 0/1/513/514/520 Modules, native cardinality, "
        "vector/replay retries without retained callbacks, physical frees and final epochs; C/JIT, both polls"
        + (
            "; positive profiles only"
            if positive_only or mixed_only or large_replay
            else "; five semantic omission controls"
        )
    )


if __name__ == "__main__":
    main()

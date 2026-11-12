# Copyright (c) 2026 bhgv
# EN: Real packet seed preservation, refused graph replay and allocation failures.
# RU: Сохранение настоящих seed пакета, повтор отклонённого графа и отказы выделения.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from rc_test_source import read_rc_source
from relink_private import relink
from resident_evidence import evidence


# EN: Run actual resident C/JIT apps using explicit runtime/compiler/include CLI
# paths. Own temporary relinks and processes through exit. Positive profiles
# cover packet capacity/root failures, duplicates, actual snapshot replay and
# Duke-sized rings. Controls need exact semantic status, never compiler failure,
# timeout or sanitizer report. --positive-only excludes intentional violations.
# RU: Запустить настоящие C/JIT приложения по CLI путям runtime/compiler/include.
# Владеть временными бинарниками/процессами до выхода. Проверить отказы места/
# корня, дубликаты, повтор настоящего снимка и большие кольца ДюкGC. Контроли
# требуют семантического кода, не ошибки сборки/таймаута/санитайзера.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    runtime, compiler, include = (
        Path(value).resolve() for value in sys.argv[1:] if value != "--positive-only"
    )
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="packet cycle retirement ") as temporary:
        work = Path(temporary)
        image = work / "packet-cycle.dis"
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
        positive_controls = (*range(7), 11)
        for control in (
            positive_controls if positive_only else (*positive_controls, 7, 8, 9, 10, 12)
        ):
            replacement = None
            if control not in positive_controls:
                source = read_rc_source(root)
                gates = {
                    12: (
                        "if (active && candidate_transfer && key.generation &&",
                        "if (candidate_transfer && key.generation &&",
                    ),
                    7: (
                        "if (!transferred && record_candidate(heap) && dvm_refcount_load_u(&heap->ref) > 1)",
                        "if (0)",
                    ),
                    8: (
                        "!dvm_gc_field_mark(&heap->color, &mutator, &nprop) || !dvm_runtime_root_add(heap)",
                        "!dvm_gc_field_mark(&heap->color, &mutator, &nprop) || 0",
                    ),
                    9: (
                        "*retry |= reserved == DVM_RETIRE_FULL || reserved == DVM_RETIRE_BYTES;",
                        "*retry |= 0;",
                    ),
                    10: (
                        "if (!preserve_packet_candidate(heap, packet, packet->next, retry))\n                {\n                    return completed;",
                        "if (!preserve_packet_candidate(heap, packet, packet->next, retry))\n                {\n                    packet->next++;\n                    return completed;",
                    ),
                }
                gate, changed = gates[control]
                if control == 8:
                    # Restrict the identical publication predicate to this private helper.
                    offset = source.index("static int preserve_packet_candidate(")
                    prefix, source = source[:offset], source[offset:]
                else:
                    prefix = ""
                if source.count(gate) != 1:
                    raise RuntimeError("packet ownership control anchor changed: " + str(control))
                replacement = work / "runtime_rc.c"
                replacement.write_text(prefix + source.replace(gate, changed))
            binary = relink(
                runtime,
                work,
                Path(__file__).with_suffix(".c"),
                control,
                [
                    "Sys_werrstr",
                    "dvm_runtime_guest_sleep",
                    "dvm_runtime_rc_defer",
                    "dvm_heap_pool_key",
                    "dvm_retirement_reserve",
                    "dvm_retirement_contains_key",
                    "dvm_retirement_commit_ready",
                    "dvm_retirement_commit_key",
                    "dvm_runtime_root_add",
                    "dvm_runtime_root_remove",
                    "dvm_runtime_root_batch_publish",
                    "dvm_runtime_root_batch_remove",
                    "dvm_runtime_root_batch_clear",
                    "dvm_runtime_rc_shared_candidates",
                    "dvm_runtime_rc_shared_leaves",
                    "dvm_gc_fast_confirm_graph",
                    "poolfree",
                    "dvm_world_try_quiescent",
                ],
                runtime_rc_source=replacement,
            )
            profiles = (
                [("--no-jit", False)]
                if control not in positive_controls
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
                    raise RuntimeError("packet cycle sanitizer failure\n" + text[-12000:])
                if control not in positive_controls:
                    marker = {
                        7: "packet cycle owner lost its replacement root",
                        8: "packet cycle owner lost its replacement root",
                        9: "packet slot refusal failed to report shared retry",
                        10: "packet cursor advanced past an untransferred owner",
                        12: "ticket cycle published before source-root withdrawal",
                    }[control]
                    if result.returncode != 12 or marker not in text:
                        raise RuntimeError(f"packet control {control} missed\n{text[-12000:]}")
                    continue
                records = 2300 if control == 6 else 2
                if (
                    result.returncode
                    or f"PASS packet cycle records={records} envelope=1" not in text
                ):
                    raise RuntimeError(f"control={control} {mode} locked={locked}\n{text[-12000:]}")
                evidence(text, children=2)
                packet = re.search(
                    r"NATIVEPACKETCYCLE preserved=(\d+) retries=(\d+) replayed=(\d+)", text
                )
                if (
                    not packet
                    or int(packet[1]) < (2 if control in (4, 5) else 1)
                    or (control in (2, 3, 5) and int(packet[2]) < 1)
                    or (control in (4, 5) and int(packet[3]) < 1)
                ):
                    raise RuntimeError(
                        "actual packet ownership boundary was not observed\n" + text[-12000:]
                    )
                if not re.search(r"NATIVERC enqueued=(\d+) released=\1 pending=0", text):
                    raise RuntimeError("packet queue accounting did not balance\n" + text[-12000:])
                if control == 6:
                    duke = re.search(
                        r"NATIVEDUKEGC promoted=(\d+) peak_fields=(\d+) resumed=(\d+) adopted_pins=(\d+)",
                        text,
                    )
                    if (
                        not duke
                        or int(duke[1]) < 1
                        or int(duke[2]) < records
                        or int(duke[3]) < 1
                        or int(duke[4]) < records - 1
                    ):
                        raise RuntimeError(
                            "actual packet ring did not use resumed Duke\n" + text[-12000:]
                        )
                if control == 11 and not re.search(r"PASS packet cycle .* live=1", text):
                    raise RuntimeError(
                        "reachable-cycle preservation was not observed\n" + text[-12000:]
                    )
                if "PASS resident packet cycle peers=2" not in text:
                    raise RuntimeError(
                        "packet peers did not resume original frames\n" + text[-12000:]
                    )
    print(
        "PASS actual packet cycle retirement: preserved seeds, slot/root refusals, duplicate retry, "
        "refused snapshots, live owner, 2300-node Duke ring; C/JIT, both polls"
        + ("; positive profiles only" if positive_only else "; five semantic omission controls")
    )


if __name__ == "__main__":
    main()

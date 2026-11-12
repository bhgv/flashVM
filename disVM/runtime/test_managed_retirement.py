# Copyright (c) 2026 bhgv
# EN: Real managed ticket strings retire before continuously admitted peers leave.
# RU: Владение настоящими строками передаётся в RC до выхода допущенных соседей.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from relink_private import relink
from resident_evidence import evidence
from rc_test_source import read_rc_source


# EN: Compile the native relay against unchanged runtime objects, then run C/JIT,
# both poll backends and a disabled-shared-path control. CLI supplies exact build
# paths. Own temporary binaries/children; reject compile errors and timeout results.
# RU: Собрать эстафету с настоящими объектами runtime, проверить C/JIT, оба режима
# опроса и контроль с отключённым shared-путём. CLI задаёт пути сборки. Владеть
# файлами/процессами; ошибки компиляции и таймаут не считать успехом контроля.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    runtime, compiler, include = (
        Path(value).resolve() for value in sys.argv[1:] if value != "--positive-only"
    )
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="shared managed retirement ") as temporary:
        work = Path(temporary)
        image = work / "manageds.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(image),
                str(root / "fixtures/resident-managed-retirement/main.b"),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        for control in (0, 4, 5) if positive_only else (0, 4, 5, 1, 2, 3, 6, 7, 8, 9):
            replacement = None
            if control in (2, 3):
                source = root / "gc/fast/managed_retirement.c"
                original = source.read_text()
                gate = (
                    "if (!dvm_world_epoch_elapsed(world, ticket->epoch))"
                    if control == 2
                    else "if (!dvm_runtime_root_remove(heap))"
                )
                if original.count(gate) != 1:
                    raise RuntimeError("managed ticket reader-epoch control anchor changed")
                # Keep relative includes resolving in the original module directory.
                replacement = work / "managed_retirement.c"
                changed = original.replace(gate, "if (0)")
                for include_name in (
                    "../common/managed_queue.h",
                    "../../runtime_rc.h",
                    "../../root_registry.h",
                ):
                    changed = changed.replace(
                        '"' + include_name + '"', '"' + str(source.parent / include_name) + '"'
                    )
                replacement.write_text(changed)
            rc_replacement = None
            if control in (6, 8, 9):
                rc_replacement = work / "runtime_rc.c"
                source = read_rc_source(root)
                gate, replacement_gate = (
                    (
                        "static unsigned int drain_shared_owners(size_t budget, int *retry)\n{\n    if (!budget)",
                        "static unsigned int drain_shared_owners(size_t budget, int *retry)\n{\n    if (1)",
                    )
                    if control == 6
                    else (
                        "if (!entry || !dvm_world_epoch_elapsed(world, entry->epoch))",
                        "if (!entry)",
                    )
                )
                if control == 9:
                    gate = "if (entry)\n                {\n                    retry = 1;\n                }"
                    replacement_gate = gate.replace("retry = 1;", "retry = 0;")
                if source.count(gate) != 1:
                    raise RuntimeError("mandatory drain/leaf epoch control anchor changed")
                rc_replacement.write_text(source.replace(gate, replacement_gate))
            binary = relink(
                runtime,
                work,
                Path(__file__).with_suffix(".c"),
                control,
                [
                    "dvm_runtime_guest_sleep",
                    "Sys_werrstr",
                    "dvm_runtime_rc_defer",
                    "dvm_managed_owner_drain_shared",
                    "dvm_runtime_root_add",
                    "dvm_runtime_rc_shared_leaves",
                    "poolfree",
                    "dvm_world_try_quiescent",
                    "dvm_world_epoch_elapsed",
                    "dvm_world_stop_begin",
                ],
                managed_retirement_source=replacement,
                runtime_rc_source=rc_replacement,
            )
            profiles = (
                [("--no-jit", False)]
                if control in (1, 2, 3, 6, 7, 8, 9)
                else [(mode, locked) for mode in ("--no-jit", "--jit") for locked in (False, True)]
            )
            for mode, locked in profiles:
                result = subprocess.run(
                    [
                        str(binary),
                        "--native-spawn",
                        "--gc-no-cycles",
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
                    raise RuntimeError("managed retirement sanitizer failure\n" + text[-12000:])
                if control in (1, 2, 3, 6, 7, 8, 9):
                    marker = {
                        1: "managed ticket retirement still requires heap quiescence",
                        2: "ticket handoff before reader epoch elapsed",
                        3: "managed ticket root did not transfer exactly once",
                        6: "managed object destruction still requires heap quiescence",
                        7: "managed object destruction still requires heap quiescence",
                        8: "physical destruction overlaps admitted peers after the reader epoch",
                        9: "RC leaf wait failed to report shared retry",
                    }[control]
                    if result.returncode != 12 or marker not in text:
                        raise RuntimeError(f"control {control} was not detected\n" + text[-12000:])
                    continue
                if (
                    result.returncode
                    or "PASS shared managed retirement final=1 external=1 admitted=2 reader_wait=1"
                    not in text
                ):
                    raise RuntimeError(f"{mode} locked={locked}\n{text[-12000:]}")
                evidence(text, children=2)
                counters = re.search(r"MANAGEDSHARED transferred=(\d+) with_peer=(\d+)", text)
                if not counters or int(counters[1]) < 2 or int(counters[2]) < 2:
                    raise RuntimeError(
                        "real shared managed retirement was not counted\n" + text[-12000:]
                    )
                mandatory = re.search(r"NATIVEMANDATORYRC shared=(\d+)", text)
                if control == 5 and (not mandatory or int(mandatory[1]) == 0):
                    raise RuntimeError(
                        "optional root refusal did not exercise mandatory rescue\n" + text[-12000:]
                    )
                if "PASS resident managed retirement peers=2" not in text:
                    raise RuntimeError("resident continuations did not finish\n" + text[-12000:])
    print(
        "PASS shared managed ticket retirement: final and externally rooted strings, "
        "physical free, optional root OOM, held reader epoch, admitted peers, C/JIT, both polls; "
        + (
            "positive/refusal profiles only"
            if positive_only
            else "ticket/leaf epochs, root removal, disabled rescue/reclamation and stopped-admission controls"
        )
    )


if __name__ == "__main__":
    main()

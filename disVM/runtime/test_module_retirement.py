# Copyright (c) 2026 bhgv
# EN: Real module data, native metadata, reader grace and refusal rollback acceptance.
# RU: Приёмка настоящих данных модуля, метаданных, эпох и отката при отказах.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from rc_test_source import read_rc_source
from relink_private import relink
from resident_evidence import evidence


# EN: Compile a real resident app using explicit runtime/compiler/include CLI
# paths. Relink production objects with native observers, never substitute heap,
# unload or collectors. Own private artifacts/children through exit. Positive
# profiles cover final/nonfinal Module ownership, MP publication OOM and changed
# final counts. Controls require exact status/predicate, never compile errors,
# sanitizer reports or timeout. --positive-only omits intentional violations.
# RU: Собрать настоящий resident app по CLI путям runtime/compiler/include.
# Линковать production с наблюдателями, не подменять кучу, unload или сборщики.
# Владеть частными артефактами/детьми до выхода. Проверить последний/непоследний
# Module, OOM публикации MP и изменение счётчика. Контроли требуют точного кода
# и предиката, не ошибок сборки/санитайзера/таймаута. --positive-only исключает нарушения.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    runtime, compiler, include = (
        Path(value).resolve() for value in sys.argv[1:] if value != "--positive-only"
    )
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="shared module retirement ") as temporary:
        work = Path(temporary)
        image = work / "module-retirement.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(image),
                str(root / "fixtures/resident-module-retirement/main.b"),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        positive_controls = (0, 1, 2, 3, 8, 9, 10, 11, 12)
        for control in (
            positive_controls if positive_only else (*positive_controls, 4, 5, 6, 7, 13, 14, 15)
        ):
            replacement = None
            if control in (5, 6, 7):
                source = read_rc_source(root)
                gates = {
                    5: ("if (link->MP == H)", "if (1)"),
                    6: (
                        "if (!entry || !dvm_world_epoch_elapsed(world, entry->epoch))",
                        "if (!entry)",
                    ),
                    7: (
                        "/* The failed module claim has not transferred any MP reference. */",
                        "/* The failed module claim has not transferred any MP reference. */\n"
                        "    *retry = 1;\n    return 0;",
                    ),
                }
                gate, changed = gates[control]
                if source.count(gate) != 1:
                    raise RuntimeError("module retirement control anchor changed: " + str(control))
                replacement = work / "runtime_rc.c"
                replacement.write_text(source.replace(gate, changed))
            if control >= 10:
                source = read_rc_source(root)
                gate = "            retry_leaf |= dvm_refcount_load_u(&entry->heap->ref) > 1;"
                declaration = (
                    "/* [GC:] EN: Visit source under sole Fast shared admission with an independent"
                )
                sole = "    int sole = dvm_refcount_load_u(&heap->ref) == 1;"
                sole_doc = (
                    "/* [GC:] EN: Sample independently live heap before child transfer. Return one"
                )
                for anchor in (gate, declaration, sole, sole_doc):
                    if source.count(anchor) != 1:
                        raise RuntimeError("module retry observation anchor drift")
                source = source.replace(
                    declaration, "extern void dvm_test_module_retry(Heap *, int);\n" + declaration
                )
                source = source.replace(
                    gate, gate + "\n            dvm_test_module_retry(entry->heap, retry_leaf);"
                )
                source = source.replace(
                    sole_doc, "extern int dvm_test_module_sole_owner(Heap *);\n\n" + sole_doc
                )
                source = source.replace(sole, "    int sole = dvm_test_module_sole_owner(heap);")
                if control >= 13:
                    gate = "    *retry |= !claimed;" if control == 13 else "    *retry |= !sole;"
                    if source.count(gate) != 1:
                        raise RuntimeError("module captured refusal omission anchor drift")
                    source = source.replace(gate, "    (void)retry;")
                replacement = work / "runtime_rc.c"
                replacement.write_text(source)
            binary = relink(
                runtime,
                work,
                Path(__file__).with_suffix(".c"),
                control,
                [
                    "Sys_werrstr",
                    "dvm_runtime_guest_sleep",
                    "dvm_runtime_rc_defer",
                    "dvm_managed_owner_drain_shared",
                    "dvm_runtime_rc_shared_leaves",
                    "dvm_runtime_root_batch_publish",
                    "dvm_refcount_claim_last_u",
                    "dvm_gc_fast_module_eligible",
                    "dvm_gc_fast_module_finalize",
                    "dvm_world_try_quiescent",
                    "poolfree",
                    "free",
                    "munmap",
                    "cjitexec",
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
                    raise RuntimeError("module retirement sanitizer failure\n" + text[-12000:])
                if control not in positive_controls:
                    marker = {
                        4: "ordinary module retirement still requires heap quiescence",
                        5: "module data lacks its transferred root",
                        6: "module finalized before reader epoch elapsed",
                        7: "module retry leaked staged data roots",
                        13: "captured module owner refusal must request shared retry",
                        14: "captured module owner refusal must request shared retry",
                        15: "captured module owner refusal must request shared retry",
                    }[control]
                    if result.returncode != 12 or marker not in text:
                        raise RuntimeError(f"module control {control} missed\n{text[-12000:]}")
                    continue
                heaps = 1 if control in (8, 10, 11) else 4
                if (
                    result.returncode
                    or f"PASS shared module retirement heaps={heaps} metadata=1 reader_epochs=2"
                    not in text
                ):
                    raise RuntimeError(f"control={control} {mode} locked={locked}\n{text[-12000:]}")
                if control == 9 and (
                    not re.search(r"PASS shared module retirement .* code=1", text)
                    or "PASS actual JIT tail returned after original mapping unmap" not in text
                ):
                    raise RuntimeError(
                        "actual target JIT mapping was not unmapped\n" + text[-12000:]
                    )
                evidence(text, children=2)
                if control == 10 and "claim_refusal=1 entry_refusal=0" not in text:
                    raise RuntimeError("actual empty module claim loss missing\n" + text[-12000:])
                if control in (11, 12) and "entry_refusal=1" not in text:
                    raise RuntimeError(
                        "actual preliminary module count loss missing\n" + text[-12000:]
                    )
                modules = re.search(r"NATIVEMODULERC shared=(\d+) with_peer=(\d+)", text)
                if not modules or int(modules[1]) < 1 or int(modules[2]) < 1:
                    raise RuntimeError(
                        "shared module finalization was not counted\n" + text[-12000:]
                    )
                if not re.search(r"NATIVERC enqueued=(\d+) released=\1 pending=0", text):
                    raise RuntimeError("module release owners did not balance\n" + text[-12000:])
                if "PASS resident module retirement peers=2" not in text:
                    raise RuntimeError(
                        "module peers did not finish original continuations\n" + text[-12000:]
                    )
    print(
        "PASS actual ordinary module retirement: mapped/empty modules and metadata, two reader epochs, "
        "lookup owner, MP root refusal, competing retain, actual JIT unmap; C/JIT, both polls"
        + ("; positive profiles only" if positive_only else "; seven semantic omission controls")
    )


if __name__ == "__main__":
    main()

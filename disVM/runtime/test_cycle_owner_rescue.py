# Copyright (c) 2026 bhgv
# EN: Real cyclic records travel through shared RC, mandatory FIFO and indexed retries.
# RU: Настоящие циклические записи проходят shared RC, обязательный FIFO и повторы индекса.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from rc_test_source import read_rc_source
from relink_private import relink
from resident_evidence import evidence


# EN: Run actual C/JIT residents and cycle collectors using the explicit CLI
# runtime/compiler/include paths. Own private relinks and children until exit.
# Positive profiles cover direct publication, slot refusal and duplicate index
# overlap; omission controls require specific semantic failures, never timeout,
# compiler error or sanitizer report. --positive-only omits intentional violations.
# RU: Запустить настоящие C/JIT процессы и сборщики по CLI путям runtime/compiler/
# include. Владеть частными бинарниками и детьми до выхода. Положительные профили
# проверяют прямую передачу, отказ места и дубликат индекса; контроли требуют
# конкретной семантической ошибки, не таймаута/ошибки сборки/санитайзера.
# --positive-only исключает намеренные нарушения протокола.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    runtime, compiler, include = (
        Path(value).resolve() for value in sys.argv[1:] if value != "--positive-only"
    )
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="cycle owner rescue ") as temporary:
        work = Path(temporary)
        image = work / "cycle-rescue.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(image),
                str(root / "fixtures/resident-cycle-rescue/main.b"),
            ],
            capture_output=True,
            text=True,
            check=True,
        )
        positive_controls = (0, 1, 2, 7)
        for control in positive_controls if positive_only else (*positive_controls, 3, 4, 5, 6):
            replacement = None
            if control not in positive_controls:
                source = read_rc_source(root)
                gates = {
                    3: (
                        "int candidate = record_candidate(heap);\n    if (!candidate",
                        "int candidate = active && record_candidate(heap);\n    if (!candidate",
                    ),
                    4: (
                        "static int rescue_cycle_owner(Heap *heap)\n{",
                        "static int rescue_cycle_owner(Heap *heap)\n{\n    return 0;",
                    ),
                    5: (
                        "int ready;\n    if (!dvm_retirement_take(retirement",
                        "int ready;\n    if (!dvm_runtime_root_remove(heap)) { abort(); }\n"
                        "    if (!dvm_retirement_take(retirement",
                    ),
                    6: (
                        "*retry |= transfer == 0;\n                dvm_retirement_rotate(retirement);",
                        "*retry = 0;\n                dvm_retirement_rotate(retirement);",
                    ),
                }
                gate, changed = gates[control]
                if source.count(gate) != 1:
                    raise RuntimeError("cycle rescue omission anchor changed: " + str(control))
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
                    "dvm_retirement_reserve",
                    "dvm_retirement_contains_key",
                    "dvm_retirement_commit_key",
                    "dvm_runtime_rc_shared_candidates",
                    "dvm_runtime_rc_shared_leaves",
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
                    raise RuntimeError("cycle rescue sanitizer failure\n" + text[-12000:])
                if control not in positive_controls:
                    marker = {
                        3: "shared cycle owner was dropped before publication",
                        4: "cycle owner rescue still requires heap quiescence",
                        5: "mandatory cycle root disappeared before destination commit",
                        6: "pending cycle owner requested stopped Fast admission",
                    }[control]
                    if result.returncode != 12 or marker not in text:
                        raise RuntimeError(
                            f"cycle rescue control {control} missed\n{text[-12000:]}"
                        )
                    continue
                records = 2300 if control == 7 else 2
                if result.returncode or f"PASS cycle owner rescue records={records}" not in text:
                    raise RuntimeError(f"control={control} {mode} locked={locked}\n{text[-12000:]}")
                evidence(text, children=2)
                rescued = re.search(r"NATIVEMANDATORYCYCLE transferred=(\d+) retries=(\d+)", text)
                if (
                    not rescued
                    or (control in (1, 2) and int(rescued[1]) == 0)
                    or (control == 2 and int(rescued[2]) == 0)
                ):
                    raise RuntimeError(
                        "required actual cycle handoff was not counted\n" + text[-12000:]
                    )
                if control == 7:
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
                            "actual large graph did not use resumed Duke slices\n" + text[-12000:]
                        )
                if not re.search(r"NATIVERC enqueued=(\d+) released=\1 pending=0", text):
                    raise RuntimeError("accepted cycle owners did not drain\n" + text[-12000:])
                if "PASS resident cycle rescue peers=2" not in text:
                    raise RuntimeError(
                        "original resident continuations did not finish\n" + text[-12000:]
                    )
    print(
        "PASS actual cycle owner rescue: small/2300-node records physically freed with admitted peers; "
        "shared staging, candidate refusal, duplicate retry, resumed Duke, C/JIT, both polls"
        + ("; positive profiles only" if positive_only else "; four semantic omission controls")
    )


if __name__ == "__main__":
    main()

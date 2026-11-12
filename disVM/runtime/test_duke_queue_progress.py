# Copyright (c) 2026 bhgv
# EN: Actual oversized Duke graph and independent shared progress under queue pressure.
# RU: Настоящий слишком большой граф Duke и независимый shared-прогресс при давлении очередей.
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from relink_private import relink
from resident_evidence import evidence
from rc_test_source import read_rc_source


# [GC:] EN: Borrow CLI build/compiler/include paths and own private binaries and
# real VM children. Check FULL/BYTES with C/JIT and both polls. Failure must carry
# an exact semantic diagnostic, never a compiler error, signal or sanitizer report.
# RU: Заимствовать CLI-пути сборки/компилятора/include; владеть личными бинарниками
# и настоящими VM. Проверить FULL/BYTES, C/JIT и оба polling. Отказ требует точной
# семантической диагностики, не ошибки сборки, сигнала или санитайзера.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    runtime, compiler, include = (Path(v).resolve() for v in sys.argv[1:] if v != "--positive-only")
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="actual Duke queue progress ") as folder:
        work = Path(folder)
        image = work / "peers.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-o",
                str(image),
                str(root / "fixtures/resident-managed-retirement/main.b"),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        for control in (0, 1) if positive_only else (0, 1, 100, 101):
            duke = rc = None
            if control == 100:
                path = root / "gc/duke/large_graph_pass.c"
                source = path.read_text()
                anchor = "if (!dvm_rc_move_candidate(queue, fallback, value))"
                if source.count(anchor) != 1:
                    raise RuntimeError("Duke failed-transfer control anchor changed")
                offset = source.index(anchor)
                source = source[:offset] + source[offset:].replace(
                    "dvm_retirement_rotate(queue);", "(void)0;", 1
                )
                source = re.sub(
                    r'^#include "([^"\n]+)"$',
                    lambda m: (
                        '#include "' + str((path.parent / m[1]).resolve()) + '"'
                        if (path.parent / m[1]).is_file()
                        else m[0]
                    ),
                    source,
                    flags=re.M,
                )
                duke = work / "large_graph_pass.c"
                duke.write_text(source)
            if control == 101:
                source = read_rc_source(root)
                anchor = "        if (source == duke_candidates)\n        {\n            return 0;\n        }"
                if source.count(anchor) != 1:
                    raise RuntimeError("Duke hard-limit routing control anchor changed")
                rc = work / "runtime_rc.c"
                rc.write_text(source.replace(anchor, ""))
            binary = relink(
                runtime,
                work,
                root / "test_duke_queue_progress.c",
                control,
                [
                    "dvm_runtime_guest_sleep",
                    "Sys_werrstr",
                    "poolfree",
                    "dvm_gc_duke_run",
                    "dvm_retirement_reserve",
                ],
                runtime_rc_source=rc,
                duke_pass_source=duke,
            )
            profiles = (
                [("--no-jit", False)]
                if control >= 100
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
                if "WARNING: ThreadSanitizer" in text or "runtime error:" in text:
                    raise RuntimeError("Duke progress sanitizer failure\n" + text[-12000:])
                if control >= 100:
                    marker = (
                        "independent Duke graph must bypass hard-refused head"
                        if control == 100
                        else "hard-limit graph must leave the shared rescue loop"
                    )
                    if result.returncode != 12 or marker not in text:
                        raise RuntimeError(
                            "Duke semantic omission was not detected\n" + text[-12000:]
                        )
                else:
                    if (
                        result.returncode
                        or "PASS Duke queue progress hard=70000 independent=2300" not in text
                    ):
                        raise RuntimeError(
                            f"control={control} {mode} locked={locked}\n{text[-12000:]}"
                        )
                    evidence(text, children=2)
    print(
        "PASS actual Duke queue progress: 70000/2300 mapped cycles, FULL/BYTES, roots, physical frees; C/JIT, both polls"
    )


if __name__ == "__main__":
    main()

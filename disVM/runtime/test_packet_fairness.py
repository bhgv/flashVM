# Copyright (c) 2026 bhgv
# EN: Validate real frame packet fairness and detect removal of the bypass.
# RU: Проверить обход настоящих пакетов кадров и обнаружить удаление обхода.
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink
from rc_test_source import read_rc_source
from resident_evidence import evidence


# [GC:] EN: Borrow CLI runtime/compiler/include paths; own real bytecode, private
# relinks and VM children through exit. Check C/JIT, both polls and a semantic
# no-rotation control; compile errors, signals and sanitizer output never pass.
# RU: Заимствовать CLI-пути runtime/compiler/include; владеть байткодом, личными
# сборками и VM до выхода. Проверить C/JIT, оба polling и контроль без перемещения;
# ошибки сборки, сигналы и сообщения санитайзера не считать успехом.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    runtime, compiler, include = (Path(v).resolve() for v in sys.argv[1:] if v != "--positive-only")
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="real packet fairness ") as folder:
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
        for control in (0, 1, 2) if positive_only else (0, 1, 2, 100, 101):
            replacement = None
            if control >= 100:
                source = read_rc_source(root)
                anchor = "            dvm_retirement_rotate(frames);"
                if source.count(anchor) != 1:
                    raise RuntimeError("packet rotation control anchor changed")
                replacement = work / "runtime_rc.c"
                changed = (
                    "            return completed;"
                    if control == 100
                    else anchor + "\n            completed++;"
                )
                replacement.write_text(source.replace(anchor, changed))
            binary = relink(
                runtime,
                work,
                root / "test_packet_fairness.c",
                control,
                [
                    "dvm_runtime_guest_sleep",
                    "Sys_werrstr",
                    "poolfree",
                    "dvm_runtime_rc_shared_leaves",
                ],
                runtime_rc_source=replacement,
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
                if "WARNING: ThreadSanitizer" in text or "runtime error:" in text:
                    raise RuntimeError("packet fairness sanitizer failure\n" + text[-12000:])
                if control >= 100:
                    marker = (
                        "independent packets must bypass unsupported head before quiescence"
                        if control == 100
                        else "blocked rotation must yield without inventing progress"
                    )
                    if result.returncode != 12 or marker not in text:
                        raise RuntimeError(
                            "packet bypass omission was not detected\n" + text[-12000:]
                        )
                elif (
                    result.returncode
                    or "PASS packet fairness independent=3 blocked=2 retained=1 peers=2" not in text
                ):
                    raise RuntimeError(f"{mode} locked={locked}\n{text[-12000:]}")
                else:
                    evidence(text, children=2)
    print(
        "PASS real packet fairness: mixed prefix/cursor/suffix, independent physical frees, retained callback; budgets 1/2/default, C/JIT, both polls; two semantic controls"
        if not positive_only
        else "PASS real packet fairness: positive profiles, budgets 1/2/default, C/JIT, both polls"
    )


if __name__ == "__main__":
    main()

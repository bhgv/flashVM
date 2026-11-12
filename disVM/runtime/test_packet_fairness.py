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
# relinks and VM children through exit. Check C/JIT, both polls, queue fairness,
# refusal visit bounds and semantic omission controls; compile errors, signals and sanitizer output never pass.
# RU: Заимствовать CLI-пути runtime/compiler/include; владеть байткодом, личными
# сборками и VM до выхода. Проверить C/JIT, оба polling, справедливые квоты,
# предел отказов и контрольные сборки с пропусками;
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
        for control in (
            (0, 1, 2, 3, 4, 5, 6, 7)
            if positive_only
            else (0, 1, 2, 3, 4, 5, 6, 7, 100, 101, 102, 103, 104, 105, 106)
        ):
            replacement = None
            admission = None
            if control == 102:
                source = (root / "gc/common/native_admission.c").read_text()
                anchor = "dvm_gc_workers_step_fast(dvm_gc_runtime.workers)"
                if source.count(anchor) != 1:
                    raise RuntimeError("shared retry control anchor changed")
                source = source.replace('"../collector_passes.h"', '"gc/collector_passes.h"')
                source = source.replace('"runtime_state.h"', '"gc/common/runtime_state.h"')
                admission = work / "native_admission.c"
                admission.write_text(
                    source.replace('"../../', '"').replace(
                        anchor,
                        "(dvm_gc_workers_request(dvm_gc_runtime.workers, DVM_GC_FAST) && dvm_gc_workers_wait_fast_idle(dvm_gc_runtime.workers))",
                    )
                )
            if control == 106:
                source = read_rc_source(root)
                anchor = "    while (budget && visits-- && !heapmonitor)\n    {\n        budget--;"
                if source.count(anchor) != 1:
                    raise RuntimeError("Fast refused visit charge anchor changed")
                replacement = work / "runtime_rc.c"
                replacement.write_text(source.replace(anchor, anchor.rsplit("\n", 1)[0]))
            if control in (103, 104, 105):
                source = read_rc_source(root)
                anchor = "        fast_work_budget(budget, shares);"
                if source.count(anchor) != 1:
                    raise RuntimeError("Fast queue budget anchor changed")
                if control == 105:
                    mutation = (
                        "shares[FastLeaves] += shares[FastFrames];\n        shares[FastFrames] = 0;"
                    )
                else:
                    queue, kind = (
                        ("retirement", "FastOwners")
                        if control == 103
                        else ("record_retries", "FastRetries")
                    )
                    mutation = (
                        f"if (dvm_retirement_peek({queue}))\n        {{\n"
                        f"            memset(shares, 0, sizeof(shares));\n"
                        f"            shares[{kind}] = budget;\n        }}"
                    )
                replacement = work / "runtime_rc.c"
                replacement.write_text(source.replace(anchor, anchor + "\n        " + mutation))
            if control in (100, 101):
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
                    "dvm_runtime_channel_claim_observed",
                ],
                runtime_rc_source=replacement,
                native_admission_source=admission,
            )
            profiles = (
                [("--no-jit", False)]
                if control >= 100
                else [(mode, locked) for mode in ("--no-jit", "--jit") for locked in (False, True)]
            )
            for mode, locked in profiles:
                independent, blocked, retained = {
                    4: (5, 3, 2),
                    5: (5, 3, 2),
                    6: (5, 2, 1),
                    7: (6, 3, 2),
                }.get(control, (3, 2, 1))
                expected = f"PASS packet fairness independent={independent} blocked={blocked} retained={retained} peers=2"
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
                    marker = {
                        100: "independent packets must bypass unsupported head before quiescence",
                        101: "blocked rotation must yield without inventing progress",
                        102: "shared retry must not wait for unrelated Fast backlog",
                        103: "independent Fast queues must progress beside blocked owners and retries",
                        104: "independent Fast queues must progress beside blocked owners and retries",
                        105: "independent Fast queues must progress beside blocked owners and retries",
                        106: "refused leaf visits must respect Fast budget",
                    }[control]
                    if result.returncode != 12 or marker not in text:
                        raise RuntimeError(
                            "packet bypass omission was not detected\n" + text[-12000:]
                        )
                elif result.returncode or expected not in text:
                    raise RuntimeError(
                        f"control={control} {mode} locked={locked} exit={result.returncode}\n{text[-12000:]}"
                    )
                else:
                    evidence(text, children=2)
    print(
        "PASS real packet fairness: mixed prefix/cursor/suffix, independent physical frees, retained callback; budgets 1/2/default, C/JIT, both polls; seven semantic controls; bounded shared retry and fair Fast queues"
        if not positive_only
        else "PASS real packet fairness: positive profiles, budgets 1/2/default, C/JIT, both polls"
    )


if __name__ == "__main__":
    main()

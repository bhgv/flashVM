# Copyright (c) 2026 bhgv
# [GC:] EN: Packet fairness with actual native pressure and exclusion forbidden.
# RU: Справедливые пакеты при настоящем отказе памяти и запрещённом эксклюзиве.
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink
from resident_evidence import evidence
from concurrent_packet_fixture import prepare_packet_fixture


# [GC:] EN: Borrow CLI paths; own bytecode/private binaries until exit. Validate
# real packet cursor/root/ref preservation and physical shared cleanup in 32 C/JIT
# and polling profiles. Controls remove rotation, invent progress, or ignore the
# actual allocator failure. Only exact semantic status 12 proves detection.
# RU: Заимствовать CLI-пути; владеть байткодом/сборками до выхода. Проверить
# курсор/корни/ссылки пакетов и настоящую shared-очистку в 32 C/JIT/poll-профилях.
# Контроли убирают вращение, создают ложный прогресс либо игнорируют отказ памяти.
# Только точная семантическая ошибка с кодом 12 подтверждает обнаружение.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    runtime, compiler, include = (Path(v).resolve() for v in sys.argv[1:] if v != "--positive-only")
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="strict concurrent packets ") as folder:
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
        fixture, source = prepare_packet_fixture(root, work)
        for control in tuple(range(8)) if positive_only else (*range(8), 100, 101, 102):
            text = source
            if control in (100, 101):
                anchor = "            dvm_retirement_rotate(frames);"
                if text.count(anchor) != 1:
                    raise RuntimeError("concurrent packet rotation anchor changed")
                text = text.replace(
                    anchor,
                    (
                        "            return completed;"
                        if control == 100
                        else anchor + "\n            completed++;"
                    ),
                )
            rc = work / "runtime_rc.c"
            rc.write_text(text)
            binary = relink(
                runtime,
                work,
                fixture,
                control,
                [
                    "dvm_runtime_guest_sleep",
                    "Sys_werrstr",
                    "poolfree",
                    "dvm_runtime_rc_shared_leaves",
                    "dvm_runtime_channel_claim_observed",
                ],
                runtime_rc_source=rc,
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
                output = result.stdout + result.stderr
                if "Sanitizer" in output or "runtime error:" in output:
                    raise RuntimeError("concurrent packet sanitizer failure\n" + output[-10000:])
                if control >= 100:
                    marker = {
                        100: "independent packets must bypass unsupported head before quiescence",
                        101: "blocked rotation must yield without inventing progress",
                        102: "blocked packet owners must survive allocation refusal",
                    }[control]
                    if result.returncode != 12 or marker not in output:
                        raise RuntimeError(
                            f"concurrent packet control {control} missed\n{output[-10000:]}"
                        )
                elif (
                    result.returncode
                    or "PASS concurrent packet owners and roots peers=2" not in output
                ):
                    raise RuntimeError(
                        f"{control} {mode} locked={locked} exit={result.returncode}\n{output[-10000:]}"
                    )
                else:
                    evidence(output, children=2)
    print(
        "PASS concurrent packets: 32 real C/JIT/poll profiles, allocator pressure, channel retries, exact owners and roots"
        + ("; positive only" if positive_only else "; three semantic controls")
    )


if __name__ == "__main__":
    main()

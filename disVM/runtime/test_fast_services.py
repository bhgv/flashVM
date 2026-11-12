# Copyright (c) 2026 bhgv
# [GC:] EN: Actual Fast service starvation and private legacy coordinator controls.
# RU: Голодание настоящих служб Fast и частные контроли прежнего координатора.
from pathlib import Path
import subprocess
import sys
import tempfile

from relink_private import relink
from resident_evidence import evidence


# [GC:] EN: CLI borrows runtime/compiler/includes; own bytecode and private
# binaries until exit. Exercise real queues/readers in C/JIT and both polls.
# Restored legacy budget 1/2 must fail on exact bounded-attempt starvation,
# never timeout, signal, compilation or sanitizer. --positive-only omits them.
# RU: CLI заимствует runtime/compiler/includes; владеть байткодом/бинарниками
# до выхода. Настоящие очереди и читатели проверяются в C/JIT и обоих опросах.
# Старые бюджеты 1/2 обязаны дать точный отказ по числу попыток, а не таймаут,
# сигнал, ошибку сборки или санитайзера. --positive-only исключает эти контроли.
def main():
    positive = "--positive-only" in sys.argv[1:]
    runtime, compiler, include = (
        Path(value).resolve() for value in sys.argv[1:] if value != "--positive-only"
    )
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="Fast service fairness ") as folder:
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
        for control in range(12) if positive else (*range(12), 100, 101, 104, 108):
            coordinator = None
            if control >= 100:
                source = (root / "gc/fast/retirement_pass.c").read_text()
                begin = source.index("    size_t shares[FastServiceCount];")
                end = source.index("\n}", begin)
                source = (
                    source[:begin]
                    + """    size_t ticket_budget = budget / 3 + (budget % 3 != 0);
    unsigned int tickets = dvm_managed_owner_drain_shared(context, ticket_budget);
    size_t remaining = budget - (tickets ? ticket_budget : 0);
    size_t reserve_budget = remaining / 2 + remaining % 2;
    unsigned int reserves = dvm_runtime_exception_retire_shared(context, reserve_budget);
    return tickets + reserves + dvm_runtime_rc_shared_leaves(context, remaining - (reserves ? reserve_budget : 0));"""
                    + source[end:]
                )
                source = (
                    source.replace('"../collector_passes.h"', '"gc/collector_passes.h"')
                    .replace('"../../', '"')
                    .replace('"service_budget.inc"', '"gc/fast/service_budget.inc"')
                )
                coordinator = work / "retirement_pass.c"
                coordinator.write_text(source)
            binary = relink(
                runtime,
                work,
                root / "test_fast_services.c",
                control,
                [
                    "Sys_werrstr",
                    "dvm_runtime_guest_sleep",
                    "dvm_gc_fast_shared",
                    "dvm_managed_owner_drain_shared",
                    "dvm_runtime_exception_retire_shared",
                    "poolfree",
                ],
                retirement_pass_source=coordinator,
            )
            for mode in ("--no-jit", "--jit"):
                for locked in (False, True):
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
                        timeout=20,
                    )
                    output = result.stdout + result.stderr
                    if "Sanitizer" in output or "runtime error:" in output:
                        raise RuntimeError("Fast services sanitizer failure\n" + output[-10000:])
                    if control >= 100:
                        valid = (
                            result.returncode == 12
                            and "eligible RC starved behind reader-blocked services" in output
                        )
                    else:
                        budget = (1, 2, 3, 256)[control % 4]
                        tickets = int(control // 4 != 2)
                        reserves = int(control // 4 != 1)
                        marker = f"PASS Fast services: budget={budget} RC=1 ticket_retry={tickets} reserve_retry={reserves} peers=2 registers=stable"
                        valid = result.returncode == 0 and marker in output
                        if valid:
                            evidence(output, children=2)
                    if not valid:
                        raise RuntimeError(
                            f"control={control} {mode} locked={locked}: exit={result.returncode}\n{output[-10000:]}"
                        )
    print(
        "PASS Fast services: 48 actual small/default budget profiles"
        + ("" if positive else "; sixteen legacy starvation controls")
    )


if __name__ == "__main__":
    main()

# Copyright (c) 2026 bhgv
# EN: Run the same native lifecycle and graph workloads on each Linux64 backend.
# RU: Запускать одинаковые native-нагрузки процессов и графов на каждом Linux64 backend.
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import platform
import struct
import subprocess
import sys
import time

MACHINES = {  # Accepted native userspace names; no instruction-set substitution.
    "x86_64": "amd64",
    "amd64": "amd64",
    "aarch64": "arm64",
    "arm64": "arm64",
    "riscv64": "riscv64",
    "rv64": "riscv64",
}
CASES = (  # Real drivers; positive-only excludes deliberately broken collector variants.
    ("continuation", "test_module_continuation.py", ("--portable-overlap",)),
    ("process-control", "test_resident_processctl.py", ()),
    ("receiver-cancel", "test_resident_processctl.py", ("--receiver-cancel",)),
    ("live-parent-reply", "test_late_reply.py", ()),
    ("native-cancel", "test_native_cancel.py", ()),
    ("table-transfer", "test_table_transfer.py", ()),
    ("managed-retirement", "test_managed_retirement.py", ("--positive-only",)),
    ("module-retirement", "test_module_retirement.py", ("--positive-only",)),
    ("cycle-rescue", "test_cycle_owner_rescue.py", ("--positive-only",)),
    ("duke-pins", "test_duke_pin_retirement.py", ("--positive-only",)),
    ("packet-cycles", "test_packet_cycle_retirement.py", ("--positive-only",)),
)


# EN: Execute case with borrowed matching build products. Own its subprocess and
# log through completion. Return serializable status/duration/log metadata;
# timeout or launch failure is a failed case, never architectural acceptance.
# RU: Выполнить case на согласованных продуктах. Владеть процессом и журналом
# до завершения. Вернуть статус/время/путь журнала; таймаут и ошибка запуска —
# отказ, а не подтверждение работоспособности архитектуры.
def execute(case, runtime, compiler, include, logs):
    name, driver, flags = case
    path = logs / (name + ".log")
    command = [
        sys.executable,
        str(Path(__file__).parent / driver),
        str(runtime),
        str(compiler),
        str(include),
        *flags,
    ]
    started = time.monotonic()
    with path.open("w") as output:
        try:
            result = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, timeout=1200)
            status = result.returncode
        except (OSError, subprocess.TimeoutExpired) as failure:
            output.write("\nFAIL architecture workload: " + str(failure) + "\n")
            status = 1
    row = {
        "case": name,
        "status": status,
        "seconds": round(time.monotonic() - started, 3),
        "log": str(path),
    }
    print(f"{'PASS' if status == 0 else 'FAIL'} {name}: {row['seconds']}s", flush=True)
    if status:
        diagnostic = path.read_text()
        if len(diagnostic) > 22000:
            print(
                diagnostic[:6000]
                + "\n... full output retained in case log ...\n"
                + diagnostic[-16000:],
                flush=True,
            )
        else:
            print(diagnostic, flush=True)
    return row


# EN: CLI build selects an already completed console build. Require Linux64,
# expected userspace and matching CMake backend inputs before executing guests.
# --emulated labels results honestly; it does not alter tests or enable fallback.
# Own logs/report below build, join every case, return nonzero if any failed.
# RU: CLI build задаёт готовую console-сборку. До запуска гостей требовать Linux64,
# ожидаемый userspace и согласованный CMake CPU. --emulated лишь помечает результат,
# не меняет тесты и не включает fallback. Владеть журналами/отчётом внутри build,
# дождаться всех случаев; любой отказ даёт ненулевой код.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--expected", choices=("amd64", "arm64", "riscv64"), required=True)
    parser.add_argument("--jobs", type=int, choices=range(1, 5), default=2)
    parser.add_argument("--emulated", action="store_true")
    parser.add_argument("--list", action="store_true")
    args = parser.parse_args()
    if args.list:
        for name, driver, flags in CASES:
            print(name, driver, *flags)
        return 0
    machine = platform.machine().lower()
    if (
        platform.system() != "Linux"
        or struct.calcsize("P") != 8
        or MACHINES.get(machine) != args.expected
    ):
        raise RuntimeError(f"wrong execution userspace: {platform.system()}/{machine}")
    build = args.build.resolve()
    runtime, compiler = build / "disvm", build / "bootstrap/limbo-bootstrap"
    include = Path(__file__).resolve().parents[1] / "upstream/module"
    if not runtime.is_file() or not compiler.is_file():
        raise RuntimeError("build the console and bootstrap compiler before acceptance")
    facts = build / "CMakeFiles"
    systems = list(facts.glob("*/CMakeSystem.cmake"))
    if len(systems) != 1:
        raise RuntimeError("missing or ambiguous configured CMake system")
    system = systems[0].read_text()
    if f'set(CMAKE_SYSTEM_PROCESSOR "{platform.machine()}")' not in system:
        raise RuntimeError("build CPU differs from executing userspace")
    logs = build / "native-acceptance"
    logs.mkdir(exist_ok=True)
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        rows = list(pool.map(lambda case: execute(case, runtime, compiler, include, logs), CASES))
    report = {
        "backend": args.expected,
        "machine": machine,
        "emulated": args.emulated,
        "jit_machine_code_instrumented": False,
        "cases": rows,
    }
    (logs / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    failed = sum(row["status"] != 0 for row in rows)
    print(
        f"Architecture {args.expected} emulated={args.emulated}: {len(rows)-failed}/{len(rows)} passed"
    )
    return int(failed != 0)


if __name__ == "__main__":
    raise SystemExit(main())

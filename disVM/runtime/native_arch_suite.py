# Copyright (c) 2026 bhgv
# EN: Run the same native lifecycle and graph workloads on each Linux64 backend.
# RU: Запускать одинаковые native-нагрузки процессов и графов на каждом Linux64 backend.
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import platform
import re
import struct
import subprocess
import sys
import time
from native_arch_cases import PROFILES

MACHINES = {  # Accepted native userspace names; no instruction-set substitution.
    "x86_64": "amd64",
    "amd64": "amd64",
    "aarch64": "arm64",
    "arm64": "arm64",
    "riscv64": "riscv64",
    "rv64": "riscv64",
}
ELF_MACHINES = {62: "amd64", 183: "arm64", 243: "riscv64"}  # ELF e_machine for Linux64 targets.


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


# EN: Read borrowed executable's ELF64 little-endian header before execution.
# Return its declared backend; malformed/unknown images fail before guest startup.
# No program is run and no executable ownership is acquired.
# RU: Прочитать ELF64 little-endian header заимствованного path до исполнения.
# Вернуть backend; неверный/неизвестный образ отвергнуть до старта гостей.
# Программа не запускается, владение executable не передаётся.
def elf_backend(path):
    with path.open("rb") as image:
        header = image.read(64)
    if len(header) != 64 or header[:6] != b"\x7fELF\x02\x01":
        raise RuntimeError("expected ELF64 little-endian executable: " + str(path))
    machine = ELF_MACHINES.get(struct.unpack_from("<H", header, 18)[0])
    if not machine:
        raise RuntimeError("unsupported ELF machine: " + str(path))
    return machine


# EN: CLI build selects an already completed console build. Require Linux64,
# matching ELF VM/compiler and configured target before executing guests.
# --emulated labels results honestly; it does not alter tests or enable fallback.
# It permits a different Python host CPU; binfmt/emulator must execute target ELFs.
# Own logs/report below build, join every case, return nonzero if any failed.
# RU: CLI build задаёт готовую console-сборку. До запуска гостей требовать Linux64,
# ELF VM/компилятора и CMake CPU. --emulated разрешает другой CPU Python-host,
# но не подменяет целевые ELF и не меняет тесты/политику. Владеть отчётами в build,
# дождаться всех случаев; любой отказ даёт ненулевой код.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--expected", choices=("amd64", "arm64", "riscv64"), required=True)
    parser.add_argument("--jobs", type=int, choices=range(1, 5), default=2)
    parser.add_argument("--emulated", action="store_true")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--profile", choices=PROFILES, default="core")
    args = parser.parse_args()
    cases = PROFILES[args.profile]
    if args.list:
        for name, driver, flags in cases:
            print(name, driver, *flags)
        return 0
    machine = platform.machine().lower()
    if (
        platform.system() != "Linux"
        or struct.calcsize("P") != 8
        or (not args.emulated and MACHINES.get(machine) != args.expected)
    ):
        raise RuntimeError(f"wrong execution userspace: {platform.system()}/{machine}")
    build = args.build.resolve()
    runtime, compiler = build / "disvm", build / "bootstrap/limbo-bootstrap"
    include = Path(__file__).resolve().parents[1] / "upstream/module"
    if not runtime.is_file() or not compiler.is_file():
        raise RuntimeError("build the console and bootstrap compiler before acceptance")
    if elf_backend(runtime) != args.expected or elf_backend(compiler) != args.expected:
        raise RuntimeError("VM/compiler ELF target differs from requested architecture")
    facts = build / "CMakeFiles"
    systems = list(facts.glob("*/CMakeSystem.cmake"))
    if len(systems) != 1:
        raise RuntimeError("missing or ambiguous configured CMake system")
    system = systems[0].read_text()
    configured = re.search(r'set\(CMAKE_SYSTEM_PROCESSOR "([^"]+)"\)', system)
    if not configured or MACHINES.get(configured[1].lower()) != args.expected:
        raise RuntimeError("configured CPU differs from requested ELF target")
    logs = build / "native-acceptance" / args.profile
    logs.mkdir(parents=True, exist_ok=True)
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        rows = list(pool.map(lambda case: execute(case, runtime, compiler, include, logs), cases))
    report = {
        "backend": args.expected,
        "host_machine": machine,
        "emulated": args.emulated,
        "profile": args.profile,
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

# Copyright (c) 2026 bhgv
# EN: Real RX signal capture, CPU publication, source retention and concurrent GC proof.
# RU: Настоящий RX signal capture, CPU publication, source retention и concurrent GC proof.
from pathlib import Path
import re
import resource
import signal
import subprocess
import sys
import tempfile
import time
from debugger.client import PipeDebugger
from test_owner_location import compile_image
from relink_private import relink
from resident_evidence import evidence


# EN/RU: Require exact semantics; compiler failures, crashes and timeouts are failures.
# Требовать exact semantics; compiler failures, crashes и timeouts — ошибки теста.
def require(condition, message):
    if not condition:
        raise RuntimeError(message)


# EN: Run one real guest. Debug mode checks retained original roots and CPU context
# before releasing the source; default mode must recover without an installed tool.
# CONTROL=1 omits only CPU copy, CONTROL=2 uses actual RX null-read SIGSEGV.
# RU: Выполнить guest. Debug проверяет original roots/CPU context до release source;
# default mode работает без installed tool. CONTROL=1 убирает только CPU copy,
# CONTROL=2 использует настоящий RX null-read SIGSEGV.
def execute(binary, image, work, variant, debug, locked, inspect_native=None, signal_type=None):
    log = work / f"signal-{variant}-{debug}-{locked}.log"
    arguments = ["--jit", "--stats", "--test-control", "--max-instructions", "1000000000"]
    if locked:
        arguments.append("--gc-locked-poll")
    arguments.append(str(image))
    omitted = False
    diagnostic_label = None
    with log.open("w") as error:
        if not debug:
            result = subprocess.run(
                [str(binary), *arguments], capture_output=True, text=True, timeout=30
            )
            output = result.stdout + result.stderr
            require(result.returncode == 0, output)
        else:
            session = PipeDebugger(
                binary, arguments, stdout=subprocess.PIPE, stderr=error, text=True
            )
            process = session.process
            prefix = ""
            try:
                line = process.stdout.readline()
                prefix += line
                match = re.search(r"JITSIGNALTARGET pid=(\d+)", line)
                require(match, "native source marker missing: " + line + log.read_text())
                pid = int(match[1])
                previous_steps = -1
                for ordinal in range(2 if variant == 7 else 1):
                    deadline = time.monotonic() + 8
                    while True:
                        state = session.command(f"status {pid}")
                        if (
                            state.get("state") == "faulted"
                            and state.get("retained")
                            and state.get("steps", 0) > previous_steps
                        ):
                            break
                        require(
                            time.monotonic() < deadline,
                            "native fault not retained: " + str(state) + log.read_text(),
                        )
                        time.sleep(0.001)
                    require(state["error"] == "sys: JIT native signal", str(state))
                    cpu = session.command(f"native-registers {pid}")
                    if variant == 1:
                        require(cpu.get("error") == "no native fault context", str(cpu))
                        omitted = True
                    else:
                        require(
                            cpu.get("ok")
                            and cpu["signal"]
                            == (
                                signal_type
                                if signal_type is not None
                                else (
                                    signal.SIGSEGV
                                    if variant == 2 or ordinal == 1
                                    else signal.SIGBUS if variant == 4 else signal.SIGILL
                                )
                            ),
                            str(cpu),
                        )
                        require(
                            cpu["code"] > 0
                            and int(cpu["pc"], 16)
                            and int(cpu["sp"], 16)
                            and 0 <= cpu["rx_offset"] < 4096
                            and len(cpu["registers"]) in (23, 32),
                            str(cpu),
                        )
                    if variant != 1:
                        argument_name = {"x64": "rdi", "arm64": "x0", "riscv64": "a0"}[
                            cpu["architecture"]
                        ]
                        source_images = re.findall(
                            r"JITSIGNALINPUT register_image=([0-9a-f]+)", log.read_text()
                        )
                        require(
                            len(source_images) > ordinal
                            and cpu["registers"][argument_name] == source_images[ordinal],
                            "kernel argument register does not match actual native entry: "
                            + str(cpu),
                        )
                        if cpu["architecture"] in ("x64", "riscv64"):
                            pc_name, sp_name = (
                                ("rip", "rsp") if cpu["architecture"] == "x64" else ("pc", "sp")
                            )
                            require(
                                cpu["registers"][pc_name] == cpu["pc"]
                                and cpu["registers"][sp_name] == cpu["sp"],
                                str(cpu),
                            )
                    if inspect_native is not None:
                        diagnostic_label = inspect_native(session, pid, cpu)
                    vm = session.command(f"registers {pid}")
                    require(vm.get("retained") and int(vm["FP"], 16), str(vm))
                    require(
                        session.command(f"operation {pid}").get("opcode") == state["opcode"],
                        "original opcode lost",
                    )
                    frame = session.command(f"backtrace {pid} 1")
                    require(frame.get("ok"), "original caller stack lost: " + str(frame))
                    found = False
                    for slot in range(32, frame["bytes"], 8):
                        row = session.command(f"object {pid} stack 1 {slot} 0 64")
                        if row.get("ok") and row.get("kind") == 1:
                            raw = bytes.fromhex(row["hex"])
                            found |= raw[32] == 91 and raw[63] == 173
                    require(found, "original caller payload was released")
                    while True:
                        census = session.command(f"census {pid}")
                        if not (census.get("flags", 0) & 4):
                            break
                        require(time.monotonic() < deadline, "native source census remained busy")
                    require(
                        census.get("ok")
                        and census["arrays"][0] > 0
                        and not (census["flags"] & (2 | 4 | 8)),
                        str(census),
                    )
                    while "JITSIGNALGC completed_while_held=1" not in log.read_text():
                        require(
                            time.monotonic() < deadline,
                            "GC made no progress while native source was held",
                        )
                        time.sleep(0.001)
                    previous_steps = state["steps"]
                    session.command(f"continue {pid}")
                rest, _ = process.communicate(timeout=20)
                require(process.returncode == 0, prefix + rest + log.read_text())
                output = prefix + rest + log.read_text()
            finally:
                session.close()
                if process.poll() is None:
                    process.kill()
                    process.wait()
    require(
        "JITSIGNALDONE caught=1 second="
        + ("1" if variant == 7 else "0")
        + " results=9,14 payload=91,173"
        in output,
        output,
    )
    require(
        output.count("JITFAULT ") == (2 if variant == 7 else 1)
        and "cause=synchronous RX scalar leaf signal" in output,
        output,
    )
    require("JITCPU pid=" in output, "native register values were not logged")
    require(
        output.count("JITSIGNAL signal=") == (2 if variant == 7 else 1)
        and "ThreadSanitizer:" not in output,
        output,
    )
    require("JITSIGNALPROBE sideeffects=" + ("1" if variant == 5 else "0") in output, output)
    require(re.search(r"instructions=\d+ native=[1-9][0-9]*", output), "no actual peer native work")
    evidence(output, 2)
    if omitted:
        print(
            "DETECTED missing native CPU fault publication with normal application retirement",
            flush=True,
        )
    elif diagnostic_label:
        print(diagnostic_label, flush=True)
    else:
        print(f"PASS RX signal variant={variant} debug={debug} locked={locked}", flush=True)

    return output


# EN/RU: Compile public guest and immutable trap probes against the exact runtime objects.
# Собрать public guest и immutable trap probes с точными runtime objects.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="native signal ") as temporary:
        work = Path(temporary)
        image = work / "main.dis"
        compile_image(compiler, include, root, root.parent / "examples/jit-signal/main.b", image)
        symbols = [
            "dvm_runtime_fast",
            "dvm_bigtable_call_ready",
            "dvm_debug_fault",
            "dvm_jit_fault_copy",
            "dvm_gc_fast_shared",
            "dvm_close_worker_stop",
            "dvm_debug_native_gate",
        ]
        for variant in (0, 2, 4, 5, 7, 1, 3, 6, 8, 9):
            binary = relink(runtime, work, root / "test_jit_signal_probe.c", variant, symbols)
            if variant in (8, 9):
                result = subprocess.run(
                    [
                        str(binary),
                        "--jit",
                        "--stats",
                        "--max-instructions",
                        "1000000000",
                        str(image),
                    ],
                    capture_output=True,
                    text=True,
                    timeout=10,
                )
                output = result.stdout + result.stderr
                require(
                    result.returncode == -signal.SIGABRT
                    and (
                        "JITSIGNAL_ROOTS mutation_armed=1"
                        if variant == 9
                        else "JITSIGNAL_DIRTY cleanup_linked=1"
                    )
                    in output
                    and "unsafe recovery refused" in output
                    and "JITFAULT " not in output,
                    "dirty native cleanup was incorrectly recovered: " + output,
                )
                print(
                    "PASS changed native cleanup/control refuses unsafe RX recovery variant="
                    + str(variant),
                    flush=True,
                )
                continue
            if variant in (3, 6):
                result = subprocess.run(
                    [
                        str(binary),
                        "--jit",
                        "--jit",
                        "--stats",
                        "--max-instructions",
                        "1000000000",
                        str(image),
                    ],
                    capture_output=True,
                    text=True,
                    timeout=10,
                )
                output = result.stdout + result.stderr
                require(
                    result.returncode == -signal.SIGILL
                    and (
                        "JITSIGNAL_GUARDED entering=1"
                        if variant == 6
                        else "JITSIGNAL_FOREIGN_C entering=1"
                    )
                    in output
                    and "JITSIGNAL signal=" not in output
                    and "JITFAULT " not in output,
                    "non-RX fault was incorrectly recovered: " + output,
                )
                print(
                    "PASS unsafe native fault retains fatal OS disposition variant=" + str(variant),
                    flush=True,
                )
                continue
            if variant == 1:
                execute(binary, image, work, variant, True, False)
                continue
            for locked in (False, True):
                execute(binary, image, work, variant, False, locked)
                execute(binary, image, work, variant, True, locked)


if __name__ == "__main__":
    main()

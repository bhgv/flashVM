# Copyright (c) 2026 bhgv
# EN: Independent supervisor inspects retained faults/loops/waits without global stop.
# RU: Independent supervisor исследует retained faults/loops/waits без global stop.
from pathlib import Path
import os
import re
import select
import subprocess
import sys
import tempfile
import time

from debugger.client import PipeDebugger, SocketDebugger
from relink_private import relink
from resident_evidence import evidence, retirement_evidence
from native_gc_evidence import require_concurrent_gc
from test_owner_location import compile_image


# EN/RU: Require one observed semantic fact, never treat timeout/crash as detection.
# Проверить наблюдаемый semantic fact; timeout/crash не считаются detection.
def require(condition, message):
    if not condition:
        raise RuntimeError(message)


# EN/RU: Poll native state with a bounded deadline; owner/native callbacks remain independent.
# Читать native state до bounded deadline; owner/native callbacks остаются независимы.
def retained(session, pid, state="paused"):
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        result = session.command("status " + str(pid))
        if result.get("state") == state and result.get("retained"):
            return result
        time.sleep(0.005)
    raise RuntimeError("retained debug state not acknowledged: " + str(result))


# EN: Exercise actual external pipes; all source targets retire normally even for
# semantic omissions. Never dereference copied register addresses in the client.
# RU: Проверить actual external pipes; все source targets завершаются нормально
# даже при semantic omissions. Client не разыменовывает copied register addresses.
def execute(binary, image, mode, locked, work, control=0, socket_transport=False):
    log = work / ("run-" + str(control) + mode + str(locked) + ".log")
    with log.open("w") as error:
        arguments = ["--stats", mode, *(["--gc-locked-poll"] if locked else []), str(image)]
        if socket_transport:
            path = work / "control.sock"
            process = subprocess.Popen(
                [str(binary), "--debug", str(path), *arguments],
                stdout=subprocess.PIPE,
                stderr=error,
                text=True,
            )
            try:
                session = SocketDebugger(path, process)
                require((path.stat().st_mode & 0o777) == 0o600, "debug socket was not private")
            except BaseException:
                if process.poll() is None:
                    process.kill()
                process.wait()
                raise
        else:
            session = PipeDebugger(
                binary, arguments, stdout=subprocess.PIPE, stderr=error, text=True
            )
        process = session.process
        try:
            require(select.select([process.stdout], [], [], 10)[0], "debug guest start missing")
            line = process.stdout.readline()
            prefix = line
            while "DEBUGTARGETS " not in line and line:
                line = process.stdout.readline()
                prefix += line
            match = re.search(
                r"DEBUGTARGETS main=(\d+) loop=(\d+) fault=(\d+) native=(\d+) caught=(\d+) builtin=(\d+)",
                line,
            )
            require(match, "missing debug fixture identities: " + line)
            main, loop, fault, native, caught, builtin = map(int, match.groups())
            bad = None
            original_session = session.session
            session.session = "0" * 32
            require(
                session.command("terminate " + str(main)).get("error") == "stale session",
                "stale VM session command was accepted",
            )
            session.session = original_session
            state = session.command("status " + str(fault))
            if control == 1:
                deadline = time.monotonic() + 5
                while state.get("ok") and state.get("state") == "running":
                    require(time.monotonic() < deadline, "omitted fault stop source did not retire")
                    time.sleep(0.001)
                    state = session.command("status " + str(fault))
                require(
                    not state.get("ok") and state.get("error") == "gone",
                    "fault-stop omission did not retire source normally",
                )
                bad = "DETECTED retained debug missing original fault stop"
            else:
                state = retained(session, fault, "faulted")
                require(state["error"] == "zero divide", "original managed error lost")
                if control == 2:
                    require(state["opcode"] != 67, "copied-opcode omission did not execute")
                    bad = "DETECTED retained debug wrong fault opcode"
                else:
                    require(state["opcode"] == 67, "fault opcode is not original IDIVW")
                    operation = session.command("operation " + str(fault))
                    require(
                        operation["opcode"] == state["opcode"],
                        "operation projection changed opcode",
                    )
                    registers = session.command("registers " + str(fault))
                    require(
                        registers["retained"] and int(registers["FP"], 16), "fault REG not retained"
                    )
                    frame = session.command("backtrace " + str(fault) + " 0")
                    require(
                        frame.get("ok") and frame["pc"] == state["pc"] + 1 and frame["bytes"] >= 64,
                        "fault next-PC/frame geometry wrong",
                    )
                    require(
                        session.command("backtrace " + str(fault) + " 1").get("ok"),
                        "nested caller frame lost",
                    )
                    raw = b""
                    for offset in range(0, frame["bytes"], 64):
                        row = session.command(
                            f"read {fault} stack 0 {offset} {min(64, frame['bytes']-offset)}"
                        )
                        require(row.get("ok"), "retained frame bytes unavailable")
                        raw += bytes.fromhex(row["hex"])
                    require(
                        bytes.fromhex("8877665544332211") in raw, "original big local was destroyed"
                    )
                    root_slot = None
                    payload = None
                    for slot in range(32, frame["bytes"], 8):
                        row = session.command(f"object {fault} stack 0 {slot} 0 64")
                        if row.get("ok") and row.get("kind") == 1:
                            data = bytes.fromhex(row["hex"])
                            if data[32] == 91 and data[63] == 173:
                                root_slot, payload = slot, data
                                break
                    require(root_slot is not None, "fault array root/payload lost")
                    deadline = time.monotonic() + 5
                    while True:
                        census = session.command("census " + str(fault))
                        if not (census.get("flags", 0) & 4):
                            break
                        require(time.monotonic() < deadline, "root-map census stayed busy")
                    require(
                        census.get("ok")
                        and not (census["flags"] & (2 | 4 | 8))
                        and census["arrays"][0] > 0
                        and census["records"][0] > 0,
                        "root-map census failed: " + str(census) + "\n" + log.read_text()[-5000:],
                    )
                    categories = [
                        census[name]
                        for name in (
                            "arrays",
                            "strings",
                            "lists",
                            "channels",
                            "modules",
                            "records",
                            "opaque",
                        )
                    ]
                    require(
                        sum(row[0] for row in categories) == census["objects"]
                        and sum(row[1] for row in categories) == census["bytes"],
                        "census category sums diverged",
                    )
                    prior = session.command("status " + str(loop))["steps"]
                    deadline = time.monotonic() + 3
                    while session.command("status " + str(loop))["steps"] <= prior:
                        require(time.monotonic() < deadline, "other guest stopped with fault")
                    deadline = time.monotonic() + 5
                    while (
                        "NATIVEDEBUGGC completed_fast_work_while_fault_held=1"
                        not in log.read_text()
                    ):
                        require(
                            time.monotonic() < deadline,
                            "Fast GC did not progress with retained fault",
                        )
                        session.command("status " + str(loop))
                    require(
                        session.command(f"object {fault} stack 0 {root_slot} 0 64")["hex"]
                        == payload.hex(),
                        "GC/peer progress changed retained fault payload",
                    )
                    require(
                        session.command("status " + str(fault))["steps"] == state["steps"],
                        "fault guest executed during inspection",
                    )
                    require(
                        not session.command(f"read {fault} stack 0 999999 8").get("ok"),
                        "out of bounds read accepted",
                    )
                    require(
                        not session.command(f"object {fault} stack 0 0 0 8").get("ok"),
                        "native frame header treated as mapped root",
                    )
                    require(
                        not session.command("read " + str(loop) + " stack 0 0 8").get("ok"),
                        "unpaused mutable source memory was exposed",
                    )
                    session.command("pause " + str(loop))
                    stopped = retained(session, loop)
                    require(
                        session.command("step " + str(loop)).get("ok"), "single-step not accepted"
                    )
                    stepped = retained(session, loop)
                    require(
                        stepped["steps"] == stopped["steps"] + 1,
                        "step did not stop at the next Dis boundary",
                    )
                    require(
                        not session.command("step " + str(fault)).get("ok"),
                        "fault retry was silently treated as step",
                    )
                    session.command("continue " + str(loop))
                    deadline = time.monotonic() + 3
                    while session.command("status " + str(loop))["steps"] <= stopped["steps"]:
                        require(
                            time.monotonic() < deadline,
                            "continued loop did not resume original owner",
                        )
                    session.command("pause " + str(native))
                    retained(session, native)
                    require(
                        session.command(f"backtrace {native} 0").get("ok"),
                        "audited native wait roots unavailable",
                    )
                    session.command("continue " + str(native))
                    require(
                        not session.command("status " + str(native))["retained"],
                        "continue cancelled/retained sleep",
                    )
                    session.command("pause " + str(main))
                    retained(session, main)
                    require(
                        session.command(f"read {main} stack 0 0 8").get("ok"),
                        "channel-blocked root/controller unavailable",
                    )
                    caught_state = retained(session, caught, "faulted")
                    require(
                        caught_state["error"] == "debug caught λ marker",
                        "Unicode error text changed",
                    )
                    native_fault = retained(session, builtin, "faulted")
                    require(
                        "Sys_chdir" in native_fault["error"] and native_fault["opcode"] == 9,
                        "native builtin fault lost its caller operation",
                    )
                    require(
                        session.command(f"backtrace {builtin} 0").get("ok")
                        and session.command(f"read {builtin} stack 0 0 8").get("ok"),
                        "native builtin fault caller roots were unwound",
                    )
                    session.command("continue " + str(caught))
                    require(
                        select.select([process.stdout], [], [], 5)[0],
                        "caught handler did not run after release",
                    )
                    require(
                        "DEBUGCAUGHT normal handler resumed" in process.stdout.readline(),
                        "normal exception handling changed",
                    )
                    # A fresh command connection/state read never automatically releases a fault.
                    require(
                        session.command("status " + str(fault))["retained"],
                        "fault released without explicit command",
                    )
            for pid in (loop, fault, native, caught, builtin, main):
                row = session.command("terminate " + str(pid))
                require(row.get("ok") or row.get("error") == "gone", "termination not accepted")
            require(
                process.wait(timeout=10) == (1 if control == 1 else 0),
                "debug fixture exit status changed",
            )
            remainder = process.stdout.read()
        finally:
            session.close()
            if process.poll() is None:
                process.kill()
                process.wait()
    output = prefix + remainder + log.read_text()
    require(
        "ThreadSanitizer:" not in output and "runtime error:" not in output,
        "debug sanitizer failure\n" + output,
    )
    require("JITFAULT " not in output, "ordinary managed errors quarantined native code")
    if control == 1:
        require_concurrent_gc(output)
        retirement_evidence(output, 5)
        require(
            "joined=1 failed=0" in output
            and re.search(r"unhandled=[1-9][0-9]*", output)
            and "PASS Dis application completed" not in output,
            "continued application error was hidden",
        )
        dispatch = re.search(
            r"NATIVEDISPATCH claims=(\d+) finishes=(\d+) active=0 migrated=0", output
        )
        require(dispatch and dispatch[1] == dispatch[2], "error exit lost a dispatch claim")
    else:
        evidence(output, 5)
    native = re.search(r"instructions=(\d+) native=(\d+)", output)
    require(
        native and (mode != "--jit" or int(native[2]) > 0),
        "debug JIT mode executed no native opcodes",
    )
    row = re.search(
        r"NATIVEDEBUGGER made=(\d+) freed=(\d+) faults=(\d+) inspections=(\d+) inspector_joined=1 supervisor_joined=1",
        output,
    )
    require(
        row and row[1] == row[2] and int(row[1]) == 6, "debug native lifetime imbalance\n" + output
    )
    if bad:
        print(bad, flush=True)
    else:
        require(int(row[3]) >= 2 and int(row[4]) > 0, "debug capture/inspector did not execute")
        print("PASS retained debug " + mode + " locked=" + str(locked), flush=True)


# EN/RU: Compile actual guest and two safe source-level projection/stop omissions.
# Собрать actual guest и два safe source-level projection/stop omissions.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="retained debug ") as temporary:
        work = Path(temporary)
        image = work / "main.dis"
        compile_image(
            compiler, include, root, root.parent / "examples/retained-debug/main.b", image
        )
        socket_probe = relink(
            runtime, work, root / "test_debug_socket_probe.c", 0, ["socket", "bind", "close"]
        )
        refused = subprocess.run(
            [str(socket_probe), "--debug", str(work / "mode-probe"), str(image)],
            capture_output=True,
            text=True,
            timeout=10,
        )
        socket_output = refused.stdout + refused.stderr
        require(
            refused.returncode == 2
            and "DEBUGSOCKETMASK mask=0177 mode=0600" in socket_output
            and "DEBUGSOCKETRESTORE matched=1" in socket_output,
            "socket creation/restoration failed: " + socket_output,
        )
        print("PASS debugger socket mode0600 and restored umask before refused startup", flush=True)
        symbols = [
            "dvm_debug_fault",
            "dvm_debug_instruction",
            "dvm_gc_fast_shared",
            "dvm_debug_native_gate",
        ]
        probe = root / "test_retained_debug_probe.c"
        binary = relink(runtime, work, probe, 0, symbols)
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                execute(binary, image, mode, locked, work)
        import socket
        import errno

        try:
            probe_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        except OSError as problem:
            if problem.errno not in (errno.EPERM, errno.EACCES, errno.EAFNOSUPPORT):
                raise
            print(
                "SKIP Unix debugger transport: host forbids AF_UNIX; private pipes passed",
                flush=True,
            )
        else:
            probe_socket.close()
            for mode in ("--no-jit", "--jit"):
                execute(binary, image, mode, False, work, socket_transport=True)
        for control in (1, 2):
            broken = relink(runtime, work, probe, control, symbols)
            execute(broken, image, "--no-jit", False, work, control)


if __name__ == "__main__":
    main()

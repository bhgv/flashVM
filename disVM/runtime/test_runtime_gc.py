# Copyright (c) 2026 bhgv
# EN: Real collector roots, reclamation and resident-thread lifecycle checks.
# RU: Проверки корней, сборки мусора и жизни закреплённых потоков.
"""Run the actual retained collector/heap with native stopped-world ownership."""
from pathlib import Path
import os
import platform
import re
import subprocess
import sys


# Execute a complete real VM, retaining output for root/reclamation/lifecycle
# checks. No replacement heap, simulated collector or Python worker is involved.
def execute(runtime, module, mode, quantum, budget=None, locked=False, event_cycles=False, dedicated_parallel=False, idle_cycles=False):
    command = [str(runtime), "--gc-worker", "--stats", mode]
    if not dedicated_parallel and not event_cycles:
        command += ["--quantum", str(quantum)]
    if dedicated_parallel:
        command += ["--native-spawn", "--guarded-copies", "--parallel-movp",
                    "--parallel-movmp", "--parallel-slicela", "--parallel-channel-buffer",
                    "--parallel-byte-copies", "--parallel-integer"]
    if event_cycles:
        command += ["--native-spawn", "--gc-event-cycles"]
    if idle_cycles:
        command.append("--gc-idle-cycles")
    if locked:
        command.append("--gc-locked-poll")
    if budget is not None:
        command += ["--max-instructions", str(budget)]
    command.append(str(module))
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy",
                       DVM_EDITOR_THREAD_STATS="1")
    result = subprocess.run(command, text=True, capture_output=True,
                            env=environment, timeout=60)
    output = result.stdout + result.stderr
    expected = 1 if budget is not None else 0
    if result.returncode != expected:
        raise RuntimeError(output[-5000:])
    gc = re.search(r"NATIVEGC collector=worker passes=(\d+) snapshots=(\d+) "
                   r"reclaimed=(\d+) joined=1 failed=0", output)
    rc = re.search(r"NATIVERC enqueued=(\d+) released=(\d+) pending=0 saturated=(\d+)", output)
    limits = re.search(r"bytes=0 byte_limited=(\d+)", output)
    if not rc or int(rc[1]) == 0 or rc[1] != rc[2] or not limits:
        raise RuntimeError("deferred RC did not drain:\n" + output[-5000:])
    if dedicated_parallel and (int(limits[1]) or int(rc[3])):
        raise RuntimeError("dedicated RC unexpectedly hit a queue limit:\n" + output[-5000:])
    lanes = re.search(r"failed=0 fast_passes=(\d+) workers=3", output)
    if not lanes or int(lanes[1]) == 0:
        raise RuntimeError("real Fast/Cycles/Duke workers did not execute:\n" + output[-5000:])
    polls = re.search(r"fast_polls=(\d+) locked_polls=(\d+)", output)
    if not polls or (locked and (int(polls[1]) != 0 or int(polls[2]) == 0)) or (not locked and int(polls[1]) == 0):
        raise RuntimeError("poll path evidence mismatch:\n" + output[-5000:])
    if not gc or int(gc[1]) == 0:
        raise RuntimeError("collector did not run/join:\n" + output[-5000:])
    counters = re.search(r"instructions=(\d+) native=(\d+) gc_runs=(\d+) gc_epochs=(\d+)", output)
    if not counters or int(counters[3]) != int(gc[1]):
        raise RuntimeError("actual collector accounting mismatch:\n" + output[-5000:])
    groups = re.search(r"NATIVEFRAMERC .*? record_groups=(\d+)", output)
    shared_reclaimed = int(groups[1]) if groups else 0
    if idle_cycles and (int(gc[2]) != 0 or int(gc[3]) + shared_reclaimed == 0):
        raise RuntimeError("idle GC must reclaim without parking active owners:\n" + output[-5000:])
    if budget is None:
        if ("PASS gc-worker: 12000" not in output or
                "PASS gc-worker shared graph: 64 unordered publications after unwind" not in output or
                "PASS gc-worker deep frame return and unwind" not in output or
                "PASS Dis application completed" not in output or
                (not idle_cycles and quantum != 65536 and min(int(gc[2]), int(gc[3]), int(counters[4])) == 0) or
                (int(counters[2]) > 0) != (mode == "--jit")):
            raise RuntimeError("missing real roots/reclamation evidence:\n" + output[-5000:])
    if event_cycles:
        events = re.search(r"cycle_losses=(\d+) cycle_skips=(\d+)", output)
        if not events or min(map(int, events.groups())) == 0 or int(gc[3]) == 0:
            raise RuntimeError("event cycles did not skip clean work and reclaim cycles:\n" + output[-5000:])
    if dedicated_parallel and budget is None:
        peak = re.search(r"shared_peak=(\d+)", output)
        integers = re.search(r"shared_integer_commits=(\d+)", output)
        slices = re.search(r"parallel_slicela_commits=(\d+)", output)
        if (not peak or int(peak[1]) == 0 or not integers or int(integers[1]) == 0
                or not slices or int(slices[1]) == 0
                or not re.search(r"NATIVEDISPATCH .* active=0 migrated=0", output)):
            raise RuntimeError("dedicated guests did not publish shared graph copies:\n" + output[-5000:])
    if budget is None and quantum == 65536 and (int(rc[3]) or int(limits[1])):
        raise RuntimeError("growing release queue refused publication:\n" + output[-5000:])
    if budget is not None and ("instruction limit exceeded" not in output or "PASS Dis application completed" in output):
        raise RuntimeError("managed error did not retire collector:\n" + output[-5000:])
    if runtime.name == "disvm-editor" and not re.search(
            r"main_dis_tls=0 worker_bound=1 worker_released=1 result=" + str(expected), output):
        raise RuntimeError("UI/Dis/collector owner retirement failed:\n" + output[-5000:])


# Repeat complete applications across dispatch modes and scheduler root boundaries.
# A limited execution must also join a collector after managed error unwinding.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    if sys.argv[3:] == ["--dedicated-only"]:
        for mode in ("--jit", "--no-jit"):
            for locked in (False, True):
                execute(runtime, module, mode, 1, locked=locked, dedicated_parallel=True)
                execute(runtime, module, mode, 1, budget=10000, locked=locked, dedicated_parallel=True)
            execute(runtime, module, mode, 1, event_cycles=True)
        print("PASS dedicated Cycles, event cycles and Fast GC in interpreter and JIT")
        return
    if sys.argv[3:] == ["--idle-cycles"]:
        for mode in ("--jit", "--no-jit"):
            for locked in (False, True):
                execute(runtime, module, mode, 32, locked=locked,
                        dedicated_parallel=True, idle_cycles=True)
        print("PASS dedicated idle Cycles and Fast GC in interpreter and JIT")
        return
    for mode in ("--jit", "--no-jit"):
        for quantum in (31, 257, 2000, 65536):
            execute(runtime, module, mode, quantum)
        execute(runtime, module, mode, 31, 10000)
        execute(runtime, module, mode, 257, locked=True)
        execute(runtime, module, mode, 31, 10000, locked=True)
        if runtime.name == "disvm" and platform.machine() in ("x86_64", "AMD64"):
            execute(runtime, module, mode, 257, event_cycles=True)
            execute(runtime, module, mode, 257, dedicated_parallel=True)
    print("PASS actual native cycle GC: 6 managed root/reclamation runs, 2 growing-queue publication runs, "
          "4 managed-error joins, interpreter/JIT, shared graph with permanently assigned native threads")


if __name__ == "__main__":
    main()

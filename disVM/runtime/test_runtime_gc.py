"""Run the actual retained collector/heap with native stopped-world ownership."""
from pathlib import Path
import os
import platform
import re
import subprocess
import sys


# Execute a complete real VM, retaining output for root/reclamation/lifecycle
# checks. No replacement heap, simulated collector or Python worker is involved.
def execute(runtime, module, mode, quantum, budget=None, locked=False, pool=False):
    command = [str(runtime), "--gc-worker", "--stats", mode,
               "--quantum", str(quantum)]
    if pool:
        command += ["--managed-workers", "2", "--guarded-copies", "--parallel-movp", "--parallel-slicela", "--parallel-movmp"]
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
    if budget is None and int(limits[1]) == 0:
        raise RuntimeError("large-block bound did not execute:\n" + output[-5000:])
    lanes = re.search(r"failed=0 fast_passes=(\d+) workers=2", output)
    if not lanes or int(lanes[1]) == 0:
        raise RuntimeError("real Fast/Cycles workers did not execute:\n" + output[-5000:])
    polls = re.search(r"fast_polls=(\d+) locked_polls=(\d+)", output)
    if not polls or (locked and (int(polls[1]) != 0 or int(polls[2]) == 0)) or (not locked and int(polls[1]) == 0):
        raise RuntimeError("poll path evidence mismatch:\n" + output[-5000:])
    if not gc or int(gc[1]) == 0:
        raise RuntimeError("collector did not run/join:\n" + output[-5000:])
    counters = re.search(r"instructions=(\d+) native=(\d+) gc_runs=(\d+) gc_epochs=(\d+)", output)
    if not counters or int(counters[3]) != int(gc[1]):
        raise RuntimeError("actual collector accounting mismatch:\n" + output[-5000:])
    if budget is None:
        if ("PASS gc-worker: 12000" not in output or
                "PASS gc-worker shared graph: 64 unordered publications after unwind" not in output or
                "PASS Dis application completed" not in output or
                (quantum != 65536 and min(int(gc[2]), int(gc[3]), int(counters[4])) == 0) or
                (int(counters[2]) > 0) != (mode == "--jit")):
            raise RuntimeError("missing real roots/reclamation evidence:\n" + output[-5000:])
    if pool:
        workers = re.search(r"NATIVEPOOL executors=2 joined=1 failed=0 controller_turns=(\d+) worker_turns=(\d+)", output)
        shared = re.search(r"native_releases=(\d+) native_reentries=(\d+)", output)
        commits = re.search(r"parallel_movp_commits=(\d+)", output)
        slices = re.search(r"parallel_slicela_commits=(\d+)", output)
        if not slices or int(slices[1]) != 65:
            raise RuntimeError("expected 64 backing and one overlapping view publication:\n" + output[-5000:])
        records = re.search(r"parallel_movmp_commits=(\d+)", output)
        if not records or int(records[1]) < 32:
            raise RuntimeError("expected shared mapped frame records:\n" + output[-5000:])
        if (not commits or int(commits[1]) < 64 or not workers or min(map(int, workers.groups())) == 0 or "serial_heap=1" not in output
                or not shared or int(shared[1]) < 64 or shared[1] != shared[2]):
            raise RuntimeError("shared graph did not run with the expected worker pool:\n" + output[-5000:])
    if budget is None and quantum == 65536 and int(rc[3]) == 0:
        raise RuntimeError("queue saturation fallback was not exercised:\n" + output[-5000:])
    if budget is not None and ("instruction limit exceeded" not in output or "PASS Dis application completed" in output):
        raise RuntimeError("managed error did not retire collector:\n" + output[-5000:])
    if runtime.name == "disvm-editor" and not re.search(
            r"main_dis_tls=0 worker_bound=1 worker_released=1 result=" + str(expected), output):
        raise RuntimeError("UI/Dis/collector owner retirement failed:\n" + output[-5000:])


# Repeat complete applications across dispatch modes and scheduler root boundaries.
# A limited execution must also join a collector after managed error unwinding.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    for mode in ("--jit", "--no-jit"):
        for quantum in (31, 257, 2000, 65536):
            execute(runtime, module, mode, quantum)
        execute(runtime, module, mode, 31, 10000)
        execute(runtime, module, mode, 257, locked=True)
        execute(runtime, module, mode, 31, 10000, locked=True)
        if runtime.name == "disvm" and platform.machine() in ("x86_64", "AMD64"):
            for locked in (False, True):
                execute(runtime, module, mode, 257, locked=locked, pool=True)
    print("PASS actual native cycle GC: 6 managed root/reclamation runs, 2 bounded-queue saturation runs, "
          "4 managed-error joins, interpreter/JIT, shared graph with two-worker hosted runs")


if __name__ == "__main__":
    main()

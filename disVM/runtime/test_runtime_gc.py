"""Run the actual retained collector/heap with native stopped-world ownership."""
from pathlib import Path
import os
import re
import subprocess
import sys


# Execute a complete real VM, retaining output for root/reclamation/lifecycle
# checks. No replacement heap, simulated collector or Python worker is involved.
def execute(runtime, module, mode, quantum, budget=None):
    command = [str(runtime), "--gc-worker", "--stats", mode,
               "--quantum", str(quantum)]
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
    if not gc or int(gc[1]) == 0:
        raise RuntimeError("collector did not run/join:\n" + output[-5000:])
    counters = re.search(r"instructions=(\d+) native=(\d+) gc_runs=(\d+) gc_epochs=(\d+)", output)
    if not counters or int(counters[3]) != int(gc[1]):
        raise RuntimeError("actual collector accounting mismatch:\n" + output[-5000:])
    if budget is None:
        if ("PASS gc-worker: 12000" not in output or
                "PASS Dis application completed" not in output or
                min(int(gc[2]), int(gc[3]), int(counters[4])) == 0 or
                (int(counters[2]) > 0) != (mode == "--jit")):
            raise RuntimeError("missing real roots/reclamation evidence:\n" + output[-5000:])
    elif "instruction limit exceeded" not in output or "PASS Dis application completed" in output:
        raise RuntimeError("managed error did not retire collector:\n" + output[-5000:])
    if runtime.name == "disvm-editor" and not re.search(
            r"main_dis_tls=0 worker_bound=1 worker_released=1 result=" + str(expected), output):
        raise RuntimeError("UI/Dis/collector owner retirement failed:\n" + output[-5000:])


# Repeat complete applications across dispatch modes and scheduler root boundaries.
# A limited execution must also join a collector after managed error unwinding.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    for mode in ("--jit", "--no-jit"):
        for quantum in (31, 257, 2000):
            execute(runtime, module, mode, quantum)
        execute(runtime, module, mode, 31, 10000)
    print("PASS actual native cycle GC: 6 managed root/reclamation runs, "
          "2 managed-error joins, interpreter/JIT, 3 scheduler quanta")


if __name__ == "__main__":
    main()

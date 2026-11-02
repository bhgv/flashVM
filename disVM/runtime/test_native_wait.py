"""Execute native sleep with real stack roots and actual Fast/Cycles workers."""
from pathlib import Path
import os
import re
import subprocess
import sys


# Every execution is a whole managed program. Scalar status and live graph checks
# are inside Limbo; runtime diagnostics verify matched releases and real GC work.
def execute(runtime, module, mode, gc_flags, quantum, limited=False):
    command = [str(runtime), mode, "--stats", "--quantum", str(quantum), *gc_flags]
    if limited:
        command += ["--max-instructions", "70"]
    command.append(str(module))
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy",
                       DVM_EDITOR_THREAD_STATS="1")
    result = subprocess.run(command, env=environment, text=True,
                            capture_output=True, timeout=30)
    output = result.stdout+result.stderr
    expected = 1 if limited else 0
    if result.returncode != expected:
        raise RuntimeError(output[-5000:])
    if limited:
        if "instruction limit exceeded" not in output or "PASS Dis application completed" in output:
            raise RuntimeError("missing managed-error retirement:\n"+output[-5000:])
    elif "PASS native wait: nested managed roots" not in output:
        raise RuntimeError("managed native-frame/root checks did not execute:\n"+output[-5000:])
    if "--gc-worker" in gc_flags:
        counters = re.search(r"native_releases=(\d+) native_reentries=(\d+) native_gc_progress=(\d+)", output)
        if (not counters or int(counters[1]) == 0 or counters[1] != counters[2] or
                int(counters[3]) == 0 or
                (not limited and int(counters[1]) != 6) or
                "pending=0" not in output or "joined=1 failed=0" not in output):
            raise RuntimeError("native release/root/GC/join evidence failed:\n"+output[-5000:])
    if runtime.name == "disvm-editor" and not re.search(
            r"main_dis_tls=0 worker_bound=1 worker_released=1 result="+str(expected), output):
        raise RuntimeError("UI/Dis owner retirement failed:\n"+output[-5000:])


# Exercise dispatch, root-boundary and polling variants plus unwinding after
# audited callbacks. Timed sleeps are not used as a performance threshold.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    for mode in ("--jit", "--no-jit"):
        for flags in (("--gc-inline",), ("--gc-worker",),
                      ("--gc-worker", "--gc-locked-poll")):
            for quantum in (1, 2000):
                execute(runtime,module,mode,flags,quantum)
        execute(runtime,module,mode,("--gc-worker",),1,limited=True)
    print("PASS native wait: 12 managed-root/scalar runs, 2 managed-error joins; "
          "inline/atomic/locked, JIT/interpreter, real GC during release")


if __name__ == "__main__":
    main()

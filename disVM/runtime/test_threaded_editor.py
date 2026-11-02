"""Execute real managed Dis applications with independent UI/Dis native owners."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile


# Run one complete process. Status/roles come from checked native lifecycle
# state, never a fabricated Python thread or a substitute interpreter.
def invoke(runtime, module, font, mode, extra=(), expected=0):
    with tempfile.TemporaryDirectory(prefix="threaded Dis owner ") as directory:
        environment = dict(os.environ, DVM_EDITOR_THREAD_STATS="1", TMPDIR=directory,
                           SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
        command = [str(runtime), "--threaded-ui", "--stats", mode,
                   "--font", str(font), *extra, str(module)]
        result = subprocess.run(command, cwd=directory, env=environment,
                                text=True, capture_output=True, timeout=45)
        output = result.stdout + result.stderr
        if result.returncode != expected:
            raise RuntimeError(output[-4000:])
        roles = re.search(r"NATIVEOWNERS ui=main dis=worker ui_polls=(\d+) "
                          r"main_dis_tls=0 worker_bound=1 worker_released=1 result=(\d+)", output)
        if not roles or int(roles[1]) < 2 or int(roles[2]) != expected:
            raise RuntimeError("missing actual owner/TLS retirement evidence:\n" + output[-4000:])
        if expected == 0:
            counters = re.search(r"instructions=(\d+) native=(\d+) gc_runs=(\d+)", output)
            if ("PASS Dis application completed" not in output or "FAIL" in output or
                    not counters or int(counters[1]) == 0 or int(counters[3]) == 0 or
                    (int(counters[2]) > 0) != (mode == "--jit")):
                raise RuntimeError("real execution/GC/dispatch evidence failed:\n" + output[-4000:])
        return output


# Repeated managed document/channel workloads exercise normal-close races.
# Failures cover module load and UI initialization, both requiring join/TLS exit.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    for mode in ("--jit", "--no-jit"):
        for unused in range(3):
            output = invoke(runtime, module, font, mode)
            if "PASS idecore" not in output:
                raise RuntimeError("document test did not execute")
    for mode in ("--jit", "--no-jit"):
        output = invoke(runtime,module.with_name("threaded-owner.dis"),font,mode)
        if "PASS threaded-owner:" not in output:
            raise RuntimeError("managed channel computation did not execute")
    invoke(runtime, module.with_name("missing-threaded-module.dis"), font, "--jit", expected=1)
    invoke(runtime, module, font.with_name("missing-threaded-font.ttf"), "--jit", expected=1)
    duplicate = subprocess.run([str(runtime), "--threaded-ui", "--sequential-ui", str(module)],
                               text=True, capture_output=True, timeout=10)
    if duplicate.returncode != 2 or "only once" not in duplicate.stderr:
        raise RuntimeError("ambiguous execution mode accepted")
    print("PASS real Dis/UI OS owners: 8 managed interpreter/JIT/GC processes, "
          "2 failure retirements, duplicate refusal, main TLS isolated, worker TLS destroyed")


if __name__ == "__main__":
    main()

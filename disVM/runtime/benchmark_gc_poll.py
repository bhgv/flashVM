"""Compare real complete Dis/JIT execution with atomic and mutex polling."""
from pathlib import Path
import json
import os
import re
import statistics
import subprocess
import sys
import time


# Run the matching native module, including process startup and collector joins.
# Return actual elapsed seconds/counters; timings are observations, never a gate.
def run(runtime, module, locked):
    command = [str(runtime), "--gc-worker", "--jit", "--stats"]
    if locked:
        command.append("--gc-locked-poll")
    command.append(str(module))
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    start = time.perf_counter()
    result = subprocess.run(command, env=environment, text=True,
                            capture_output=True, timeout=60)
    elapsed = time.perf_counter()-start
    output = result.stdout+result.stderr
    counters = re.search(r"instructions=(\d+) native=(\d+) gc_runs=(\d+)", output)
    polls = re.search(r"fast_polls=(\d+) locked_polls=(\d+)", output)
    skips = re.search(r"empty_fast_skips=(\d+)", output)
    if (result.returncode or "PASS Dis application completed" not in output or
            not counters or not polls or not skips or
            (locked and (int(polls[1]) or not int(polls[2]))) or
            (not locked and not int(polls[1]))):
        raise RuntimeError(output[-5000:])
    return dict(seconds=elapsed, instructions=int(counters[1]),
                native=int(counters[2]), gc_passes=int(counters[3]),
                fast_polls=int(polls[1]), locked_polls=int(polls[2]),
                empty_fast_skips=int(skips[1]))


# Alternate five runs per route, require identical application instruction counts
# and emit all samples plus medians. This is whole-process measurement, not an
# isolated opcode timing, UI latency promise or cross-architecture acceptance.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    samples = {"atomic": [], "locked": []}
    for unused in range(5):
        for locked in (False, True):
            samples["locked" if locked else "atomic"].append(run(runtime,module,locked))
    instructions = {sample["instructions"] for group in samples.values() for sample in group}
    if len(instructions) != 1:
        raise RuntimeError("comparison did not execute identical application work")
    print(json.dumps(dict(samples=samples, median_seconds={
        name: statistics.median(sample["seconds"] for sample in group)
        for name, group in samples.items()}), indent=2))


if __name__ == "__main__":
    main()

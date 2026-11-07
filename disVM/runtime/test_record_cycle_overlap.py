"""Check live roots and concurrent record-cycle collection in a real Dis VM."""
import argparse
import os
from pathlib import Path
import re
import subprocess


def check(runtime, module, mode, require_peer, require_snapshot):
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    command = [str(runtime), "--gc-idle-cycles", "--stats", mode, str(module)]
    observations = []
    # A bounded retry allows for scheduler variation in the overlap window.
    for _ in range(8):
        result = subprocess.run(command, env=environment, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                timeout=20, check=True)
        output = result.stdout
        counters = re.search(
            r"record_groups=(\d+) shared_record_groups=(\d+) "
            r"shared_record_groups_with_peer=(\d+)", output)
        if "PASS live group" not in output or not counters or int(counters[1]) < 128:
            raise RuntimeError(output)
        snapshots = re.search(r"shared_record_snapshots_with_peer=(\d+)", output)
        if require_snapshot and snapshots is None:
            raise RuntimeError(f"{mode}: runtime does not report concurrent graph snapshots")
        observations.append(tuple(map(int, counters.groups())))
        if (int(counters[2]) and (not require_peer or int(counters[3]))
                and (not require_snapshot or int(snapshots[1]))):
            print(f"PASS {mode}: live group preserved, groups/shared/with-peer {observations[-1]}")
            return
    raise RuntimeError(f"{mode}: no required overlap after eight runs: {observations}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runtime", type=Path)
    parser.add_argument("module", type=Path)
    parser.add_argument("--require-peer", action="store_true")
    parser.add_argument("--require-snapshot", action="store_true")
    args = parser.parse_args()
    for mode in ("--jit", "--no-jit"):
        check(args.runtime.resolve(), args.module.resolve(), mode,
              args.require_peer or args.require_snapshot, args.require_snapshot)


if __name__ == "__main__":
    main()

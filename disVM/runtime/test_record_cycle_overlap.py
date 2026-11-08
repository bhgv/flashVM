"""Check live roots and concurrent record-cycle collection in a real Dis VM."""
import argparse
import os
from pathlib import Path
import re
import subprocess


def check(runtime, module, mode, require_peer, require_snapshot, require_confirmation, native_spawn, large_cycle):
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    command = [str(runtime), "--gc-idle-cycles", "--stats", mode, str(module)]
    if native_spawn:
        command.insert(1, "--native-spawn")
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
        if "PASS live group" not in output or not counters:
            raise RuntimeError(output)
        snapshots = re.search(r"shared_record_snapshots_with_peer=(\d+)", output)
        if require_snapshot and snapshots is None:
            raise RuntimeError(f"{mode}: runtime does not report concurrent graph snapshots")
        confirmations = re.search(r"shared_record_confirmations=(\d+)", output)
        observations.append((*map(int, counters.groups()), int(snapshots[1]) if snapshots else None))
        if large_cycle:
            oversized = re.search(r"cycle_candidates_oversized=(\d+)", output)
            traced = re.search(r"NATIVEGC .*? reclaimed=(\d+)", output)
            full = re.search(r"cycle_candidates_full=(\d+)", output)
            queue = re.search(r"NATIVECYCLEQUEUE ready=0 reserved=0 capacity=256 peak_capacity=(\d+)", output)
            if (int(counters[2]) >= 33 and oversized and int(oversized[1]) == 0
                    and full and int(full[1]) == 0 and traced and int(traced[1]) < 40
                    and queue and int(queue[1]) > 1024):
                print(f"PASS {mode}: 33 cycles collected by shared GC, queue peaked at {queue[1]}")
                return
            continue
        if (int(counters[1]) >= 64 and int(counters[2]) and (not require_peer or int(counters[3]))
                and (not require_snapshot or int(snapshots[1]))
                and (not require_confirmation or (confirmations and int(confirmations[1])))):
            print(f"PASS {mode}: live group preserved, groups/shared/reclaim-peer/scan-peer {observations[-1]}")
            return
    raise RuntimeError(f"{mode}: no required overlap after eight runs: {observations}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runtime", type=Path)
    parser.add_argument("module", type=Path)
    parser.add_argument("--require-peer", action="store_true")
    parser.add_argument("--require-snapshot", action="store_true")
    parser.add_argument("--require-confirmation", action="store_true")
    parser.add_argument("--native-spawn", action="store_true")
    parser.add_argument("--large-cycle", action="store_true")
    args = parser.parse_args()
    failures = []
    for mode in ("--jit", "--no-jit"):
        try:
            check(args.runtime.resolve(), args.module.resolve(), mode,
                  args.require_peer, args.require_snapshot, args.require_confirmation, args.native_spawn,
                  args.large_cycle)
        except RuntimeError as error:
            failures.append(str(error))
    if failures:
        raise SystemExit("\n".join(failures))


if __name__ == "__main__":
    main()

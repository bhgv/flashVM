# Copyright (c) 2026 bhgv
"""EN: Check live roots and shared cycle collection in DisVM.
RU: Проверка живых корней и параллельной сборки циклов в DisVM.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess


def check(runtime, module, mode, require_peer, require_snapshot, require_confirmation,
          native_spawn, large_cycle, live_branch_charge):
    """EN: Run module on runtime in mode and check the selected GC requirements.
    The require flags select overlap evidence. native_spawn selects native guests.
    large_cycle and live_branch_charge select dedicated regression scenarios.
    RU: Запустить module через runtime в режиме mode и проверить требования к GC.
    Флаги require задают проверки перекрытия, native_spawn включает нативные потоки.
    large_cycle и live_branch_charge выбирают отдельные сценарии регрессий.
    """
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
        if live_branch_charge:
            retained = re.search(r"NATIVERCPASSES .*? groups=(\d+)", output)
            arrays = re.search(r"NATIVEARRAYRC shared=\d+ with_peer=\d+ graphs=(\d+)", output)
            if (int(counters[2]) >= 1 and retained and int(retained[1]) == 0
                    and arrays and int(arrays[1]) >= 1):
                print(f"PASS {mode}: live array preserved, both containers freed by shared GC")
                return
            continue
        if large_cycle:
            oversized = re.search(r"cycle_candidates_oversized=(\d+)", output)
            traced = re.search(r"NATIVEGC .*? reclaimed=(\d+)", output)
            full = re.search(r"cycle_candidates_full=(\d+)", output)
            queue = re.search(r"NATIVECYCLEQUEUE ready=0 reserved=0 capacity=256 peak_capacity=(\d+)", output)
            duke = re.search(r"NATIVEDUKEGC promoted=(\d+) peak_fields=(\d+) resumed=(\d+) adopted_pins=(\d+)", output)
            if (int(counters[2]) >= 33 and oversized and int(oversized[1]) > 0
                    and duke and int(duke[1]) >= 1 and int(duke[2]) >= 2300 and int(duke[3]) > 0 and int(duke[4]) >= 2299
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
    """EN: Run both execution modes. RU: Проверить оба режима исполнения."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runtime", type=Path)
    parser.add_argument("module", type=Path)
    parser.add_argument("--require-peer", action="store_true")
    parser.add_argument("--require-snapshot", action="store_true")
    parser.add_argument("--require-confirmation", action="store_true")
    parser.add_argument("--native-spawn", action="store_true")
    parser.add_argument("--large-cycle", action="store_true")
    parser.add_argument("--live-branch-charge", action="store_true")
    args = parser.parse_args()
    failures = []
    for mode in ("--jit", "--no-jit"):
        try:
            check(args.runtime.resolve(), args.module.resolve(), mode,
                  args.require_peer, args.require_snapshot, args.require_confirmation, args.native_spawn,
                  args.large_cycle, args.live_branch_charge)
        except RuntimeError as error:
            failures.append(str(error))
    if failures:
        raise SystemExit("\n".join(failures))


if __name__ == "__main__":
    main()

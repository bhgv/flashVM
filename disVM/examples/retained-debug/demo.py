#!/usr/bin/env python3
# Copyright (c) 2026 bhgv
# EN: Demonstrate retained fault, independent peer progress and on-request GC census.
# RU: Показать retained fault, independent peer progress и GC census по запросу.
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "runtime"))
from debugger.client import PipeDebugger


# EN/RU: Wait for acknowledged retention, not merely an accepted pause request.
# Ждать acknowledged retention, а не только accepted pause request.
def wait(session, pid, state):
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        result = session.command("status " + str(pid))
        if result.get("state") == state and result.get("retained"):
            return result
        time.sleep(0.01)
    raise RuntimeError("debug stop was not acknowledged")


# EN: Inspect real targets, then explicitly release all resources. The generic
# interactive debugger detaches without termination; this demo intentionally joins.
# RU: Исследовать actual targets, затем явно освободить resources. Общий interactive
# debugger отключается без termination; этот demo намеренно делает joins.
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("runtime", type=Path)
    parser.add_argument("image", type=Path)
    parser.add_argument("--jit", action="store_true")
    args = parser.parse_args()
    session = PipeDebugger(
        args.runtime,
        ["--stats", "--jit" if args.jit else "--no-jit", str(args.image)],
        stdout=subprocess.PIPE,
        text=True,
    )
    ids = []
    try:
        line = session.process.stdout.readline()
        print(line, end="", flush=True)
        match = re.search(
            r"main=(\d+) loop=(\d+) fault=(\d+) native=(\d+) caught=(\d+) builtin=(\d+)", line
        )
        if not match:
            raise RuntimeError("demo targets were not published")
        ids = list(map(int, match.groups()))
        fault = ids[2]
        print(json.dumps(wait(session, fault, "faulted")), flush=True)
        for command in (
            f"registers {fault}",
            f"operation {fault}",
            f"backtrace {fault} 0",
            f"backtrace {fault} 1",
            f"census {fault}",
        ):
            print(command, json.dumps(session.command(command)), flush=True)
        for pid in (ids[1], ids[3], ids[0]):
            session.command("pause " + str(pid))
            print(json.dumps(wait(session, pid, "paused")), flush=True)
        print("PASS retained debugger example", flush=True)
    finally:
        for pid in ids[1:] + ids[:1]:
            if session.process.poll() is None:
                session.command("terminate " + str(pid))
        session.close()
        try:
            session.process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            session.process.kill()
            session.process.wait()
            raise
    if session.process.returncode:
        raise RuntimeError("debug example failed to retire normally")


if __name__ == "__main__":
    main()

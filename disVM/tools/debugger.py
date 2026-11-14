#!/usr/bin/env python3
# Copyright (c) 2026 bhgv
# EN: Launch a VM with independent debug pipes and issue line-oriented commands.
# RU: Запустить VM с independent debug pipes и отправлять line-oriented commands.
import argparse
import json
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "runtime"))
from debugger.client import PipeDebugger, SocketDebugger


# EN: Read debugger commands from stdin; application input is explicitly closed.
# EOF detaches without releasing frozen continuations; print PID for native attach.
# RU: Читать debug commands из stdin; application input явно закрыт.
# EOF отсоединяет без release frozen continuations; вывести PID для native attach.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--socket", type=Path, help="attach to existing --debug endpoint")
    parser.add_argument("runtime", type=Path, nargs="?")
    parser.add_argument("arguments", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if args.socket:
        if args.runtime or args.arguments:
            parser.error("--socket attaches without runtime arguments")
        session = SocketDebugger(args.socket)
    else:
        if not args.runtime:
            parser.error("runtime is required when launching")
        session = PipeDebugger(args.runtime, args.arguments, stdin=subprocess.DEVNULL)
        print("Native VM PID:", session.process.pid, file=sys.stderr, flush=True)
    try:
        for command in sys.stdin:
            command = command.strip()
            if command:
                print(json.dumps(session.command(command), ensure_ascii=False), flush=True)
    finally:
        session.close()
        if session.process and session.process.poll() is None:
            print(
                "Debugger detached; retained targets remain in VM PID",
                session.process.pid,
                file=sys.stderr,
            )


if __name__ == "__main__":
    main()

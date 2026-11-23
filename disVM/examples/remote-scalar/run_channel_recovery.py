# Copyright (c) 2026 bhgv
# EN: Break exact TCP waits between two DisVMs and verify fresh mounts.
# RU: Оборвать заданные ожидания TCP между двумя DisVM и проверить новые маунты.
import argparse
from contextlib import suppress
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import socket
import subprocess
import tempfile
import time

from run_chain import DISVM, EXAMPLE, accept, compile_images, connect, delegate, listen


# EN: Wait only until the caller verifies its checkpoint, then break the live carrier.
# RU: Дождаться подтверждённой контрольной точки клиента и оборвать живое соединение.
def break_connection(root, carrier, caller):
    deadline = time.monotonic() + 10
    while not (root / "break-now").exists():
        if caller.done():
            return
        if time.monotonic() >= deadline:
            raise RuntimeError("caller did not reach the break checkpoint")
        time.sleep(0.005)
    carrier.shutdown(socket.SHUT_RDWR)


# EN: Run one solo provider and one caller, requiring both to exit successfully.
# RU: Запустить одного провайдера и одного клиента и проверить завершение обоих.
def exercise(runtime, provider, owner, caller, mode, case, expected):
    (caller / "case").write_text(case)
    (caller / "break-now").unlink(missing_ok=True)
    module = "channel-lifetime.dis"
    with listen(("127.0.0.1", 0)) as listener, ThreadPoolExecutor(max_workers=2) as executor:
        with connect(listener.getsockname()) as client, accept(listener) as server, client.dup() as breaker:
            options = ["--", "--native-spawn", "--stats", mode]
            serving = executor.submit(delegate,
                [provider, "--styx-library-serve-fd", str(server.fileno()),
                 "--export", module, module, "waitvalue", "--export", module, module, "buffered",
                 *options, "dispatcher.dis"], (server,), owner)
            calling = executor.submit(delegate,
                [runtime, "--styx-mount-fd", str(client.fileno()), "--at", "/transport",
                 *options, "client.dis"], (client,), caller)
            try:
                if case in ("call", "channel"):
                    break_connection(caller, breaker, calling)
                stdout, stderr = calling.result()
            except (OSError, RuntimeError, subprocess.SubprocessError):
                with suppress(OSError):
                    breaker.shutdown(socket.SHUT_RDWR)
                raise
            _, owner_stats = serving.result()
            for log in (stderr, owner_stats):
                if not any("MANAGEDOWNERS " in line and " live=0 pending=0 tickets=0 " in line
                           for line in log.splitlines()):
                    raise RuntimeError("managed owners survived connection retirement\n" + log)
            if expected not in stdout:
                raise RuntimeError(stdout + stderr)
            print(stdout, end="")


# EN: Compile once, then give each independent mount a fresh TCP carrier and VM pair.
# RU: Собрать один раз, затем для каждого маунта создать свою TCP-сессию и пару VM.
def main():
    parser = argparse.ArgumentParser(description="Exercise link loss and a fresh Styx mount.")
    parser.add_argument("--runtime", required=True, type=lambda path: Path(path).resolve())
    parser.add_argument("--limbo", required=True, type=lambda path: Path(path).resolve())
    parser.add_argument("--legacy-runtime", type=lambda path: Path(path).resolve())
    parser.add_argument("--jit", action="store_const", const="--jit", default="--no-jit", dest="mode")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="disvm-channel-recovery-") as scratch:
        root = Path(scratch)
        owner, caller = root / "owner", root / "caller"
        owner.mkdir()
        caller.mkdir()
        compile_images(args.limbo, owner, (
            (DISVM / "limbo/scalarowner.b", "dispatcher.dis"),
            (EXAMPLE / "styx-channel-lifetime-owner.b", "channel-lifetime.dis"),
        ))
        compile_images(args.limbo, caller, ((EXAMPLE / "styx-channel-recovery-client.b", "client.dis"),))
        for case, expected in (("call", "unknown call outcome"),
                               ("channel", "interrupted channel wait"),
                               ("fresh", "fresh mount with working channel")):
            exercise(args.runtime, args.runtime, owner, caller, args.mode, case, "PASS " + expected)
        if args.legacy_runtime:
            exercise(args.runtime, args.legacy_runtime, owner, caller, args.mode,
                     "legacy", "PASS incompatible Styx profile at load")


if __name__ == "__main__":
    main()

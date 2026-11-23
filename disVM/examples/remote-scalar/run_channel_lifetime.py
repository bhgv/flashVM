# Copyright (c) 2026 bhgv
# EN: Run mixed ALT and returned channel fixtures on two TCP-connected DisVMs.
# RU: Запуск проверок смешанного ALT и возврата каналов на двух DisVM через TCP.
import argparse
from contextlib import ExitStack
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import tempfile

from run_chain import DISVM, EXAMPLE, accept, compile_images, connect, delegate, listen


CASES = {
    "mixed": ("styx-channel-mixed-client.b", "PASS blocking mixed channel ALT"),
    "result": ("styx-channel-result-client.b", "PASS provider-owned returned channels"),
    "recursive": ("styx-channel-recursive-client.b", "PASS nested grants across both UniBus directions"),
}  # Caller's source image and required end-to-end marker.
EXPORTS = ("mixed", "buffered", "worker", "relay", "relayput", "nested",
           "relaypair", "grantworker")  # Common provider interface.


# EN: Compile provider and client from case, run them on one TCP connection,
# and require the chosen marker and successful shutdown of both runtimes.
# RU: Собрать провайдер и клиент для case, запустить их через одно TCP-соединение,
# проверить отметку и успешное завершение обеих VM.
def exercise(runtime, compiler, root, mode, case):
    owner = root / "owner"
    caller = root / "caller"
    owner.mkdir()
    caller.mkdir()
    module = "channel-lifetime.dis"
    source, marker = CASES[case]
    owner_source = "styx-channel-recursive-owner.b" if case == "recursive" else "styx-channel-lifetime-owner.b"
    names = ("open",) if case == "recursive" else EXPORTS
    compile_images(compiler, owner, (
        (DISVM / "limbo/scalarowner.b", "dispatcher.dis"),
        (EXAMPLE / owner_source, module),
    ))
    compile_images(compiler, caller, ((EXAMPLE / source, "client.dis"),))
    exports = [arg for name in names for arg in ("--export", module, module, name)]
    with listen(("127.0.0.1", 0)) as listener, ThreadPoolExecutor(max_workers=1) as worker:
        with connect(listener.getsockname()) as forward, accept(listener) as reverse:
            server = worker.submit(delegate,
                [runtime, "--styx-library-serve-fd", str(reverse.fileno()),
                 *exports, "--", "--native-spawn", mode, "dispatcher.dis"],
                (reverse,), owner)
            stdout, stderr = delegate(
                [runtime, "--styx-mount-fd", str(forward.fileno()), "--at", "/transport",
                 "--", "--native-spawn", mode, "client.dis"], (forward,), caller)
            server.result()
    if marker not in stdout:
        raise RuntimeError(stdout + stderr)
    print(stdout, end="")


# EN: Use independent TCP sessions A → B and B → C. Verify both providers exit.
# RU: Использовать независимые сессии TCP A → B и B → C и проверить выход обоих провайдеров.
def exercise_bridge(runtime, compiler, root, mode):
    a, b, c = (root / role for role in ("a", "b", "c"))
    for role in (a, b, c):
        role.mkdir()
    dispatcher = (DISVM / "limbo/scalarowner.b", "dispatcher.dis")
    compile_images(compiler, a, ((EXAMPLE / "styx-channel-bridge-client.b", "client.dis"),))
    compile_images(compiler, b, (dispatcher, (EXAMPLE / "styx-channel-bridge-owner.b", "bridge.dis")))
    leaf = "channel-lifetime.dis"
    compile_images(compiler, c, (dispatcher, (EXAMPLE / "styx-channel-lifetime-owner.b", leaf)))
    with ExitStack() as sockets, ThreadPoolExecutor(max_workers=2) as workers:
        b_listener = sockets.enter_context(listen(("127.0.0.1", 0)))
        c_listener = sockets.enter_context(listen(("127.0.0.1", 0)))
        ab = sockets.enter_context(connect(b_listener.getsockname()))
        ba = sockets.enter_context(accept(b_listener))
        bc = sockets.enter_context(connect(c_listener.getsockname()))
        cb = sockets.enter_context(accept(c_listener))
        vm = ["--", "--native-spawn", mode]
        server_c = workers.submit(delegate,
            [runtime, "--styx-library-serve-fd", str(cb.fileno()),
             "--export", leaf, leaf, "relay", "--export", leaf, leaf, "worker",
             "--export", leaf, leaf, "waitvalue",
             *vm, "dispatcher.dis"], (cb,), c)
        server_b = workers.submit(delegate,
            [runtime, "--styx-library-serve-fd", str(ba.fileno()),
             "--export", "bridge.dis", "bridge.dis", "roundtrip",
             "--export", "bridge.dis", "bridge.dis", "worker",
             "--export", "bridge.dis", "bridge.dis", "waitvalue",
             "--library-bidir-fd", str(bc.fileno()), "--library-at", "/dependency",
             *vm, "dispatcher.dis"], (ba, bc), b)
        stdout, stderr = delegate(
            [runtime, "--styx-mount-fd", str(ab.fileno()), "--at", "/transport",
             *vm, "client.dis"], (ab,), a)
        server_b.result()
        server_c.result()
    if "PASS live channels across three DisVMs" not in stdout:
        raise RuntimeError(stdout + stderr)
    print(stdout, end="")


# EN: Resolve CLI tool paths and put the selected scenario in a temporary directory.
# RU: Разрешить пути к инструментам и запустить выбранный сценарий во временном каталоге.
def main():
    parser = argparse.ArgumentParser(description="Run one TCP channel lifetime fixture.")
    parser.add_argument("case", choices=(*CASES, "bridge"))
    parser.add_argument("--runtime", required=True, type=lambda path: Path(path).resolve())
    parser.add_argument("--limbo", required=True, type=lambda path: Path(path).resolve())
    parser.add_argument("--jit", action="store_const", const="--jit", default="--no-jit", dest="mode")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="disvm-channel-lifetime-") as scratch:
        if args.case == "bridge":
            exercise_bridge(args.runtime, args.limbo, Path(scratch), args.mode)
        else:
            exercise(args.runtime, args.limbo, Path(scratch), args.mode, args.case)


if __name__ == "__main__":
    main()

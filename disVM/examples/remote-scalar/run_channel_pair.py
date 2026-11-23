# Copyright (c) 2026 bhgv
# EN: Connect a channel-pair client and provider over one TCP socket.
# RU: Связать клиента и провайдера примера сопоставления через один сокет TCP.
import argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import tempfile

from run_chain import DISVM, EXAMPLE, accept, compile_images, connect, delegate, listen


# EN: Compile the two roles below root and run both with the given runtime and mode.
# Verify the caller's marker after the provider has exited. The caller owns root.
# RU: Собрать обе роли в root и запустить их через runtime в режиме mode.
# Проверить сообщение клиента после выхода провайдера. root принадлежит вызывающему.
def exercise(runtime, compiler, root, mode):
    owner = root / "owner"
    caller = root / "caller"
    owner.mkdir()
    caller.mkdir()
    compile_images(compiler, owner, (
        (DISVM / "limbo/scalarowner.b", "dispatcher.dis"),
        (EXAMPLE / "styx-channel-pair-owner.b", "pair.dis"),
    ))
    compile_images(compiler, caller, ((EXAMPLE / "styx-channel-pair-client.b", "client.dis"),))
    with listen(("127.0.0.1", 0)) as listener, ThreadPoolExecutor(max_workers=1) as worker:
        with connect(listener.getsockname()) as forward, accept(listener) as reverse:
            server = worker.submit(delegate,
                [runtime, "--styx-library-serve-fd", str(reverse.fileno()),
                 "--export", "pair.dis", "pair.dis", "integer",
                 "--export", "pair.dis", "pair.dis", "managed", "--", "--native-spawn", mode, "dispatcher.dis"],
                (reverse,), owner)
            try:
                stdout, stderr = delegate(
                    [runtime, "--styx-mount-fd", str(forward.fileno()), "--at", "/transport",
                     "--", "--native-spawn", mode, "client.dis"], (forward,), caller)
            except RuntimeError as client_error:
                try:
                    provider_stdout, provider_stderr = server.result()
                except RuntimeError as provider_error:
                    raise RuntimeError(f"{client_error}\n{provider_error}") from client_error
                raise RuntimeError(f"{client_error}\n{provider_stdout}{provider_stderr}") from client_error
            server.result()
    if "PASS same-bus proxy channel rendezvous" not in stdout:
        raise RuntimeError(stdout + stderr)
    print(stdout, end="")


# EN: Parse binary paths and mode, run the fixture in a fresh temporary directory.
# RU: Прочитать пути и режим, запустить пример в новом временном каталоге.
def main():
    parser = argparse.ArgumentParser(description="Run a same-bus UniBus channel rendezvous.")
    parser.add_argument("--runtime", required=True, type=lambda path: Path(path).resolve())
    parser.add_argument("--limbo", required=True, type=lambda path: Path(path).resolve())
    parser.add_argument("--jit", action="store_const", const="--jit", default="--no-jit", dest="mode")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="disvm-channel-pair-") as scratch:
        exercise(args.runtime, args.limbo, Path(scratch), args.mode)


if __name__ == "__main__":
    main()

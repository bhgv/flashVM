# Copyright (c) 2026 bhgv
# EN: Connect the channel example's caller and provider over one TCP socket.
# RU: Связать клиента и провайдера примера каналов одним сокетом TCP.
import argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import tempfile

from run_chain import DISVM, EXAMPLE, accept, compile_images, connect, delegate, listen


# EN: Run both images in separate directories, then verify the caller's marker.
# RU: Запустить образы в отдельных каталогах и проверить маркер клиента.
def exercise(runtime, compiler, root, mode):
    owner = root / "owner"
    caller = root / "caller"
    owner.mkdir()
    caller.mkdir()
    compile_images(compiler, owner, (
        (DISVM / "limbo/scalarowner.b", "dispatcher.dis"),
        (EXAMPLE / "styx-channel-owner.b", "channel.dis"),
    ))
    compile_images(compiler, caller, ((EXAMPLE / "styx-channel-client.b", "client.dis"),))
    with listen(("127.0.0.1", 0)) as listener, ThreadPoolExecutor(max_workers=1) as worker:
        with connect(listener.getsockname()) as forward, accept(listener) as reverse:
            server = worker.submit(delegate,
                [runtime, "--styx-library-serve-fd", str(reverse.fileno()),
                 "--export", "channel.dis", "channel.dis", "exchange",
                 "--export", "channel.dis", "channel.dis", "typed",
                 "--export", "channel.dis", "channel.dis", "scalars",
                 "--export", "channel.dis", "channel.dis", "channelresult",
                 "--export", "channel.dis", "channel.dis", "nested",
                 "--export", "channel.dis", "channel.dis", "choose", "--", "--native-spawn", mode, "dispatcher.dis"],
                (reverse,), owner)
            stdout, stderr = delegate(
                [runtime, "--styx-mount-fd", str(forward.fileno()), "--at", "/transport",
                 "--", "--native-spawn", mode, "client.dis"], (forward,), caller)
            server.result()
    if "PASS automatic Limbo channels over UniBus" not in stdout:
        raise RuntimeError(stdout + stderr)
    print(stdout, end="")


# EN: Select one execution mode and keep build products in a temporary directory.
# RU: Выбрать режим исполнения и хранить сборку во временном каталоге.
def main():
    parser = argparse.ArgumentParser(description="Run a remote Limbo channel over UniBus and Styx.")
    parser.add_argument("--runtime", required=True, type=lambda path: Path(path).resolve())
    parser.add_argument("--limbo", required=True, type=lambda path: Path(path).resolve())
    parser.add_argument("--jit", action="store_const", const="--jit", default="--no-jit", dest="mode")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="disvm-channel-") as scratch:
        exercise(args.runtime, args.limbo, Path(scratch), args.mode)


if __name__ == "__main__":
    main()

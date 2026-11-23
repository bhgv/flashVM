# Copyright (c) 2026 bhgv
# EN: Run mixed ALT and returned channel fixtures on two TCP-connected DisVMs.
# RU: Запуск проверок смешанного ALT и возврата каналов на двух DisVM через TCP.
import argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import tempfile

from run_chain import DISVM, EXAMPLE, accept, compile_images, connect, delegate, listen


CASES = {
    "mixed": ("styx-channel-mixed-client.b", "PASS blocking mixed channel ALT"),
    "result": ("styx-channel-result-client.b", "PASS provider-owned returned channels"),
}  # Caller's source image and required end-to-end marker.
EXPORTS = ("mixed", "buffered", "worker", "relay", "relayput", "nested")  # Common provider interface.


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
    compile_images(compiler, owner, (
        (DISVM / "limbo/scalarowner.b", "dispatcher.dis"),
        (EXAMPLE / "styx-channel-lifetime-owner.b", module),
    ))
    compile_images(compiler, caller, ((EXAMPLE / source, "client.dis"),))
    exports = [arg for name in EXPORTS for arg in ("--export", module, module, name)]
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


# EN: Resolve CLI tool paths and put the selected scenario in a temporary directory.
# RU: Разрешить пути к инструментам и запустить выбранный сценарий во временном каталоге.
def main():
    parser = argparse.ArgumentParser(description="Run one TCP channel lifetime fixture.")
    parser.add_argument("case", choices=CASES)
    parser.add_argument("--runtime", required=True, type=lambda path: Path(path).resolve())
    parser.add_argument("--limbo", required=True, type=lambda path: Path(path).resolve())
    parser.add_argument("--jit", action="store_const", const="--jit", default="--no-jit", dest="mode")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="disvm-channel-lifetime-") as scratch:
        exercise(args.runtime, args.limbo, Path(scratch), args.mode, args.case)


if __name__ == "__main__":
    main()

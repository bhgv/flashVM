# Copyright (c) 2026 bhgv
"""Run two stock FlashVMs on one StyxRPC stream. / Запустить две FlashVM на одном потоке StyxRPC."""

import argparse
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory
import time

ROOT = Path(__file__).resolve().parents[2]  # Repository root / Корень репозитория.
sys.path.insert(0, str(ROOT / "lib/styxrpc/tests/flashvm"))
from run import stream_pair


# EN: Compile the role's AS source with project AS2, FIR and linker.
# RU: Собрать исходник роли через AS2, FIR и линкер проекта.
def compile_role(args, directory, role):
    name = f"peer-{role}"
    source = ROOT / "flashVM/tests/styxrpc" / f"{name}.as"
    fir = directory / f"{name}.fir"
    obj = directory / f"{name}.swo"
    swf = directory / f"{name}.swf"
    for command in ((args.as2, source, fir), (args.fir / "fir1", "check", fir),
                    (args.fir / "fir1", "object", fir, obj)):
        subprocess.run(command, check=True, timeout=20)
    objects = [obj]
    if role == "caller":
        report = directory / "peer-report.swo"
        subprocess.run((args.fir / "fir1", "object", source.with_name("peer-report.fir"), report),
                       check=True, timeout=20)
        objects.append(report)
    subprocess.run((args.swl, "link", swf, *objects), check=True, timeout=20)
    return swf


# EN: Spawn both owners, await the caller's exact result, then check the
# provider retires after its connection closes. No wire relay is involved.
# RU: Запустить обоих владельцев, проверить ответ и завершение провайдера
# при разрыве связи. Поток идёт напрямую между плеерами.
def exchange(args, directory, caller_swf, provider_swf, mode, reverse):
    caller_lane, provider_lane = ("reverse", "forward") if reverse else ("forward", "reverse")
    caller_socket, provider_socket = stream_pair(args.tcp)
    caller_log = directory / "caller.log"
    provider_log = directory / "provider.log"
    with caller_socket, provider_socket, caller_log.open("w") as c_log, provider_log.open("w") as p_log:
        processes = []
        try:
            for endpoint, lane, swf, log in ((caller_socket, caller_lane, caller_swf, c_log),
                                             (provider_socket, provider_lane, provider_swf, p_log)):
                fd = endpoint.fileno()
                command = (args.player, "--fd", str(fd), "--peer", lane, "--movie", swf, mode)
                processes.append(subprocess.Popen(command, cwd=directory, pass_fds=(fd,), stdout=log,
                                                  stderr=subprocess.STDOUT))
                endpoint.close()
            caller, provider = processes
            cutoff = time.monotonic() + 15
            while "PASS FlashVM peer replied 12" not in caller_log.read_text():
                if caller.poll() is not None or provider.poll() is not None or time.monotonic() >= cutoff:
                    raise RuntimeError(f"{caller_lane} {mode}\ncaller: {caller_log.read_text()}\n"
                                       f"provider: {provider_log.read_text()}")
                time.sleep(0.02)
            caller.terminate()
            caller.wait(timeout=5)
            if provider.wait(timeout=5):
                raise RuntimeError(f"provider failed on disconnect: {provider_log.read_text()}")
        finally:
            for process in processes:
                if process.poll() is None:
                    process.kill()
                    process.wait()
    print(f"PASS {mode} {caller_lane} caller, exact Int32 and peer retirement")


# EN: Exercise both service lane assignments in each execution mode.
# RU: Проверить оба назначения серверных каналов в каждом режиме.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("player", "as2", "fir", "swl"):
        parser.add_argument("--" + name, type=lambda path: Path(path).resolve(), required=True)
    parser.add_argument("--tcp", action="store_true", help="use a direct TCP loopback connection")
    args = parser.parse_args()
    with TemporaryDirectory(prefix="flashvm-peer-") as scratch:
        directory = Path(scratch)
        caller = compile_role(args, directory, "caller")
        provider = compile_role(args, directory, "provider")
        for mode in ("--no-jit", "--jit"):
            for reverse in (False, True):
                exchange(args, directory, caller, provider, mode, reverse)


if __name__ == "__main__":
    main()

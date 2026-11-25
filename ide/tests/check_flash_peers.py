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
from run import listener_port, stream_pair


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


# EN: Start provider then caller using either their TCP CLI or two delegated FDs.
# RU: Запустить провайдера и вызывающего командами TCP либо с выданными FD.
def start_pair(args, directory, caller_options, provider_options, caller_log, provider_log, processes):
    if args.tcp:
        host = "::1" if getattr(args, "ipv6", False) else "127.0.0.1"
        provider = subprocess.Popen((args.player, "--listen", host, "0", *provider_options), cwd=directory,
                                    stdout=subprocess.PIPE, stderr=provider_log, text=True)
        processes.append(provider)
        port = listener_port(provider.stdout)
        caller = subprocess.Popen((args.player, "--connect", host, port, *caller_options), cwd=directory,
                                  stdout=caller_log, stderr=subprocess.STDOUT)
        processes.append(caller)
        return caller, provider
    caller_socket, provider_socket = stream_pair()
    with caller_socket, provider_socket:
        provider_fd = provider_socket.fileno()
        provider = subprocess.Popen((args.player, "--fd", str(provider_fd), *provider_options), cwd=directory,
                                    pass_fds=(provider_fd,), stdout=provider_log, stderr=subprocess.STDOUT)
        processes.append(provider)
        caller_fd = caller_socket.fileno()
        caller = subprocess.Popen((args.player, "--fd", str(caller_fd), *caller_options), cwd=directory,
                                  pass_fds=(caller_fd,), stdout=caller_log, stderr=subprocess.STDOUT)
        processes.append(caller)
        return caller, provider


# EN: Await the exact result, then require the provider to retire after disconnect.
# RU: Дождаться точного ответа и завершения провайдера после разрыва связи.
def exchange(args, directory, caller_swf, provider_swf, mode, reverse):
    caller_lane, provider_lane = ("reverse", "forward") if reverse else ("forward", "reverse")
    caller_options = ("--peer", caller_lane, "--movie", caller_swf, mode)
    provider_options = ("--peer", provider_lane, "--movie", provider_swf, mode)
    caller_log = directory / "caller.log"
    provider_log = directory / "provider.log"
    with caller_log.open("w") as c_log, provider_log.open("w") as p_log:
        processes = []
        try:
            caller, provider = start_pair(args, directory, caller_options, provider_options, c_log, p_log, processes)
            cutoff = time.monotonic() + 15
            while "PASS FlashVM peer replied 12" not in caller_log.read_text():
                if caller.poll() is not None or provider.poll() is not None or time.monotonic() >= cutoff:
                    raise RuntimeError(f"{caller_lane} {mode}\ncaller: {caller_log.read_text()}\n"
                                       f"provider: {provider_log.read_text()}")
                time.sleep(0.02)
            caller.terminate()
            caller.wait(timeout=5)
            output, _ = provider.communicate(timeout=5)
            if provider.returncode:
                raise RuntimeError(f"provider failed on disconnect: {provider_log.read_text()}\n{output or ''}")
        finally:
            for process in processes:
                if process.poll() is None:
                    process.kill()
                process.communicate()
    print(f"PASS {mode} {caller_lane} caller, exact Int32 and peer retirement")


# EN: Exercise both service lane assignments in each execution mode.
# RU: Проверить оба назначения серверных каналов в каждом режиме.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("player", "as2", "fir", "swl"):
        parser.add_argument("--" + name, type=lambda path: Path(path).resolve(), required=True)
    parser.add_argument("--tcp", action="store_true", help="use a direct TCP loopback connection")
    parser.add_argument("--ipv6", action="store_true", help="use ::1 for the TCP loopback test")
    args = parser.parse_args()
    if args.ipv6 and not args.tcp:
        parser.error("--ipv6 requires --tcp")
    with TemporaryDirectory(prefix="flashvm-peer-") as scratch:
        directory = Path(scratch)
        caller = compile_role(args, directory, "caller")
        provider = compile_role(args, directory, "provider")
        for mode in ("--no-jit", "--jit"):
            for reverse in (False, True):
                exchange(args, directory, caller, provider, mode, reverse)


if __name__ == "__main__":
    main()

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
from run import compile_image, listener_port, paired_exchange, stream_pair


# EN: Compile one named AS unit through the project AS2 and FIR compilers.
# RU: Собрать именованный модуль AS компиляторами проекта AS2 и FIR.
def compile_unit(args, directory, name, source=None):
    if source is None:
        source = ROOT / "flashVM/tests/styxrpc" / f"{name}.as"
    fir = directory / f"{name}.fir"
    obj = directory / f"{name}.swo"
    for command in ((args.as2, source, fir), (args.fir / "fir1", "check", fir),
                    (args.fir / "fir1", "object", fir, obj)):
        subprocess.run(command, check=True, timeout=20)
    return obj


# EN: Link the role's entry and callback units into one stand-alone movie.
# RU: Слинковать точку входа роли и колбэки в самостоятельный фильм.
def compile_role(args, directory, role):
    swf = directory / f"peer-{role}.swf"
    objects = [compile_unit(args, directory, f"peer-{role}")]
    if role in ("caller", "dis-caller", "ref", "exec-caller", "named-caller", "module-refs-caller", "messages-caller",
                "asinc", "asinc-chan-caller", "asinc-load-caller", "asinc-scope-caller"):
        report = directory / "peer-report.swo"
        source = ROOT / "flashVM/tests/styxrpc/peer-report.fir"
        subprocess.run((args.fir / "fir1", "object", source, report), check=True, timeout=20)
        objects.append(report)
    if role in ("asinc-load-caller", "asinc-scope-caller"):
        worker = role.rsplit("-", 1)[0] + "-worker"
        objects.append(compile_unit(args, directory, "peer-" + worker))
    if role == "asinc":
        objects.append(compile_unit(args, directory, "peer-asinc-frame"))
    if role in ("asinc-chan-caller", "asinc-chan-provider"):
        source = ROOT / "lib/styxrpc/tests/flashvm/asinc-chan_worker.as"
        objects.append(compile_unit(args, directory, "asinc-chan-worker", source))
    if role == "caller":
        objects.extend((compile_unit(args, directory, "peer-credit"),
                        compile_unit(args, directory, "peer-frame")))
    if role == "ref-provider":
        objects.append(compile_unit(args, directory, "peer-ref-release"))
    if role == "module-refs-provider":
        objects.append(compile_unit(args, directory, "peer-module-refs-start"))
    if role == "messages-provider":
        objects.extend(compile_unit(args, directory, "peer-messages-" + name)
                       for name in ("handlers", "second", "ready"))
    if role == "messages-caller":
        objects.extend(compile_unit(args, directory, "peer-messages-" + name)
                       for name in ("first-reply", "second-reply"))
    subprocess.run((args.swl, "link", swf, *objects), check=True, timeout=20)
    return swf


# EN: Build the same AS provider used by the Limbo-to-Flash executable-node test.
# RU: Собрать тот же AS-провайдер, который используется в проверке Limbo → FlashVM.
def compile_exec_provider(args, directory):
    sources = ROOT / "lib/styxrpc/tests/flashvm"
    units = ("execute_pair", "execute_apply", "execute_refuse")
    objects = [compile_unit(args, directory, name, sources / f"{name}.as") for name in units]
    swf = directory / "peer-exec-provider.swf"
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
def exchange(args, directory, caller_swf, provider_swf, mode, reverse, label="checked imports, variable, UniBus"):
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
            while "PASS StyxRPC script contract" not in caller_log.read_text():
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
    print(f"PASS {mode} {caller_lane} caller, {label} and peer retirement")


# EN: Use stock DisVM to load the AS movie and call its image through the mounted session.
# RU: Загрузить фильм AS штатной DisVM и вызвать его образ через смонтированную сессию.
def dis_calls_flash(args, directory, mode):
    paired_exchange(directory, lambda connection: [args.player, *connection, mode],
                    lambda connection: [args.runtime, *connection, "--at", "/rpc", "--", "--native-spawn",
                                        mode, "peer-consumer.dis"],
                    "PASS DisVM called the AS module and updated its live variable", tcp=args.tcp)


# EN: Run movie and mode against the Dis provider; use caller_label in diagnostics.
# RU: Запустить movie в режиме mode с провайдером Dis; использовать caller_label в диагностике.
def flash_calls_dis(args, directory, movie, mode, caller_label="movie", image="limbo.dis", method="next"):
    options = ("--export", image, image, method, "--", "--native-spawn", mode, "dispatcher.dis")
    # EN/RU: Exercise the stock listening CLI, including the reverse attach.
    # Проверить штатный TCP-слушатель вместе с встречным подключением.
    with subprocess.Popen((args.runtime, "--styx-library-serve-tcp", "127.0.0.1", "0", *options),
                          cwd=directory, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True) as provider:
        try:
            port = listener_port(provider.stdout)
            command = (args.player, "--connect", "127.0.0.1", port, "--peer", "reverse", "--movie", movie, mode)
            with (directory / "dis-caller.log").open("w+") as log, subprocess.Popen(
                    command, cwd=directory, stdout=log, stderr=subprocess.STDOUT) as caller:
                try:
                    cutoff = time.monotonic() + 10
                    while True:
                        log.seek(0)
                        output = log.read()
                        if "PASS StyxRPC script contract" in output:
                            break
                        if caller.poll() is not None or provider.poll() is not None or time.monotonic() >= cutoff:
                            raise RuntimeError(f"FlashVM {caller_label} call to DisVM {mode}: {output}")
                        time.sleep(0.02)
                finally:
                    if caller.poll() is None:
                        caller.terminate()
            output, _ = provider.communicate(timeout=5)
            if provider.returncode:
                raise RuntimeError(f"DisVM provider failed: {output}")
        finally:
            if provider.poll() is None:
                provider.kill()
    print(f"PASS {mode} FlashVM {caller_label} loaded and called a compiled Dis export")


# EN: Build the Dis export and its resident dispatcher in the test directory.
# RU: Собрать экспорт Dis и его резидентный диспетчер в каталоге проверки.
def compile_dis_provider(args, directory):
    compile_image(args, directory, ROOT / "lib/styxrpc/tests/flashvm/limbo.b", "limbo.dis")
    compile_image(args, directory, args.source / "limbo/scalarowner.b", "dispatcher.dis")
    compile_image(args, directory, ROOT / "lib/styxrpc/tests/flashvm/asinc-scope_owner.b", "asinc-scope.dis")


# EN: Compile OS fixtures and run both movies against stock DisVM and its live VFS.
# RU: Собрать фильмы OS и запустить их на штатной паре с живой VFS DisVM.
def os_files(args, directory, mode):
    directory.mkdir()
    assets = directory / "os-assets"
    assets.mkdir()
    for name, path in (("os-client", directory / "os-client.swf"), ("os-child", assets / "child.swf")):
        subprocess.run((args.swl, "link", path, compile_unit(args, directory, name)), check=True, timeout=20)
    (assets / "marker.txt").write_bytes(b"A")
    (directory / "child.txt").write_bytes(b"A")
    compile_image(args, directory, ROOT / "lib/styxrpc/tests/flashvm/os_check.b", "os-check.dis")
    paired_exchange(directory, lambda connection: [args.player, *connection, mode],
                    lambda connection: [args.runtime, *connection, "--at", "/rpc", "--", "--native-spawn",
                                        mode, "os-check.dis"],
                    "PASS os VFS handles and attached SWF", tcp=args.tcp)


# EN: Require frame diagnostics while DisVM reads .state and stages a replacement.
# RU: Проверить диагностику кадра, пока DisVM читает .state и загружает замену.
def failed_frame(args, directory, mode, client):
    paired_exchange(directory, lambda connection: [args.player, *connection, mode],
                    lambda connection: [args.runtime, *connection, "--at", "/peer", "--", "--native-spawn",
                                        mode, client.name],
                    "PASS DisVM inspected and replaced a failed FlashVM movie",
                    ("flashvm_styx_player: movie tick failed with status -3",), tcp=args.tcp)


# EN: Exercise both service lane assignments in each execution mode.
# RU: Проверить оба назначения серверных каналов в каждом режиме.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("player", "as2", "fir", "swl"):
        parser.add_argument("--" + name, type=lambda path: Path(path).resolve(), required=True)
    for name in ("runtime", "limbo"):
        parser.add_argument("--" + name, type=lambda path: Path(path).resolve())
    parser.add_argument("--tcp", action="store_true", help="use a direct TCP loopback connection")
    parser.add_argument("--ipv6", action="store_true", help="use ::1 for the TCP loopback test")
    only = parser.add_mutually_exclusive_group()
    only.add_argument("--exec-only", action="store_true", help="run only executable VFS callback scenarios")
    only.add_argument("--messages-only", action="store_true", help="run only published UniBus root scenarios")
    only.add_argument("--asinc-only", action="store_true", help="run only the compiled OS-thread task scenarios")
    args = parser.parse_args()
    if args.ipv6 and not args.tcp:
        parser.error("--ipv6 requires --tcp")
    if bool(args.runtime) != bool(args.limbo):
        parser.error("--runtime and --limbo are required together")
    args.source = ROOT / "disVM"
    with TemporaryDirectory(prefix="flashvm-peer-") as scratch:
        directory = Path(scratch)
        if args.asinc_only:
            movie = compile_role(args, directory, "asinc")
            provider = compile_exec_provider(args, directory)
            for mode in ("--no-jit", "--jit"):
                for reverse in (False, True):
                    exchange(args, directory, movie, provider, mode, reverse, "compiled detached asinc task")
            caller = compile_role(args, directory, "asinc-chan-caller")
            provider = compile_role(args, directory, "asinc-chan-provider")
            for mode in ("--no-jit", "--jit"):
                for reverse in (False, True):
                    exchange(args, directory, caller, provider, mode, reverse, "named channel in asinc task")
            if args.runtime:
                compile_dis_provider(args, directory)
                caller = compile_role(args, directory, "asinc-load-caller")
                scoped_caller = compile_role(args, directory, "asinc-scope-caller")
                for mode in ("--no-jit", "--jit"):
                    flash_calls_dis(args, directory, caller, mode, "asinc task")
                    flash_calls_dis(args, directory, scoped_caller, mode, "scoped asinc task",
                                    "asinc-scope.dis", "bounce")
            return
        if not args.exec_only:
            message_caller = compile_role(args, directory, "messages-caller")
            message_provider = compile_role(args, directory, "messages-provider")
            for mode in ("--no-jit", "--jit"):
                for reverse in (False, True):
                    exchange(args, directory, message_caller, message_provider, mode, reverse,
                             "script-published UniBus roots and hidden VFS paths")
        if args.messages_only:
            return
        exec_caller = compile_role(args, directory, "exec-caller")
        exec_provider = compile_exec_provider(args, directory)
        for mode in ("--no-jit", "--jit"):
            for reverse in (False, True):
                exchange(args, directory, exec_caller, exec_provider, mode, reverse, "executable VFS callbacks")
        if args.exec_only:
            return
        caller = compile_role(args, directory, "caller")
        reference = compile_role(args, directory, "ref")
        reference_provider = compile_role(args, directory, "ref-provider")
        provider = compile_role(args, directory, "provider")
        named_caller = compile_role(args, directory, "named-caller")
        named_provider = compile_role(args, directory, "named-provider")
        module_refs_caller = compile_role(args, directory, "module-refs-caller")
        module_refs_provider = compile_role(args, directory, "module-refs-provider")
        if args.runtime:
            tests = ROOT / "lib/styxrpc/tests/flashvm"
            compile_image(args, directory, tests / "peer_consumer.b", "peer-consumer.dis")
            compile_dis_provider(args, directory)
            dis_caller = compile_role(args, directory, "dis-caller")
            dis_asinc_caller = compile_role(args, directory, "asinc-load-caller")
            dis_scoped_caller = compile_role(args, directory, "asinc-scope-caller")
            compile_role(args, directory, "ref-failure")
            failure_client = compile_image(args, directory, tests / "peer_frame_failure.b", "frame-failure.dis")
        for mode in ("--no-jit", "--jit"):
            for reverse in (False, True):
                exchange(args, directory, reference, reference_provider, mode, reverse,
                         "transparent local refs across delayed peer readiness")
                exchange(args, directory, caller, provider, mode, reverse)
                exchange(args, directory, named_caller, named_provider, mode, reverse,
                         "blocking named exchange, cyclic graph and live refs")
                exchange(args, directory, module_refs_caller, module_refs_provider, mode, reverse,
                         "scoped module variable refs and original-slot updates")
            if args.runtime:
                dis_calls_flash(args, directory, mode)
                flash_calls_dis(args, directory, dis_caller, mode)
                flash_calls_dis(args, directory, dis_asinc_caller, mode, "asinc task")
                flash_calls_dis(args, directory, dis_scoped_caller, mode, "scoped asinc task",
                                "asinc-scope.dis", "bounce")
                os_files(args, directory / mode[2:], mode)
                failed_frame(args, directory, mode, failure_client)


if __name__ == "__main__":
    main()

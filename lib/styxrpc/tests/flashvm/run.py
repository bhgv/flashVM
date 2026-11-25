# Copyright (c) 2026 bhgv
# EN: Run two processes over one delegated socket per VM-bridge direction.
# RU: Запуск двух процессов через выданный сокет для каждого направления моста VM.
import argparse
from collections import Counter
from pathlib import Path
import selectors
import socket
import subprocess
import tempfile
from movie_assets import prepare as prepare_movies, terminal_movie


# EN: Compile a source with resolved imports/exports and return the image path.
# RU: Собрать исходник с типами импортов/экспортов и вернуть путь образа.
def compile_image(args, directory, source, image):
    target = directory / image
    subprocess.run(
        [args.limbo, "-D", "RG", "-I", args.source / "upstream/module",
         "-I", args.source / "module", "-I", source.parent, "-o", target, source],
        check=True, timeout=15,
    )
    return target


# EN: Return ordered client/provider stream ends; TCP is direct, with no relay.
# RU: Вернуть концы потоков клиента и провайдера; TCP без посредника.
def stream_pair(tcp=False):
    if not tcp:
        return socket.socketpair()
    with socket.create_server(("127.0.0.1", 0), backlog=1) as listener:
        client = socket.create_connection(listener.getsockname(), timeout=5)
        try:
            server, _ = listener.accept()
        except BaseException:
            client.close()
            raise
    client.settimeout(None)
    for endpoint in (client, server):
        endpoint.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    return client, server


# EN: Read the native listener's first readiness line from a borrowed pipe.
# RU: Прочитать первую строку готовности штатного слушателя из канала вывода.
def listener_port(output, timeout=10):
    with selectors.DefaultSelector() as selector:
        selector.register(output, selectors.EVENT_READ)
        if not selector.select(timeout):
            raise RuntimeError("native Styx listener did not announce its port")
        line = output.readline().strip()
    prefix = "styx listening "
    port = line.removeprefix(prefix)
    if port == line or not port.isdecimal() or not 1 <= int(port) <= 65535:
        raise RuntimeError(f"invalid native Styx listener announcement: {line!r}")
    return port


# EN: Collect provider output and require the agreed exit codes and VM markers.
# RU: Собрать вывод провайдера и проверить коды завершения и маркеры VM.
def verify_exchange(provider, result, expected, provider_trace):
    output, _ = provider.communicate(timeout=10)
    if (result.returncode or provider.returncode or expected not in result.stdout
            or Counter(provider_trace) - Counter(output.splitlines())
            or "local-fallback" in output.splitlines()):
        raise RuntimeError(
            f"client ({result.returncode}):\n{result.stdout}\n"
            f"provider ({provider.returncode}):\n{output}"
        )
    print(result.stdout, end="")


# EN: Connect the argv builders; bound the client by timeout seconds and check both exit codes.
# RU: Соединить команды, ограничить работу клиента по timeout и проверить коды завершения.
def exchange(directory, server_command, client_command, expected, provider_trace=(), *, timeout=15, tcp=False):
    client, server = stream_pair(tcp)
    with client, server:
        with subprocess.Popen(
            server_command(str(server.fileno())), cwd=directory,
            pass_fds=(server.fileno(),), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        ) as provider:
            server.close()
            try:
                result = subprocess.run(
                    client_command(str(client.fileno())), cwd=directory, pass_fds=(client.fileno(),),
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=timeout,
                )
                client.close()
                verify_exchange(provider, result, expected, provider_trace)
            finally:
                if provider.poll() is None:
                    provider.kill()


# EN: Pass only an address between real FlashVM and DisVM commands over TCP.
# RU: Передать штатным командам FlashVM и DisVM по TCP только адрес.
def paired_exchange(directory, player, desktop, expected, provider_trace=(), *, timeout=15, tcp=False):
    if not tcp:
        return exchange(directory, lambda fd: player(["--fd", fd]),
                        lambda fd: desktop(["--styx-mount-fd", fd]), expected, provider_trace,
                        timeout=timeout)
    host = "127.0.0.1"
    with subprocess.Popen(player(["--listen", host, "0"]), cwd=directory, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, text=True) as provider:
        try:
            port = listener_port(provider.stdout)
            result = subprocess.run(desktop(["--styx-mount-tcp", host, port]), cwd=directory,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=timeout)
            verify_exchange(provider, result, expected, provider_trace)
        finally:
            if provider.poll() is None:
                provider.kill()


# EN: Exercise a DisVM TCP listener with the SDK's FlashVM test caller, whose
# entry point still takes a delegated FD. Only the test caller receives an FD.
# RU: Проверить слушатель DisVM клиентом SDK на FlashVM. Дескриптор передаётся
# только тестовому клиенту, штатный DisVM сам слушает TCP.
def disvm_listener_exchange(directory, server_command, client_command, expected, *, tcp=False):
    if not tcp:
        return exchange(directory, lambda fd: server_command("fd", fd), client_command, expected)
    host = "127.0.0.1"
    with subprocess.Popen(server_command("tcp", host, "0"), cwd=directory, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, text=True) as provider:
        try:
            port = listener_port(provider.stdout)
            with socket.create_connection((host, int(port)), timeout=5) as stream:
                stream.settimeout(None)
                stream.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                fd = stream.fileno()
                result = subprocess.run(client_command(str(fd)), cwd=directory, pass_fds=(fd,),
                                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=15)
            verify_exchange(provider, result, expected, ())
        finally:
            if provider.poll() is None:
                provider.kill()


# EN: Exercise the default reverse namespace with both production executables.
# RU: Проверить встречное пространство имён по умолчанию на штатной паре.
def native_player(args, directory, tests, mode, windowed=False):
    directory.mkdir()
    prepare_movies(directory, native=True, writable=True)
    root = directory / "share"
    (directory / "paired-root.swf").rename(root / "paired-root.swf")
    (root / "movies").rename(root / "assets")
    (root / "movies").mkdir()
    image = compile_image(args, root, tests / "native_player.b", "native-player.dis")
    capture = root / "frame.ppm"
    options = ["--window", "--capture", capture] if windowed else []
    paired_exchange(
        root,
        lambda connection: [args.flash_host, *connection, mode, *options],
        lambda connection: [args.runtime, *connection, "--at", "/rpc",
                    "--", "--native-spawn", mode, image.name],
        "PASS native FlashVM and DisVM pair",
        ("styx-child", "styx-leaf", "styxrpc-read:0", "styxrpc-read:0",
         "styx-channel:object", "styx-channel:object"),
        tcp=args.tcp,
    )
    if (root / "marker.txt").read_bytes() != b"Z":
        raise RuntimeError("native namespace write differs")


    if windowed:
        check_frame(capture)


# EN: Check final RGB pixels through the raster backend shared with SDL.
# RU: Проверить RGB итогового кадра через общий с SDL растер.
def check_frame(capture):
    header = b"P6\n100 60\n255\n"
    frame = capture.read_bytes()
    if not frame.startswith(header) or len(frame) != len(header) + 100 * 60 * 3:
        raise RuntimeError("missing or malformed paired frame")
    # The selected path sets x=66. Verify the new fill and the cleared original position.
    for x, y, expected in ((70, 10, b"\xff\x00\x00"), (4, 10, b"\xff\xff\xff")):
        start = len(header) + (y * 100 + x) * 3
        if frame[start:start + 3] != expected:
            raise RuntimeError(f"paired frame mismatch at {x},{y}: {frame[start:start + 3].hex()}")


# EN: Run the real mounted DisVM and SWF on one connection with no application host.
# RU: Запустить штатный DisVM и SWF на одном соединении без хоста приложения.
def native_terminal(args, directory, tests, mode, windowed=False):
    (directory / "terminal.swf").write_bytes(terminal_movie())
    image = compile_image(args, directory, tests / "native_terminal.b", "terminal-test.dis")
    capture = directory / "terminal.ppm"
    options = ["--window", "--capture", capture] if windowed else []
    paired_exchange(
        directory,
        lambda connection: [args.flash_host, *connection, mode, *options],
        lambda connection: [args.runtime, *connection, "--at", "/rpc",
                    "--", "--native-spawn", mode, image.name],
        "PASS paired Unibus channels",
        tcp=args.tcp,
    )


    if windowed:
        check_frame(capture)


# EN: Select provider and caller, using only the real VM entry points.
# RU: Выбрать провайдера и клиента с использованием штатных точек входа VM.
def main():
    parser = argparse.ArgumentParser(description="Check the production FlashVM/DisVM Styx RPC bridge")
    for name in ("runtime", "limbo", "source", "flash-host"):
        parser.add_argument("--" + name, type=lambda value: Path(value).resolve(), required=True)
    parser.add_argument("--direction", choices=("flash-to-dis", "dis-to-flash", "launch", "vfs", "native-player", "native-window", "native-terminal", "native-terminal-window"), required=True)
    parser.add_argument("--device", type=lambda value: Path(value).resolve())
    profiles = parser.add_mutually_exclusive_group()
    for option, profile in (("playback", "playback"), ("playback-movies", "playback-movies"), ("object-image", "object-image"), ("module-load", "module-load"),
                            ("channel-pid-graph", "channel-pid-graph"), ("channel-pid-result", "channel-pid-result"),
                            ("channel-delegated", "channel-delegated"), ("channel-incoming", "channel-incoming"),
                            ("channel-args", "channel-args"), ("channel", "channel"), ("hybrid", "hybrid"),
                            ("graph", "graph"), ("refs", "ref"), ("managed", "managed")):
        profiles.add_argument("--" + option, dest="profile", action="store_const", const=profile)
    parser.add_argument("--jit", action="store_true")
    parser.add_argument("--tcp", action="store_true", help="use a direct TCP loopback connection")
    args = parser.parse_args()
    tests = Path(__file__).resolve().parent
    mode = "--jit" if args.jit else "--no-jit"
    with tempfile.TemporaryDirectory(prefix="flashvm-styxrpc-") as scratch:
        directory = Path(scratch)
        owner, caller, module, exports, result = {
            "playback": (None, "playback_client.b", "playback.dis", (), "SWF controls"),
            "playback-movies": (None, "playback_client.b", "playback.dis", (), "nested SWFs through Styx VFS"),
            "object-image": (None, "object_image_client.b", "flash-object.dis", (), "object image"),
            "module-load": ("limbo.b", None, "limbo.dis", ("next", "number"), None),
            "channel-delegated": (args.source / "examples/remote-scalar/styx-channel-recursive-owner.b",
                                  None, "channel-lifetime.dis", ("open", "idle"), None),
            "channel-pid-graph": ("channel_pid_graph.b", None, "channel-pid-graph.dis", ("open",), None),
            "channel-pid-result": (None, "channel_pid_result.b", "flash-pid-result.dis", (), "PID result mailbox"),
            "channel-incoming": (None, "channel_incoming.b", "flash-incoming.dis", (), "channel argument handlers"),
            "channel-args": ("channel_args_owner.b", None, "channel-args.dis",
                             ("exchange", "reject", "result", "relay", "wait", "nested"), None),
            "channel": ("channel_owner.b", "channel_client.b", "channel.dis", ("ports", "empty", "nested"), "live channels"),
            "hybrid": ("hybrid_owner.b", "hybrid_client.b", "hybrid.dis", ("change", "reject", "empty"),
                       "hybrid tables"),
            "graph": ("graph_owner.b", "graph_client.b", "graphs.dis",
                      ("nested", "replace", "make", "fail", "fan", "tupleedit"), "graph exports"),
            "ref": ("refs_owner.b", "refs_client.b", "refs.dis", ("change",), "ref exports"),
            "managed": ("limbo.b", "values.b", "limbo.dis", ("echo", "number"),
                        "managed values through Styx RPC"),
            None: ("limbo.b", "client.b", "limbo.dis", ("next",), "AVM1 through Styx RPC"),
        }[args.profile]
        action = f"{args.profile}-" if args.profile else ""
        if args.direction in ("native-player", "native-window"):
            native_player(args, directory / "live", tests, mode, args.direction == "native-window")
        elif args.direction in ("native-terminal", "native-terminal-window"):
            native_terminal(args, directory, tests, mode, args.direction == "native-terminal-window")
        elif args.direction == "flash-to-dis":
            if args.profile == "module-load":
                (directory / "local-only").write_bytes(b"native file excluded by the paired root")
            image = compile_image(args, directory, tests / owner, module)
            dispatcher = compile_image(args, directory, args.source / "limbo/scalarowner.b", "dispatcher.dis")
            arguments = [piece for name in exports for piece in ("--export", image.name, image.name, name)]
            disvm_listener_exchange(
                directory,
                lambda kind, *address: [args.runtime, "--styx-library-serve-" + kind, *address, *arguments,
                            "--", "--native-spawn", mode, dispatcher.name],
                lambda fd: [args.flash_host, action + "call", fd],
                "PASS FlashVM Styx RPC host",
                tcp=args.tcp,
            )
        elif args.direction == "launch":
            image = compile_image(args, directory, tests / "context.b", "context.dis")
            disvm_listener_exchange(
                directory,
                lambda kind, *address: [args.runtime, "--styx-serve-" + kind, *address, "--persistent",
                            "--execute-as", "context.dis", image.name, "--execute-as", "park.dis", image.name,
                            "--", "--native-spawn", mode, image.name],
                lambda fd: [args.flash_host, "launch", fd],
                "PASS FlashVM Styx RPC host",
                tcp=args.tcp,
            )
        elif args.direction == "vfs":
            if not args.device:
                parser.error("--device is required for VFS")
            exchange(
                directory,
                lambda fd: [args.device, fd],
                lambda fd: [args.flash_host, "vfs", fd],
                "PASS FlashVM Styx RPC host",
                tcp=args.tcp,
            )
        else:
            if args.profile == "playback-movies":
                prepare_movies(directory)
            image = compile_image(args, directory, tests / caller, "client.dis")
            transport = [] if args.profile in ("channel", "channel-incoming", "channel-pid-result", "object-image") else ["--raw"]
            jit_role = args.jit and args.profile in ("playback", "playback-movies")
            exchange(
                directory,
                lambda fd: [args.flash_host, action + ("jit-serve" if jit_role else "serve"), fd],
                lambda fd: [args.runtime, "--styx-mount-fd", fd, *transport, "--at", "/rpc",
                            "--", "--native-spawn", mode, image.name],
                f"PASS DisVM loaded FlashVM {result}",
                tcp=args.tcp,
            )


if __name__ == "__main__":
    main()

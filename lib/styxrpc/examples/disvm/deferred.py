# Copyright (c) 2026 bhgv
# EN: Run the deferred C provider with a standalone SDK or a real Limbo client.
# RU: Запустить отложенный провайдер C с автономным SDK или настоящим клиентом Limbo.
from pathlib import Path
import argparse
import socket
import subprocess
import tempfile


# EN: Return connected owned streams for the selected local transport.
# RU: Вернуть пару собственных соединённых потоков выбранного локального транспорта.
def stream_pair(tcp):
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


# EN: Serve one socket, require successful client completion and exact provider retirement.
# RU: Обслужить один сокет и проверить завершение клиента и точное освобождение запросов.
def exercise(device, command, client, server, expected, retired, directory):
    with subprocess.Popen([device, str(server.fileno())], pass_fds=(server.fileno(),),
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True) as provider:
        server.close()
        try:
            result = subprocess.run(command, pass_fds=(client.fileno(),), cwd=directory,
                                    capture_output=True, text=True, timeout=16)
            client.close()
            output, _ = provider.communicate(timeout=8)
            if result.returncode or provider.returncode or expected not in result.stdout or retired not in output:
                raise RuntimeError(result.stdout + result.stderr + output)
            print(result.stdout, end="")
            print(output, end="")
        finally:
            if provider.poll() is None:
                provider.kill()


# EN: Compile the Limbo fixture when requested and run the appropriate caller.
# RU: При необходимости собрать модуль Limbo и запустить выбранного клиента.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--device", required=True, type=lambda path: Path(path).resolve())
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--client", type=lambda path: Path(path).resolve())
    modes.add_argument("--runtime", type=lambda path: Path(path).resolve())
    parser.add_argument("--limbo", type=Path)
    parser.add_argument("--module", type=Path)
    parser.add_argument("--native-module", type=Path)
    parser.add_argument("--jit", action="store_true")
    parser.add_argument("--tcp", action="store_true")
    parser.add_argument("--pressure", action="store_true")
    args = parser.parse_args()
    if args.runtime and not (args.limbo and args.module and args.native_module):
        parser.error("--runtime requires --limbo, --module and --native-module")
    if args.runtime and args.pressure:
        parser.error("--pressure requires the standalone --client")
    with tempfile.TemporaryDirectory(prefix="styxrpc-deferred-") as temporary:
        client, server = stream_pair(args.tcp)
        with client, server:
            if args.runtime:
                source = Path(__file__).with_suffix(".b").resolve()
                image = Path(temporary) / "deferred.dis"
                subprocess.run([args.limbo.resolve(), "-D", "RG", "-I", args.module.resolve(),
                                "-I", args.native_module.resolve(), "-o", image, source], check=True, timeout=15)
                command = [args.runtime, "--styx-mount-fd", str(client.fileno()), "--raw", "--at", "/device",
                           "--", "--native-spawn", "--jit" if args.jit else "--no-jit", image]
                expected = "PASS Limbo cancelled mounted C provider reads and writes"
                retired = "retired 1 reads and 1 writes by cancellation"
            else:
                command = [args.client, str(client.fileno())]
                expected = "PASS standalone C SDK deferred file transfers"
                retired = "retired 2 reads and 1 writes by cancellation"
                if args.pressure:
                    command.append("pressure")
                    retired = "retired 1 reads and 0 writes by cancellation"
            exercise(args.device, command, client, server, expected, retired, temporary)


if __name__ == "__main__":
    main()

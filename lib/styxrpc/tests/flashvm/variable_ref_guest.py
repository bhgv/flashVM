# Copyright (c) 2026 bhgv
"""Exercise a live wire reference on DisVM. / Проверка сетевой ссылки на DisVM."""
import argparse
from pathlib import Path
import selectors
import socket
import subprocess
from tempfile import TemporaryDirectory

from run import compile_image


# EN: Receive the test provider's exact descriptor before launching the guest.
# RU: Получить дескриптор провайдера перед запуском гостя.
def descriptor(stream):
    with selectors.DefaultSelector() as waiter:
        waiter.register(stream, selectors.EVENT_READ)
        if not waiter.select(5):
            raise RuntimeError("reference provider timed out")
    data = stream.read(8)
    if len(data) != 8:
        raise RuntimeError("reference provider did not announce a descriptor")
    return data


# EN: Compile the library and guest, then check both process exits on a paired connection.
# RU: Скомпилировать библиотеку и гостя, проверить завершение обоих процессов.
def main():
    parser = argparse.ArgumentParser()
    for field in ("runtime", "limbo", "source", "provider"):
        parser.add_argument("--" + field, type=lambda path: Path(path).resolve(), required=True)
    parser.add_argument("--jit", action="store_true")
    parser.add_argument("--mixed", action="store_true")
    args = parser.parse_args()
    with TemporaryDirectory(prefix="styxrpc-limbo-ref-") as scratch:
        root = Path(scratch)
        compile_image(args, root, args.source / "limbo/styxintref.b", "styxintref.dis")
        guest = Path(__file__).with_suffix(".b")
        if args.mixed:
            compile_image(args, root, args.source / "limbo/styxchannel.b", "styxchannel.dis")
            guest = guest.with_name("mixed_ref.b")
        compile_image(args, root, guest, "guest.dis")
        client, server = socket.socketpair()
        with client, server, subprocess.Popen(
                [args.provider, "--dis-mixed-fd" if args.mixed else "--dis-fd", str(server.fileno())],
                pass_fds=(server.fileno(),), stdout=subprocess.PIPE) as provider:
            server.close()
            try:
                if not args.mixed:
                    (root / "reference.bin").write_bytes(descriptor(provider.stdout))
                command = [args.runtime, "--styx-mount-fd", str(client.fileno()), "--at", "/rpc",
                           "--", "--native-spawn", "--jit" if args.jit else "--no-jit", "guest.dis"]
                result = subprocess.run(command, cwd=root, pass_fds=(client.fileno(),), text=True,
                                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=10)
                client.close()
                if result.returncode or "PASS Limbo live Int32 reference" not in result.stdout:
                    raise RuntimeError(result.stdout)
                if provider.wait(timeout=5):
                    raise RuntimeError("issuer did not observe the guest's write")
                print(result.stdout, end="")
            finally:
                if provider.poll() is None:
                    provider.kill()


if __name__ == "__main__":
    main()

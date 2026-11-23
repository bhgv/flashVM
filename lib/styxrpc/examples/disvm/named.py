# Copyright (c) 2026 bhgv
# EN: Exercise named Dis launches with a C-hosted reverse tree in both VM modes.
# RU: Проверка именованных запусков и встречного дерева на C в двух режимах VM.
import argparse
from pathlib import Path
import socket
import subprocess
import tempfile


# EN: Run one persistent provider and one C launcher on opposite socket ends.
# RU: Запустить постоянный сервис и C-клиента на двух концах сокета.
def exercise(args, directory, mode):
    client, server = socket.socketpair()
    with client, server, subprocess.Popen(
        [args.runtime, "--styx-serve-fd", str(server.fileno()), "--persistent",
         "--execute-as", "device.dis", "device.dis", "--execute-as", "other.dis", "other.dis",
         "--", mode, "other.dis"],
        pass_fds=(server.fileno(),), cwd=directory, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, text=True
    ) as vm:
        server.close()
        try:
            result = subprocess.run([args.driver, str(client.fileno())],
                                    pass_fds=(client.fileno(),), capture_output=True, text=True, timeout=15)
            client.close()
            output, _ = vm.communicate(timeout=10)
            if result.returncode or vm.returncode or "PASS one socket" not in result.stdout:
                raise RuntimeError(result.stdout + result.stderr + output + str(vm.returncode))
            print(mode, result.stdout, end="\n")
        finally:
            if vm.poll() is None:
                vm.kill()


# EN: Compile the two provider images; run each VM mode on a fresh session.
# RU: Собрать два образа и проверить каждый режим VM в отдельной сессии.
def main():
    parser = argparse.ArgumentParser()
    for name in ("runtime", "limbo", "module", "native-module", "driver"):
        parser.add_argument("--" + name, required=True, type=lambda arg: Path(arg).resolve())
    args = parser.parse_args()
    source = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="styxrpc-named-") as directory:
        for image, program in (("device.dis", "device-program.b"), ("other.dis", "program.b")):
            subprocess.run([args.limbo, "-I", args.module, "-I", args.native_module, "-o",
                            Path(directory) / image, source / program], check=True, timeout=10)
        for mode in ("--no-jit", "--jit"):
            exercise(args, directory, mode)


if __name__ == "__main__":
    main()

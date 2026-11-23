# Copyright (c) 2026 bhgv
# EN: Check both successful and failed DisVM launches driven by a C peer.
# RU: Проверить успешный и неуспешный запуски DisVM из C-клиента.
import argparse
from pathlib import Path
import socket
import subprocess
import tempfile


# EN: Serve one fresh connection; require the exact exit interpretation on both ends.
# RU: Обслужить новое соединение и проверить точное значение кода на обоих концах.
def exercise(runtime, driver, directory, mode, startup, expected):
    client, server = socket.socketpair()
    with client, server, subprocess.Popen(
        [runtime, "--styx-serve-fd", str(server.fileno()), "--", mode, "program.dis"],
        pass_fds=(server.fileno(),), cwd=directory, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, text=True
    ) as vm:
        server.close()
        try:
            result = subprocess.run([driver, str(client.fileno()), startup],
                                    pass_fds=(client.fileno(),), capture_output=True, text=True, timeout=8)
            client.close()
            output, _ = vm.communicate(timeout=8)
            if result.returncode != expected or (vm.returncode == 0) != (expected == 0):
                raise RuntimeError(result.stdout + result.stderr + output)
            print(result.stdout, end="")
        finally:
            if vm.poll() is None:
                vm.kill()


# EN: Compile the fixture and run independent interpreter and JIT launch cases.
# RU: Собрать пример и выполнить отдельные запуски в интерпретаторе и JIT.
def main():
    parser = argparse.ArgumentParser()
    for name in ("runtime", "limbo", "module", "driver"):
        parser.add_argument("--" + name, required=True, type=lambda arg: Path(arg).resolve())
    args = parser.parse_args()
    source = Path(__file__).with_name("program.b").resolve()
    with tempfile.TemporaryDirectory(prefix="styxrpc-launch-") as directory:
        subprocess.run([args.limbo, "-I", args.module, "-o", Path(directory) / "program.dis", source],
                       check=True, timeout=10)
        for mode in ("--no-jit", "--jit"):
            exercise(args.runtime, args.driver, directory, mode, "native-c", 0)
            exercise(args.runtime, args.driver, directory, mode, "wrong", 3)


if __name__ == "__main__":
    main()

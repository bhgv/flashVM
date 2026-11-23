# Copyright (c) 2026 bhgv
# EN: Check both delegated channel routes between the C example and a stock DisVM provider.
# RU: Проверить оба делегированных маршрута между клиентом C и штатным провайдером DisVM.
import argparse
from pathlib import Path
import socket
import subprocess
import tempfile


# EN: Use one socket and terminate the provider on failure without retaining inherited descriptors.
# RU: Подключить одним сокетом и завершить провайдер при ошибке, не оставляя дескрипторов.
def exercise(args, directory, mode):
    module = "channel-lifetime.dis"
    client, server = socket.socketpair()
    with client, server, subprocess.Popen(
        [args.runtime, "--styx-library-serve-fd", str(server.fileno()),
         "--export", module, module, "open", "--", "--native-spawn", mode, "dispatcher.dis"],
        pass_fds=(server.fileno(),), cwd=directory,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
    ) as vm:
        server.close()
        try:
            result = subprocess.run([args.driver, str(client.fileno())], pass_fds=(client.fileno(),),
                                    capture_output=True, text=True, timeout=20)
            client.close()
            output, _ = vm.communicate(timeout=15)
            if result.returncode or vm.returncode or "PASS delegated PID and module channels" not in result.stdout:
                raise RuntimeError(result.stdout + result.stderr + output)
            print(mode, result.stdout, end="")
        finally:
            if vm.poll() is None:
                vm.kill()


# EN: Compile the repository's recursive export and dispatcher, then run both VM profiles.
# RU: Собрать рекурсивный экспорт и диспетчер репозитория, затем проверить оба режима VM.
def main():
    parser = argparse.ArgumentParser(description="Exchange nested channel grants with an independent C client")
    for name in ("runtime", "limbo", "disvm-source", "driver"):
        parser.add_argument("--" + name, required=True, type=lambda arg: Path(arg).resolve())
    args = parser.parse_args()
    source = args.disvm_source
    examples = source / "examples/remote-scalar"
    with tempfile.TemporaryDirectory(prefix="styxrpc-routed-") as directory:
        for file, image in ((source / "limbo/scalarowner.b", "dispatcher.dis"),
                            (examples / "styx-channel-recursive-owner.b", "channel-lifetime.dis")):
            subprocess.run([args.limbo, "-D", "RG", "-I", source / "upstream/module", "-I", source / "module",
                            "-I", examples, "-o", Path(directory) / image, file], check=True, timeout=15)
        for mode in ("--no-jit", "--jit"):
            exercise(args, directory, mode)


if __name__ == "__main__":
    main()

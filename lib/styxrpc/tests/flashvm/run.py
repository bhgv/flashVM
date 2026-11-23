# Copyright (c) 2026 bhgv
# EN: Run two processes over one delegated socket per VM-bridge direction.
# RU: Запуск двух процессов через выданный сокет для каждого направления моста VM.
import argparse
from pathlib import Path
import socket
import subprocess
import tempfile


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


# EN: Connect the two argv builders to opposite ends; check output and both exit codes.
# RU: Подключить команды к концам сокета и проверить вывод и оба кода завершения.
def exchange(directory, server_command, client_command, expected):
    client, server = socket.socketpair()
    with client, server:
        with subprocess.Popen(
            server_command(str(server.fileno())), cwd=directory,
            pass_fds=(server.fileno(),), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        ) as provider:
            server.close()
            try:
                result = subprocess.run(
                    client_command(str(client.fileno())), cwd=directory, pass_fds=(client.fileno(),),
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=15,
                )
                client.close()
                output, _ = provider.communicate(timeout=10)
                if result.returncode or provider.returncode or expected not in result.stdout:
                    raise RuntimeError(
                        f"client ({result.returncode}):\n{result.stdout}\n"
                        f"provider ({provider.returncode}):\n{output}"
                    )
                print(result.stdout, end="")
            finally:
                if provider.poll() is None:
                    provider.kill()


# EN: Select provider and caller, using only the real VM entry points.
# RU: Выбрать провайдера и клиента с использованием штатных точек входа VM.
def main():
    parser = argparse.ArgumentParser(description="Check the production FlashVM/DisVM Styx RPC bridge")
    for name in ("runtime", "limbo", "source", "flash-host"):
        parser.add_argument("--" + name, type=lambda value: Path(value).resolve(), required=True)
    parser.add_argument("--direction", choices=("flash-to-dis", "dis-to-flash", "launch", "vfs"), required=True)
    parser.add_argument("--device", type=lambda value: Path(value).resolve())
    profiles = parser.add_mutually_exclusive_group()
    for option, profile in (("hybrid", "hybrid"), ("graph", "graph"), ("refs", "ref"), ("managed", "managed")):
        profiles.add_argument("--" + option, dest="profile", action="store_const", const=profile)
    parser.add_argument("--jit", action="store_true")
    args = parser.parse_args()
    tests = Path(__file__).resolve().parent
    mode = "--jit" if args.jit else "--no-jit"
    with tempfile.TemporaryDirectory(prefix="flashvm-styxrpc-") as scratch:
        directory = Path(scratch)
        owner, caller, module, exports, result = {
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
        if args.direction == "flash-to-dis":
            image = compile_image(args, directory, tests / owner, module)
            dispatcher = compile_image(args, directory, args.source / "limbo/scalarowner.b", "dispatcher.dis")
            arguments = [piece for name in exports for piece in ("--export", image.name, image.name, name)]
            exchange(
                directory,
                lambda fd: [args.runtime, "--styx-library-serve-fd", fd, *arguments,
                            "--", "--native-spawn", mode, dispatcher.name],
                lambda fd: [args.flash_host, action + "call", fd],
                "PASS FlashVM Styx RPC host",
            )
        elif args.direction == "launch":
            image = compile_image(args, directory, tests / "context.b", "context.dis")
            exchange(
                directory,
                lambda fd: [args.runtime, "--styx-serve-fd", fd, "--persistent",
                            "--execute-as", "context.dis", image.name, "--execute-as", "park.dis", image.name,
                            "--", "--native-spawn", mode, image.name],
                lambda fd: [args.flash_host, "launch", fd],
                "PASS FlashVM Styx RPC host",
            )
        elif args.direction == "vfs":
            if not args.device:
                parser.error("--device is required for VFS")
            exchange(
                directory,
                lambda fd: [args.device, fd],
                lambda fd: [args.flash_host, "vfs", fd],
                "PASS FlashVM Styx RPC host",
            )
        else:
            image = compile_image(args, directory, tests / caller, "client.dis")
            exchange(
                directory,
                lambda fd: [args.flash_host, action + "serve", fd],
                lambda fd: [args.runtime, "--styx-mount-fd", fd, "--raw", "--at", "/rpc",
                            "--", "--native-spawn", mode, image.name],
                f"PASS DisVM loaded FlashVM {result}",
            )


if __name__ == "__main__":
    main()

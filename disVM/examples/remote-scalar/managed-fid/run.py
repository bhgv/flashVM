# Copyright (c) 2026 bhgv
# EN: Build and run the original library and the mounted client in separate DisVMs.
# RU: Собрать и запустить исходную библиотеку и клиента в отдельных DisVM.
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile


# EN: Compile each source with resolved Styx export and import metadata.
# RU: Собрать исходники с разрешёнными типами экспортов и импортов Styx.
def compile_images(compiler, disvm, example, owner, caller):
    compiler_args = [
        compiler, "-D", "RG", "-I", disvm / "upstream/module",
        "-I", disvm / "module", "-I", example,
    ]
    for source, image in (
        (disvm / "limbo/scalarowner.b", owner / "scalarowner.dis"),
        (example / "target.b", owner / "target.dis"),
        (example / "client.b", caller / "client.dis"),
    ):
        subprocess.run([*compiler_args, "-o", image, source], check=True)


# EN: Run both stock CLI entry points on one socket and verify both exit successfully.
# RU: Запустить обе штатные команды на одном сокете и проверить их завершение.
def exercise(runtime, owner, caller, mode):
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        listener.listen()
        client_fd = socket.create_connection(listener.getsockname())
        owner_fd, _ = listener.accept()
    with client_fd, owner_fd, subprocess.Popen(
        [runtime, "--styx-library-serve-fd", str(owner_fd.fileno()),
         "--export", "managed.dis", "target.dis", "echo",
         "--export", "managed.dis", "target.dis", "accept",
         "--export", "managed.dis", "target.dis", "update",
         "--export", "managed.dis", "target.dis", "nested",
         "--export", "managed.dis", "target.dis", "reference",
         "--", mode, "scalarowner.dis"],
        cwd=owner, pass_fds=(owner_fd.fileno(),), stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, text=True,
    ) as server:
        owner_fd.close()
        try:
            client = subprocess.run(
                [runtime, "--styx-mount-fd", str(client_fd.fileno()), "--at",
                 "/transport", "--", mode, "client.dis"],
                cwd=caller, pass_fds=(client_fd.fileno(),),
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=55,
            )
            client_fd.close()
            output, _ = server.communicate(timeout=10)
            if client.returncode or server.returncode or "PASS managed Styx load" not in client.stdout:
                raise RuntimeError(f"{mode}\nClient:\n{client.stdout}\nOwner:\n{output}")
            print(f"{mode}: PASS managed Styx load", flush=True)
        finally:
            if server.poll() is None:
                server.kill()


# EN: Keep generated images private and exercise both execution modes.
# RU: Хранить сборку во временном каталоге и проверить оба режима выполнения.
def main():
    if len(sys.argv) != 3:
        sys.exit(f"usage: {sys.argv[0]} DISVM LIMBO")
    runtime, compiler = (Path(arg).resolve() for arg in sys.argv[1:])
    example = Path(__file__).resolve().parent
    disvm = example.parents[2]
    with tempfile.TemporaryDirectory(prefix="disvm-managed-fid-", dir=os.environ.get("TMPDIR")) as scratch:
        owner, caller = Path(scratch) / "owner", Path(scratch) / "caller"
        owner.mkdir()
        caller.mkdir()
        compile_images(compiler, disvm, example, owner, caller)
        for mode in ("--no-jit", "--jit"):
            exercise(runtime, owner, caller, mode)


if __name__ == "__main__":
    main()

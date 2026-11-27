# Copyright (c) 2026 bhgv
# EN: Compile a real Limbo guest, delegate one raw Styx socket, and verify both processes.
# RU: Собрать гостя Limbo, выдать один raw-сокет Styx и проверить оба процесса.
import argparse
from pathlib import Path
import socket
import subprocess
from tempfile import TemporaryDirectory


# EN: Compile the example and its two existing Limbo libraries into directory.
# RU: Собрать пример и две существующие библиотеки Limbo в directory.
def compile_guest(args, directory):
    guest = Path(__file__).with_name("guest.b").resolve()
    for name, source in (("styxvalue", args.limbo_sources / "styxvalue.b"),
                         ("styxnamed", args.limbo_sources / "styxnamed.b"),
                         ("guest", guest)):
        subprocess.run([args.limbo, "-D", "RG", "-I", args.system_modules,
                        "-I", args.native_modules, "-o", directory / (name + ".dis"), source],
                       check=True, timeout=15)
    return directory / "guest.dis"


# EN: Launch the C provider and the Limbo guest. Each inherits only its own end;
# the launcher closes its duplicates so a peer's exit reaches the other.
# RU: Запустить сервис C и гостя Limbo. Каждый наследует свой конец. Скрипт
# закрывает копии, чтобы отключение одной стороны достигло другой.
def run_pair(args, directory, image):
    guest_fd, host_fd = socket.socketpair()
    with guest_fd, host_fd, subprocess.Popen([args.host, str(host_fd.fileno())],
            pass_fds=(host_fd.fileno(),), stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            cwd=directory, text=True) as host:
        host_fd.close()
        try:
            mode = "--jit" if args.jit else "--no-jit"
            guest = subprocess.run([args.disvm, "--styx-mount-fd", str(guest_fd.fileno()), "--raw",
                                    "--at", "/device", "--", "--native-spawn", mode, image],
                                   pass_fds=(guest_fd.fileno(),), stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, cwd=directory, text=True, timeout=15)
            guest_fd.close()
            host_output, _ = host.communicate(timeout=10)
            if guest.returncode or host.returncode or                 "PASS C shared cell cancellation" not in host_output or                 "PASS Limbo observed both values" not in guest.stdout:
                raise RuntimeError(f"C ({host.returncode}):\n{host_output}\n"
                                   f"Limbo ({guest.returncode}):\n{guest.stdout}")
            print(host_output, end="")
            print(guest.stdout, end="")
        finally:
            if host.poll() is None:
                host.kill()  # Reap the disposable provider even if the guest times out.


# EN: Resolve paths before switching to the temporary workspace used by DisVM.
# RU: Разрешить пути до перехода во временный рабочий каталог DisVM.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("host", "disvm", "limbo", "system-modules", "native-modules", "limbo-sources"):
        parser.add_argument("--" + name, required=True, type=lambda path: Path(path).resolve())
    parser.add_argument("--jit", action="store_true")
    args = parser.parse_args()
    with TemporaryDirectory(prefix="styxrpc-shared-cell-") as temporary:
        directory = Path(temporary)
        run_pair(args, directory, compile_guest(args, directory))


if __name__ == "__main__":
    main()

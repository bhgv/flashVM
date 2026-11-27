# Copyright (c) 2026 bhgv
# EN: Drive one C provider from the standalone C client or a mounted Dis guest.
# RU: Проверить провайдера C из клиента C или из гостя Dis через mount.
from pathlib import Path
import argparse
import socket
import subprocess
import tempfile


# EN: Compile a guest if selected, run both socket owners and require clean retirement.
# RU: При выборе гостя собрать модуль, запустить оба конца сокета и проверить завершение.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--device", required=True, type=Path)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--client", type=Path)
    modes.add_argument("--runtime", type=Path)
    parser.add_argument("--limbo", type=Path)
    parser.add_argument("--module", type=Path)
    parser.add_argument("--jit", action="store_true")
    args = parser.parse_args()
    if args.runtime and (not args.limbo or not args.module):
        parser.error("--runtime requires --limbo and --module")
    with tempfile.TemporaryDirectory(prefix="styxrpc-mutations-") as directory:
        left, right = socket.socketpair()
        with left, right:
            if args.runtime:
                source = Path(__file__).with_suffix(".b")
                image = Path(directory) / "mutations.dis"
                subprocess.run([args.limbo.resolve(), "-D", "RG", "-I", args.module.resolve(),
                                "-o", image, source.resolve()], check=True, timeout=15)
                command = [args.runtime.resolve(), "--styx-mount-fd", str(left.fileno()), "--raw",
                           "--at", "/device", "--", "--native-spawn",
                           "--jit" if args.jit else "--no-jit", image]
                expected = "PASS mounted Sys.create and Sys.remove against the C SDK"
            else:
                command = [args.client.resolve(), str(left.fileno())]
                expected = "PASS C SDK mkdir, create and remove"
            with subprocess.Popen([args.device.resolve(), str(right.fileno())],
                                  pass_fds=(right.fileno(),)) as device:
                right.close()
                try:
                    result = subprocess.run(command, pass_fds=(left.fileno(),), cwd=directory,
                                            capture_output=True, text=True, timeout=20)
                    if result.returncode or expected not in result.stdout:
                        raise RuntimeError(result.stdout + result.stderr)
                    print(result.stdout, end="")
                finally:
                    left.close()
                    try:
                        code = device.wait(timeout=10)
                    except subprocess.TimeoutExpired:
                        device.kill()
                        device.wait()
                        raise
                if code:
                    raise RuntimeError(f"device exited {code}")


if __name__ == "__main__":
    main()

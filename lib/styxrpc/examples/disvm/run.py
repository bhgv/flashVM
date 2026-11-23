# Copyright (c) 2026 bhgv
# EN: Test an ordinary Dis load over 9P on a fresh device socket.
# RU: Проверить штатный load Dis через 9P на новом сокете устройства.
from pathlib import Path
import argparse
import socket
import subprocess
import tempfile


# EN: Compile the caller, then run the independent C device against one DisVM.
# RU: Собрать клиента, затем запустить независимое устройство C против DisVM.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("runtime", "limbo", "module", "device"):
        parser.add_argument("--" + name, required=True, type=lambda arg: Path(arg).resolve())
    parser.add_argument("--interface", type=lambda arg: Path(arg).resolve())
    parser.add_argument("--jit", action="store_true")
    args = parser.parse_args()
    source = Path(__file__).with_name("client.b")
    with tempfile.TemporaryDirectory(prefix="styxrpc-device-") as temporary:
        image = Path(temporary) / "client.dis"
        generator = args.interface or args.device.with_name("styxrpc-interface")
        with (Path(temporary) / "device.m").open("wb") as interface:
            subprocess.run([generator], stdout=interface, check=True, timeout=15)
        subprocess.run([args.limbo, "-D", "RG", "-I", args.module, "-I", temporary, "-o", image, source],
                       check=True, timeout=15)
        caller, device = socket.socketpair()
        with caller, device:
            process = subprocess.Popen([args.device, str(device.fileno())], pass_fds=(device.fileno(),))
            device.close()
            try:
                result = subprocess.run(
                    [args.runtime, "--styx-mount-fd", str(caller.fileno()), "--raw", "--at", "/device",
                     "--", "--native-spawn", "--jit" if args.jit else "--no-jit", image],
                    pass_fds=(caller.fileno(),), cwd=temporary, capture_output=True, text=True, timeout=15)
                if result.returncode or "PASS Dis load of standalone C device" not in result.stdout:
                    raise RuntimeError(result.stdout + result.stderr)
            finally:
                caller.close()
                try:
                    code = process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
                    raise
            if code:
                raise RuntimeError(f"device exited {code}")
            print(result.stdout, end="")


if __name__ == "__main__":
    main()

# Copyright (c) 2026 bhgv
"""Launch the two VMs on a single inherited socketpair. / Запуск двух VM на паре сокетов."""

import argparse
from pathlib import Path
import socket
import subprocess
from tempfile import TemporaryDirectory


# EN: Check the final 320 × 240 PPM and the displaced panel's interior.
# RU: Проверить итоговый PPM 320 × 240 и внутреннюю точку смещённой панели.
def check_capture(path):
    header = b"P6\n320 240\n255\n"
    data = path.read_bytes()
    if not data.startswith(header) or len(data) != len(header) + 320 * 240 * 3:
        raise RuntimeError("malformed 320 × 240 PPM")
    for x, y, color in ((25, 35, b"\xff\xff\xff"), (100, 42, b"\x17\x62\x46")):
        offset = len(header) + (y * 320 + x) * 3
        if data[offset:offset + 3] != color:
            raise RuntimeError(f"unexpected panel position at {x},{y}")


# EN: Pass each process one owned descriptor, wait for both, and check the Dis receipt.
# RU: Передать каждому процессу свой дескриптор, дождаться обоих и проверить ответ Dis.
def run(args, capture=None):
    mode = "--jit" if args.jit else "--no-jit"
    options = ["--window"] if args.window or capture else []
    if capture:
        options += ["--capture", capture]
    client, provider = socket.socketpair()
    with client, provider:
        with subprocess.Popen(
            [args.flashvm, "--fd", str(provider.fileno()), mode, *options],
            cwd=args.directory, pass_fds=(provider.fileno(),),
        ) as player:
            provider.close()
            try:
                result = subprocess.run(
                    [args.disvm, "--styx-mount-fd", str(client.fileno()), "--at", "/rpc",
                     "--", mode, "terminal.dis"],
                    cwd=args.directory, pass_fds=(client.fileno(),), timeout=15,
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, encoding="utf-8",
                )
                client.close()
                player_status = player.wait(timeout=5)
            finally:
                if player.poll() is None:
                    player.kill()
    print(result.stdout, end="")
    if result.returncode or player_status or "PASS compiled AS2 + Limbo: " not in result.stdout:
        raise RuntimeError(f"DisVM exit {result.returncode}; FlashVM exit {player_status}")


# EN: Make tools and artifacts explicit, and surface failures as a nonzero status.
# RU: Явно указать инструменты и файлы и сообщить об ошибке кодом завершения.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("disvm", type=Path)
    parser.add_argument("flashvm", type=Path)
    parser.add_argument("--directory", type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument("--jit", action="store_true")
    parser.add_argument("--window", action="store_true")
    parser.add_argument("--capture", type=Path, metavar="PPM")
    args = parser.parse_args()
    args.disvm = args.disvm.resolve()
    args.flashvm = args.flashvm.resolve()
    try:
        if args.capture:
            destination = args.capture.resolve()
            with TemporaryDirectory(dir=destination.parent) as directory:
                capture = Path(directory) / "frame.ppm"
                run(args, capture)
                check_capture(capture)
                capture.replace(destination)
            print(f"PASS final RGB pixels: {destination}")
        else:
            run(args)
    except (OSError, subprocess.TimeoutExpired, RuntimeError) as error:
        parser.exit(1, f"paired-terminal: {error}\n")


if __name__ == "__main__":
    main()

# Copyright (c) 2026 bhgv
# EN: Give the independent native example processes opposite socket ends.
# RU: Соединить независимые нативные примеры через пару сокетов.
import socket
import subprocess
import sys


# EN: Start the binaries named on the command line and return their joint status.
# RU: Запустить указанные бинарники и вернуть общий статус выполнения.
def main():
    device, caller = socket.socketpair()
    with device, caller:
        with subprocess.Popen([sys.argv[1], str(device.fileno())], pass_fds=(device.fileno(),)) as server:
            device.close()
            try:
                result = subprocess.run(
                    [sys.argv[2], str(caller.fileno())], pass_fds=(caller.fileno(),), timeout=10)
                caller.close()
                return result.returncode or server.wait(timeout=5)
            finally:
                if server.poll() is None:
                    server.kill()


if __name__ == "__main__":
    sys.exit(main())

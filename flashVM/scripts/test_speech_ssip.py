#!/usr/bin/env python3
"""Exercise the real Unix-socket SSIP client against a protocol fixture, never hardware."""
import argparse
import errno
import sys
from pathlib import Path
import socket
import subprocess
import tempfile
import threading
import uuid


def exercise(program, mode, abstract=False):
    errors = []
    transcript = "first\n.\nCANCEL SELF\nПривет 😀"
    with tempfile.TemporaryDirectory(prefix="fvm-ssip-") as folder:
        name = "flashvm-ssip-" + uuid.uuid4().hex
        path = "\0" + name if abstract else str(Path(folder) / "socket")
        server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        server.bind(path)
        server.listen(1)
        server.settimeout(15)

        def serve():
            try:
                connection, _ = server.accept()
                with connection:
                    connection.settimeout(10)
                    with connection.makefile("rb") as reader:
                        if reader.readline() != b"SET SELF CLIENT_NAME flashvm:tts:default\r\n":
                            raise RuntimeError("Unexpected SSIP identity framing")
                        if mode == "malformed":
                            connection.sendall(b"not-a-status\r\n")
                            return
                        if mode == "rejected":
                            connection.sendall(b"500 ERROR\r\n")
                            return
                        connection.sendall(b"208-CLIENT\r\n208 OK\r\n")
                        if reader.readline() != b"SPEAK\r\n":
                            raise RuntimeError("SPEAK command was not correctly framed")
                        connection.sendall(b"230 OK RECEIVING DATA\r\n")
                        payload = []
                        while True:
                            line = reader.readline()
                            if not line:
                                raise RuntimeError("SSIP text ended without a dot terminator")
                            if line == b".\r\n":
                                break
                            if line.startswith(b".."):
                                line = line[1:]
                            payload.append(line)
                        if (
                            b"".join(payload).decode("utf-8")
                            != transcript.replace("\n", "\r\n") + "\r\n"
                        ):
                            raise RuntimeError(
                                "Dot-stuffing/newline conversion corrupted speech or injected commands"
                            )
                        connection.sendall(b"225-17\r\n225 OK MESSAGE QUEUED\r\n")
                        if mode == "disconnect":
                            return
                        for _ in range(2):
                            if reader.readline() != b"CANCEL SELF\r\n":
                                raise RuntimeError(
                                    "Cancellation was not scoped to this SSIP client"
                                )
                            connection.sendall(b"200 OK CANCELED\r\n")
            except BaseException as error:
                errors.append(error)
            finally:
                server.close()

        worker = threading.Thread(target=serve, daemon=True)
        worker.start()
        result = subprocess.run(
            [
                str(program),
                "--abstract" if abstract else "--linux",
                name if abstract else path,
                transcript,
            ],
            input="\n",
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=20,
        )
        worker.join(12)
        if worker.is_alive():
            raise RuntimeError("SSIP fixture did not finish")
        if errors:
            raise errors[0]
        if result.returncode < 0:
            raise RuntimeError(f"Speech client died from a signal: {result.returncode}")
        if (result.returncode == 0) != (mode == "success"):
            raise RuntimeError(f"Unexpected {mode} result {result.returncode}: {result.stderr}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tts_example", type=Path)
    args = parser.parse_args()
    try:
        for abstract in [False, True]:
            for mode in ["success", "malformed", "rejected", "disconnect"]:
                exercise(args.tts_example.resolve(), mode, abstract)
    except PermissionError as error:
        if error.errno not in (errno.EPERM, errno.EACCES):
            raise
        print(
            "SKIP explicit Unix-listener integration: runner denies AF_UNIX endpoint creation; socketpair transport tests remain separate"
        )
        return 77
    print(
        "PASS real SSIP socket client: UTF-8/dot framing, multiline replies, client cancellation, rejection, disconnect/SIGPIPE safety; fixture only, no audible-output claim"
    )


if __name__ == "__main__":
    sys.exit(main() or 0)

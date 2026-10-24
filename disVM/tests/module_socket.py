#!/usr/bin/env python3
"""Real loopback transport with explicit framing and a bounded server lifetime."""
import socket
import subprocess
import sys
import threading


# Send byte fragments on one connection, preserving a second unread frame.
def serve(listener, errors):
    try:
        connection, _ = listener.accept()
        with connection:
            connection.settimeout(5)
            for part in [b'\xc0', b'\x0c\x80', b'\x30\x00\xff\x2a', b'end']:
                connection.sendall(part)
    except Exception as error:
        errors.append(error)


# Always close and join; the helper is not part of the deployed runtime.
def main():
    errors = []
    with socket.socket() as listener:
        listener.bind(('127.0.0.1', 0))
        listener.listen(1)
        listener.settimeout(8)
        worker = threading.Thread(target=serve, args=(listener, errors))
        worker.start()
        try:
            result = subprocess.run([sys.argv[1], str(listener.getsockname()[1])], timeout=10)
        finally:
            worker.join(timeout=10)
        if worker.is_alive() or errors:
            raise RuntimeError('loopback server failed: ' + repr(errors))
        if result.returncode:
            raise RuntimeError('socket client failed: ' + str(result.returncode))


if __name__ == '__main__':
    main()

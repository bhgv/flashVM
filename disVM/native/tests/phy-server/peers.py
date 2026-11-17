# Copyright (c) 2026 bhgv
# EN: Real client and original server over raw PTY, with producer wakes and exact teardown checkpoints.
# RU: Настоящий client и original server через raw PTY с producer wake и teardown checkpoints.
import os
import selectors
import subprocess
import sys
import tty


# EN/RU: Read one unbuffered checkpoint with a deadline, without sleeps or guessed scheduling.
# Прочитать unbuffered checkpoint с deadline без sleeps/угаданного scheduling.
def line(stream):
    with selectors.DefaultSelector() as ready:
        ready.register(stream, selectors.EVENT_READ)
        if not ready.select(5):
            raise RuntimeError('PHY checkpoint deadline')
    return stream.readline().strip()


# EN/RU: Drive peer RPC and native producer separately, then require original request/fid/client cleanup.
# Управлять peer RPC и producer отдельно, проверить original request/fid/client cleanup.
def main():
    master, slave = os.openpty()
    tty.setraw(slave)
    server = subprocess.Popen([sys.argv[1], str(master)], pass_fds=(master,), bufsize=0,
                              stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    client = None
    try:
        if line(server.stdout) != b'READY':
            raise RuntimeError('server readiness')
        client = subprocess.Popen([sys.argv[2], str(slave), sys.argv[3]], pass_fds=(slave,), bufsize=0,
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        os.close(master)
        master = None
        os.close(slave)
        slave = None
        if line(server.stdout) != b'PENDING' or line(client.stdout) != b'ORDINARY':
            raise RuntimeError('pending read prevented ordinary RPC')
        if sys.argv[3] == 'normal':
            server.stdin.write(b'e')
            server.stdin.flush()
        client.stdin.write(b'g')
        client.stdin.flush()
        output, failure = client.communicate(timeout=5)
        if client.returncode or b'PASS both native PHY peers' not in output:
            raise RuntimeError(str(client.returncode)+repr(output+failure))
        if line(server.stdout) != b'ENDED':
            raise RuntimeError('server end checkpoint')
        server.stdin.write(b'q')
        server.stdin.flush()
        output, failure = server.communicate(timeout=5)
        if server.returncode or b'PASS original PHY server clients=1' not in output or b'retired=1' not in output:
            raise RuntimeError(str(server.returncode)+repr(output+failure))
        print('PASS original server/native client PHY over PTY: '+sys.argv[3])
    finally:
        for child in (client, server):
            if child and child.poll() is None:
                child.kill()
                child.communicate(timeout=5)
        for descriptor in (master, slave):
            if descriptor is not None:
                os.close(descriptor)


if __name__ == '__main__':
    main()

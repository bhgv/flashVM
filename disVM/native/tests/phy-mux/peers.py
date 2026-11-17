# Copyright (c) 2026 bhgv
# EN: Two native symmetric 9P peers, one raw PTY and externally witnessed pending requests.
# RU: Два native symmetric 9P peers, один raw PTY и наблюдаемые pending requests.
import os
import selectors
import subprocess
import sys
import time
import tty


# EN/RU: Wait for each required checkpoint, bounded by one deadline; fail on early exit.
# Дождаться checkpoints под одним deadline, early exit означает failure.
def checkpoints(peers, words):
    pending = {(peer, word) for peer in peers for word in words}
    deadline = time.monotonic() + 8
    selector = selectors.DefaultSelector()
    for peer in peers:
        selector.register(peer.stdout, selectors.EVENT_READ, peer)
    try:
        while pending:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise RuntimeError(f"missing checkpoints: {pending}")
            for key, _ in selector.select(remaining):
                line = key.fileobj.readline()
                if not line:
                    raise RuntimeError(f"early peer EOF: {key.data.poll()}")
                pending.discard((key.data, line.decode().strip()))
    finally:
        selector.close()


# EN/RU: Own descriptors/processes through bounded teardown, no TCP/UDP/socketpair.
# Владеть fd/processes до bounded teardown, без TCP/UDP/socketpair.
def main():
    master, slave = os.openpty()
    tty.setraw(slave)
    peers = []
    try:
        for side, descriptor in enumerate((master, slave)):
            peers.append(subprocess.Popen([sys.argv[1], str(descriptor), str(side), sys.argv[2]],
                pass_fds=(descriptor,), stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                bufsize=0))
        os.close(master)
        os.close(slave)
        master = slave = -1
        checkpoints(peers, ("PENDING", "ORDINARY"))
        for peer in peers:
            peer.stdin.write(b"e")
        checkpoints(peers, ("COMPLETE",))
        for peer in peers:
            peer.stdin.write(b"q")
        checkpoints(peers, ("ENDED",))
        for peer in peers:
            peer.stdin.write(b"t")
        for peer in peers:
            output = peer.communicate(timeout=8)[0]
            if peer.returncode or b"PASS simultaneous" not in output:
                raise RuntimeError(f"peer failed {peer.returncode}: {output!r}")
        print("PASS both original 9P sessions, native events and independent logical close")
    finally:
        for descriptor in (master, slave):
            if descriptor >= 0:
                os.close(descriptor)
        for peer in peers:
            if peer.poll() is None:
                peer.kill()
            peer.wait()


if __name__ == "__main__":
    main()

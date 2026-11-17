# Copyright (c) 2026 bhgv
# EN: Independent PTY server delays original metadata until native cancellation or injects exact failures.
# RU: Независимый PTY server задерживает metadata до native cancellation либо вводит exact failures.
import os
import struct
import subprocess
import sys
import tty
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../lookup"))
from oracle import request, reply, string


# EN/RU: Encode a complete ordinary stat with exact nested lengths; no production codec.
# Кодировать complete stat с точными nested lengths без production codec.
def attributes(directory):
    mode = 0x8000016d if directory else 0o444
    body = struct.pack("<HIBIQIIIQ", 0, 0, 128 if directory else 0, 0, 0,
                       mode, 0, 0, 0)
    body += string("/")+string("fixture")+string("fixture")+string("")
    record = struct.pack("<H", len(body))+body
    return struct.pack("<H", len(record))+record


# EN/RU: Own native processes/fds through bounded failure retirement; cancel only after actual Tstat.
# Владеть process/fds до bounded retirement; cancel только после настоящего Tstat.
def main():
    mode = sys.argv[2]
    master, slave = os.openpty()
    tty.setraw(slave)
    peer = subprocess.Popen([sys.argv[1], str(slave), mode], pass_fds=(slave,), stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    os.close(slave)
    stats = 0
    flushes = 0
    try:
        while True:
            try:
                kind, tag, payload = request(master)
            except EOFError:
                break
            if kind == 100:
                reply(master, 101, tag, struct.pack("<I", 256)+string("9P2000"))
            elif kind == 104:
                reply(master, 105, tag, struct.pack("<BIQ", 0 if mode == "file" else 128, 0, 0))
            elif kind == 110:
                if struct.unpack_from("<H", payload, 8)[0] != 0:
                    raise RuntimeError("unexpected descendant lookup")
                reply(master, 111, tag, struct.pack("<H", 0))
            elif kind == 124:
                stats += 1
                if mode == "loss":
                    os.close(master)
                    master = -1
                    break
                if mode == "badtag":
                    reply(master, 107, (tag+1) % 65535, string("file does not exist"))
                elif (mode == "cancel" and stats == 1) or (mode == "cancelcheck" and stats == 2):
                    peer.stdin.write(b"c")
                    peer.stdin.flush()
                elif mode == "staterror" or (mode == "secondstat" and stats == 2):
                    reply(master, 107, tag, string("permission denied"))
                else:
                    reply(master, 125, tag, attributes(mode != "file"))
            elif kind == 108:
                flushes += 1
                reply(master, 109, tag, b"")
            elif kind == 120:
                reply(master, 121, tag, b"")
            else:
                raise RuntimeError(f"unexpected request {kind}")
        output, error = peer.communicate(timeout=5)
        if peer.returncode or b"PASS unpublished" not in output:
            raise RuntimeError(str(peer.returncode)+repr(output+error))
        if mode in ("cancel", "cancelcheck") and flushes != 1:
            raise RuntimeError(f"metadata cancellation did not use one Tflush: {flushes}")
        print("PASS request acquisition fault: "+mode)
    finally:
        if master >= 0:
            os.close(master)
        if peer.poll() is None:
            peer.kill()
        peer.wait()


if __name__ == "__main__":
    main()

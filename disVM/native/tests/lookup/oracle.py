# Copyright (c) 2026 bhgv
# EN: Independent raw 9P peer over PTY; exact vectors, reverse replies and finite deadlines.
# RU: Независимый raw 9P peer через PTY; exact vectors, reverse replies и finite deadlines.
import os
import selectors
import struct
import subprocess
import sys
import tty


# EN/RU: Read exact borrowed fd bytes under finite waits; EOF/EIO marks peer retirement.
# Прочитать exact fd bytes под finite waits; EOF/EIO означает retirement.
def exact(fd, count):
    result = b""
    while len(result) < count:
        with selectors.DefaultSelector() as ready:
            ready.register(fd, selectors.EVENT_READ)
            if not ready.select(5):
                raise RuntimeError("oracle receive deadline")
        try:
            part = os.read(fd, count-len(result))
        except OSError as error:
            if error.errno != 5:
                raise
            part = b""
        if not part:
            raise EOFError()
        result += part
    return result


# EN/RU: Decode one bounded request header independently of lib9. / Декодировать bounded request header.
def request(fd):
    size = struct.unpack("<I", exact(fd, 4))[0]
    if not 7 <= size <= 256:
        raise RuntimeError("request bound")
    packet = exact(fd, size-4)
    kind, tag = struct.unpack_from("<BH", packet)
    return kind, tag, packet[3:]


# EN/RU: Encode a counted fixture string. / Кодировать counted fixture string.
def string(value):
    data = value.encode()
    return struct.pack("<H", len(data))+data


# EN/RU: Send a fragmented independent response; tag is the actual request lease.
# Послать fragmented response; tag взят из request lease.
def reply(fd, kind, tag, payload):
    packet = struct.pack("<IBH", 7+len(payload), kind, tag)+payload
    while packet:
        count = os.write(fd, packet[:2])
        packet = packet[count:]


# EN/RU: Return the last walk component from checked one/two-element fixture requests.
# Получить last walk component из checked fixture requests.
def component(payload):
    count = struct.unpack_from("<H", payload, 8)[0]
    offset = 10
    names = []
    for _ in range(count):
        size = struct.unpack_from("<H", payload, offset)[0]
        offset += 2
        names.append(payload[offset:offset+size].decode())
        offset += size
    if offset != len(payload):
        raise RuntimeError("walk payload")
    return names


# EN/RU: Exercise real native client; no socket transport or production packet encoder.
# Проверить native client без socket transport и production encoder.
def main():
    mode = sys.argv[2]
    master, slave = os.openpty()
    tty.setraw(slave)
    peer = subprocess.Popen([sys.argv[1], str(slave), mode], pass_fds=(slave,),
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    os.close(slave)
    held = []
    walks = []
    try:
        while True:
            try:
                kind, tag, payload = request(master)
            except EOFError:
                break
            if kind == 100:
                reply(master, 101, tag, struct.pack("<I", 256)+string("9P2000"))
            elif kind == 104:
                reply(master, 105, tag, struct.pack("<BIQ", 128, 0, 0))
            elif kind == 108:
                if mode == "lateerror":
                    reply(master, 107, held[0], string("file does not exist"))
                reply(master, 109, tag, b"")
            elif kind == 120:
                reply(master, 121, tag, b"")
            elif kind == 110:
                names = component(payload)
                walks.append(names)
                if not names:
                    if mode == "cloneerror":
                        reply(master, 107, tag, string("file does not exist"))
                    else:
                        reply(master, 111, tag, struct.pack("<H", 0))
                elif mode == "drop":
                    os.close(master)
                    master = -1
                    break
                elif mode == "wrongtag":
                    reply(master, 107, (tag+1) % 65535, string("file does not exist"))
                elif mode in ("timeout", "lateerror"):
                    held.append(tag)
                elif mode == "race":
                    held.append((tag, names[-1]))
                    if len(held) == 2:
                        for saved_tag, name in reversed(held):
                            reply(master, 107, saved_tag, string("file does not exist" if name == "missing" else "permission denied"))
                elif mode == "partial":
                    reply(master, 111, tag, struct.pack("<HBIQ", 1, 128, 0, 2))
                elif mode == "deep" and names == ["dir"]:
                    reply(master, 111, tag, struct.pack("<HBIQ", 1, 128, 0, 2))
                else:
                    text = {"denied": "permission denied", "unknown": "not found",
                            "prefix": "file does not exist: secret", "empty": ""}.get(mode, "file does not exist")
                    reply(master, 107, tag, string(text))
            else:
                raise RuntimeError(f"unexpected request {kind}")
        output, error = peer.communicate(timeout=5)
        if peer.returncode or b"PASS exact lookup" not in output:
            raise RuntimeError(str(peer.returncode)+repr(output+error))
        if mode == "deep" and walks[:2] != [["dir"], ["leaf"]]:
            raise RuntimeError(f"Inferno lookup lost failure reason: {walks!r}")
        if mode == "cancel" and walks:
            raise RuntimeError("pre-cancel emitted a walk")
        print("PASS independent lookup oracle: "+mode)
    finally:
        if master >= 0:
            os.close(master)
        if peer.poll() is None:
            peer.kill()
        peer.wait()


if __name__ == "__main__":
    main()

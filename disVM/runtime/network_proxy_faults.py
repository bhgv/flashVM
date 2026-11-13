# Copyright (c) 2026 bhgv
# EN: Real-VM wire faults, exact replay, stale authority and admitted-disconnect acceptance.
# RU: Wire faults настоящей VM, exact replay, stale authority и admitted-disconnect acceptance.
from contextlib import contextmanager
import fcntl
import re
import socket
import struct
import time

from network_proxy_fixture import Guest

SIZE = 152  # Exact private wire bytes; never sizeof a host struct.
HIGH, LOW = 12345, 67890  # Test-only externally provisioned connection incarnation.


# EN/RU: Read one bounded record or fail; EOF/partial read is never an RPC reply.
# Прочитать bounded record либо отказать; EOF/partial read не является RPC reply.
def receive(peer):
    data = bytearray()
    while len(data) < SIZE:
        part = peer.recv(SIZE - len(data))
        if not part:
            raise RuntimeError("truncated proxy record")
        data.extend(part)
    return bytes(data)


# EN/RU: Construct canonical independent-oracle record from numeric fields only.
# Построить canonical independent-oracle record только из numeric fields.
def frame(
    kind,
    identity=0,
    operation=0,
    refs=0,
    value=0,
    target=(0, 0, 0),
    left=(0, 0, 0),
    right=(0, 0, 0),
    low=LOW,
):
    values = (SIZE, HIGH, low, identity, operation, refs, value, *target, *left, *right, 0, 0)
    return struct.pack(">4sBBH18Q", b"DVP2", 2, kind, 0, *(v & ((1 << 64) - 1) for v in values))


# EN/RU: Decode one signed scalar at an exact canonical byte offset.
# Читать signed scalar по точному canonical byte offset.
def scalar(data, offset):
    return struct.unpack_from(">q", data, offset)[0]


# EN/RU: Borrow greeting's variable/function handle values; no native addresses exist.
# Заимствовать handle values variable/function из greeting; native addresses отсутствуют.
def handles(greeting):
    if greeting[:8] != b"DVP2\2\1\0\0" or scalar(greeting, 8) != SIZE:
        raise RuntimeError("bad real VM greeting")
    return struct.unpack_from(">3Q", greeting, 64), struct.unpack_from(">3Q", greeting, 88)


# EN/RU: Send one request in real fragments, then verify exact response correlation.
# Передать request настоящими fragments, затем проверить точную reply correlation.
def exchange(peer, request):
    for first in range(0, SIZE, 7):
        peer.sendall(request[first : first + 7])
        time.sleep(0.0002)
    reply = receive(peer)
    if reply[:5] != request[:5] or reply[5] != 3 or reply[6:136] != request[6:136]:
        raise RuntimeError("uncorrelated real server reply")
    return scalar(reply, 136), scalar(reply, 144)


# EN/RU: Own real server/socket until the fault has joined all guest residents.
# Владеть настоящими server/socket до join всех guest residents после fault.
@contextmanager
def server(runtime, work, mode, locked, tag, name="server"):
    peer, child = socket.socketpair()
    peer.settimeout(15)
    guest = Guest(runtime, work, name, mode, locked, child, tag)
    child.close()
    try:
        yield peer, guest, receive(peer)
    finally:
        peer.close()
        guest.abort()


# EN/RU: Replay/loss/conflict/old-ID probes must show one original callback effect.
# Replay/loss/conflict/old-ID проверки должны показать один исходный callback effect.
def replay(runtime, work, mode, locked, tag):
    with server(runtime, work, mode, locked, tag) as (peer, guest, hello):
        variable, function = handles(hello)
        request = frame(2, 1, 3, 2, 3, function, variable, variable)
        if exchange(peer, request) != (0, 16) or exchange(peer, request) != (0, 16):
            raise RuntimeError("exact completed request replayed an owner effect")
        changed = frame(2, 1, 3, 2, 4, function, variable, variable)
        if exchange(peer, changed)[0] != 6:
            raise RuntimeError("conflicting request identity was accepted")
        if exchange(peer, frame(2, 2, 1, target=variable)) != (0, 16):
            raise RuntimeError("replayed/conflicting invocation changed original variable")
        if exchange(peer, request)[0] != 5:
            raise RuntimeError("old request escaped bounded replay window")
        if exchange(peer, frame(2, 3, 5))[0] != 0:
            raise RuntimeError("replay server stop failed")
        guest.finish("result=0 value=16")


# EN/RU: Duplicate Drop consumes no replacement ownership; later Read is stale.
# Duplicate Drop не снимает replacement ownership; последующий Read stale.
def release_replay(runtime, work, mode, locked, tag):
    with server(runtime, work, mode, locked, tag) as (peer, guest, hello):
        variable, _ = handles(hello)
        drop = frame(2, 1, 4, target=variable)
        if exchange(peer, drop)[0] != 0 or exchange(peer, drop)[0] != 0:
            raise RuntimeError("release result not cached")
        if exchange(peer, frame(2, 2, 1, target=variable))[0] == 0:
            raise RuntimeError("released remote variable still resolves")
        exchange(peer, frame(2, 3, 5))
        guest.finish("result=0 value=10")


# EN/RU: Retain replies replay once; each distinct final Release removes one peer owner.
# Retain replies повторяются без нового effect; каждый final Release снимает один owner.
def retain_replay(runtime, work, mode, locked, tag):
    with server(runtime, work, mode, locked, tag) as (peer, guest, hello):
        variable, function = handles(hello)
        for identity, target in ((1, variable), (2, function)):
            retain = frame(2, identity, 6, target=target)
            if exchange(peer, retain)[0] != 0 or exchange(peer, retain)[0] != 0:
                raise RuntimeError("Retain replay did not preserve cached outcome")
        if exchange(peer, frame(2, 3, 4, target=variable))[0] != 0:
            raise RuntimeError("original variable image Release")
        if exchange(peer, frame(2, 4, 4, target=function))[0] != 0:
            raise RuntimeError("original function image Release")
        if exchange(peer, frame(2, 5, 3, 2, 2, function, variable, variable)) != (0, 14):
            raise RuntimeError("distinct retained function/ref owner was revoked")
        exchange(peer, frame(2, 6, 4, target=variable))
        exchange(peer, frame(2, 7, 4, target=function))
        if exchange(peer, frame(2, 8, 1, target=variable))[0] != 5:
            raise RuntimeError("duplicate Retain leaked variable ownership")
        if exchange(peer, frame(2, 9, 3, 2, 1, function, variable, variable))[0] != 5:
            raise RuntimeError("duplicate Retain leaked function ownership")
        exchange(peer, frame(2, 10, 5))
        guest.finish("result=0 value=14")


# EN/RU: Malformed/incomplete/reopened authority never enters the original callback.
# Malformed/incomplete/reopened authority не входит в исходный callback.
def refusal(runtime, work, mode, locked, tag, mutation, name="server"):
    with server(runtime, work, mode, locked, tag, name) as (peer, guest, hello):
        variable, function = handles(hello)
        request = bytearray(frame(2, 1, 3, 2, 9, function, variable, variable))
        mutation(request)
        peer.sendall(request)
        peer.shutdown(socket.SHUT_WR)
        guest.finish("value=10")


# EN: Disconnect after the actual callback published admission; its independent
# native sleep must retire with all export/in-flight roots, without replay.
# RU: Разорвать связь после marker настоящего callback; его native sleep должен
# завершиться со всеми export/in-flight roots, без replay.
def admitted_disconnect(runtime, work, mode, locked, tag):
    with server(runtime, work, mode, locked, tag) as (peer, guest, hello):
        variable, function = handles(hello)
        peer.sendall(frame(2, 1, 3, 2, -999, function, variable, variable))
        deadline = time.monotonic() + 10
        while "NETWORK callback admitted and blocked" not in guest.path.read_text():
            if time.monotonic() >= deadline:
                raise RuntimeError("real network callback never entered")
            time.sleep(0.005)
        peer.shutdown(socket.SHUT_RDWR)
        guest.finish("value=777")


# EN/RU: Trickle bytes cannot renew the server's overall frame deadline.
# Trickle bytes не продлевают общий frame deadline server.
def trickle(runtime, work, mode, locked, tag):
    with server(runtime, work, mode, locked, tag) as (peer, guest, hello):
        variable, _ = handles(hello)
        request = frame(2, 1, 2, value=999, target=variable)
        started = time.monotonic()
        for byte in request:
            if guest.process.poll() is not None:
                break
            try:
                peer.sendall(bytes([byte]))
            except BrokenPipeError:
                break
            time.sleep(0.5)
            if time.monotonic() - started > 10:
                raise RuntimeError("trickle restarted frame deadline")
        guest.finish("value=10")


# EN/RU: Corrupt/lose a real client's reply; terminal connection must not replay.
# Повредить/потерять reply настоящего клиента; terminal connection не повторяет call.
def client_refusal(runtime, work, mode, locked, tag, mutation):
    peer, child = socket.socketpair()
    peer.settimeout(15)
    guest = Guest(runtime, work, "failure-client", mode, locked, child, tag)
    child.close()
    try:
        peer.sendall(frame(1, target=(1, 0, 1), left=(1, 1, 1)))
        response = bytearray(receive(peer))
        response[5] = 3
        struct.pack_into(">q", response, 144, 999)
        mutation(response)
        if response:
            peer.sendall(response)
        else:
            peer.shutdown(socket.SHUT_WR)
        guest.finish("PASS network unknown fenced without replay")
        if peer.recv(1):
            raise RuntimeError("terminal client emitted another wire request")
    finally:
        peer.close()
        guest.abort()


# EN/RU: Kill public IO/session caller while real native reads remain pending.
# Уничтожить public IO/session caller при настоящих pending native reads.
def orphan(runtime, work, mode, locked, tag, name):
    peer, child = socket.socketpair()
    peer.settimeout(15)
    guest = Guest(runtime, work, name, mode, locked, child, tag)
    child.close()
    try:
        if name == "orphan-server":
            handles(receive(peer))
        output = guest.finish("PASS retired")
        if not re.search(r"NATIVEINPUT pins=0 staging=0 peak=152 fragments=[1-9]", output):
            raise RuntimeError("orphan probe never entered actual native input")
    finally:
        peer.close()
        guest.abort()


# EN/RU: Malformed greeting returns failure before publishing any endpoint/images.
# Malformed greeting возвращает отказ до публикации endpoint/images.
def greeting_refusal(runtime, work, mode, locked, tag, mutation):
    peer, child = socket.socketpair()
    peer.settimeout(15)
    guest = Guest(runtime, work, "greeting-client", mode, locked, child, tag)
    child.close()
    try:
        hello = bytearray(frame(1, target=(1, 0, 1), left=(1, 1, 1)))
        mutation(hello)
        peer.sendall(hello)
        guest.finish("PASS bad greeting returns no endpoint or images")
    finally:
        peer.close()
        guest.abort()


# EN/RU: Break backpressured Sys.write and verify shared descriptor flags survive.
# Разорвать backpressured Sys.write и проверить shared descriptor flags.
def output_disconnect(runtime, work, mode, locked, tag):
    peer, child = socket.socketpair()
    flags = fcntl.fcntl(child.fileno(), fcntl.F_GETFL)
    guest = Guest(runtime, work, "socket-output", mode, locked, child, tag)
    try:
        deadline = time.monotonic() + 10
        while "NETWORK socket output entered" not in guest.path.read_text():
            if time.monotonic() >= deadline:
                raise RuntimeError("actual socket output never entered")
            time.sleep(0.005)
        peer.close()
        guest.finish("PASS socket output peer loss returns IO error")
        if fcntl.fcntl(child.fileno(), fcntl.F_GETFL) != flags:
            raise RuntimeError("native socket IO changed shared descriptor flags")
    finally:
        peer.close()
        child.close()
        guest.abort()


# EN/RU: Unused client expires without another byte request or caller Stop.
# Неиспользуемый client истекает без нового byte request и caller Stop.
def idle_client(runtime, work, mode, locked, tag):
    peer, child = socket.socketpair()
    peer.settimeout(15)
    guest = Guest(runtime, work, "idle-client", mode, locked, child, tag)
    child.close()
    try:
        peer.sendall(frame(1, target=(1, 0, 1), left=(1, 1, 1)))
        guest.finish("PASS idle network images are terminal after broker retirement")
        if peer.recv(1):
            raise RuntimeError("idle broker emitted a new wire request")
    finally:
        peer.close()
        guest.abort()

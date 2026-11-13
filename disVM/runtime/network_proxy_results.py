# Copyright (c) 2026 bhgv
# EN: Independent result-handle wire oracle, exact replay and admitted-return disconnect.
# RU: Независимый result-handle wire oracle, exact replay и admitted-return disconnect.
import socket
import struct
import time

from network_proxy_fixture import Guest
from network_proxy_faults import exchange, frame, handles, receive, scalar, server


# EN/RU: One complete result request; compare exact correlation and all extension fields.
# Полный result request; проверить exact correlation и все extension fields.
def result_reply(peer, request, expected_kind):
    peer.sendall(request)
    reply = receive(peer)
    if reply[5] != 3 or reply[:5] != request[:5] or reply[6:136] != request[6:136]:
        raise RuntimeError("uncorrelated callback result")
    if scalar(reply, 136) != 0 or scalar(reply, 144) != 0 or scalar(reply, 152) != expected_kind:
        raise RuntimeError("callback result kind/status/scalar")
    owner = struct.unpack_from(">3Q", reply, 160)
    if owner[0] != scalar(request, 64) or owner[1] > 255 or owner[2] < 1:
        raise RuntimeError("callback result handle identity")
    return reply, owner


# EN: Replay must return the same independently owned variable/function, never
# another export or callback. Final Drop followed by stale proves exact ownership.
# RU: Replay возвращает тот же variable/function, не новый export/callback.
# Final Drop и stale доказывают точное ownership.
def result_replay(runtime, work, mode, locked, tag):
    with server(runtime, work, mode, locked, tag) as (peer, guest, hello):
        variable, function = handles(hello)
        request = frame(2, 1, 7, 2, 71, function, variable, variable)
        first, returned = result_reply(peer, request, 1)
        repeated, again = result_reply(peer, request, 1)
        if first != repeated or again != returned:
            raise RuntimeError("callback result replay created a second export")
        if exchange(peer, frame(2, 2, 1, target=returned)) != (0, 71):
            raise RuntimeError("callback result variable does not resolve")
        exchange(peer, frame(2, 3, 4, target=returned))
        if exchange(peer, frame(2, 4, 1, target=returned))[0] != 5:
            raise RuntimeError("result replay retained extra ownership")
        request = frame(2, 5, 8, 2, 0, function, variable, variable)
        first, callable_owner = result_reply(peer, request, 2)
        repeated, again = result_reply(peer, request, 2)
        if first != repeated or again != callable_owner:
            raise RuntimeError("function result replay created a second capture")
        exchange(peer, frame(2, 6, 4, target=function))
        if exchange(peer, frame(2, 7, 3, 2, 3, callable_owner, variable, variable)) != (0, 16):
            raise RuntimeError("returned function lost original owner/ref arguments")
        exchange(peer, frame(2, 8, 4, target=callable_owner))
        if exchange(peer, frame(2, 9, 3, 2, 1, callable_owner, variable, variable))[0] != 5:
            raise RuntimeError("function result replay leaked its final owner")
        exchange(peer, frame(2, 10, 5))
        guest.finish("result=0 value=16")


# EN/RU: Actual result callback is admitted before connection loss; cleanup owns its pins.
# Actual result callback admitted до обрыва; cleanup владеет его pins.
def result_disconnect(runtime, work, mode, locked, tag):
    with server(runtime, work, mode, locked, tag) as (peer, guest, hello):
        variable, function = handles(hello)
        peer.sendall(frame(2, 1, 7, 2, -999, function, variable, variable))
        deadline = time.monotonic() + 10
        while "NETWORK result callback admitted and blocked" not in guest.path.read_text():
            if time.monotonic() >= deadline:
                raise RuntimeError("result callback did not enter before disconnect")
            time.sleep(0.005)
        peer.shutdown(2)
        guest.finish("value=777")


# EN: Independent peer withholds an already offered result until caller retirement,
# then verifies one GC Drop for that exact result before original function/refs work.
# RU: Independent peer держит offered result до caller retirement и проверяет один
# GC Drop exact result, затем работу original function/refs.
def orphan_result(runtime, work, mode, locked, tag):
    peer, child = socket.socketpair()
    peer.settimeout(15)
    guest = Guest(runtime, work, "orphan-result", mode, locked, child, tag)
    child.close()
    try:
        variable, function, returned = (1, 0, 1), (1, 1, 1), (1, 2, 1)
        peer.sendall(frame(1, target=variable, left=function))
        request = receive(peer)
        if scalar(request, 40) != 7:
            raise RuntimeError("orphan caller did not offer an actual result request")
        deadline = time.monotonic() + 10
        while "PASS retired result caller before reply" not in guest.path.read_text():
            if time.monotonic() >= deadline:
                raise RuntimeError("result caller did not retire before reply")
            time.sleep(0.005)
        reply = bytearray(request)
        reply[5] = 3
        struct.pack_into(">4Q", reply, 152, 1, *returned)
        peer.sendall(reply)
        release = receive(peer)
        if scalar(release, 40) != 4 or struct.unpack_from(">3Q", release, 64) != returned:
            raise RuntimeError("orphan result did not release its exact returned owner")
        reply = bytearray(release)
        reply[5] = 3
        peer.sendall(reply)
        invoke = receive(peer)
        if scalar(invoke, 40) != 3 or struct.unpack_from(">3Q", invoke, 64) != function:
            raise RuntimeError("orphan result replayed or released an original owner")
        reply = bytearray(invoke)
        reply[5] = 3
        struct.pack_into(">q", reply, 144, 16)
        peer.sendall(reply)
        stop = receive(peer)
        if scalar(stop, 40) != 5:
            raise RuntimeError("orphan result emitted another unexpected effect")
        reply = bytearray(stop)
        reply[5] = 3
        peer.sendall(reply)
        guest.finish("PASS orphan result reply image GC released exactly once")
    finally:
        peer.close()
        guest.abort()

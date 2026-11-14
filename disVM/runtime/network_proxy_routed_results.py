# Copyright (c) 2026 bhgv
# EN: Actual three-VM routed result lifetime, independent replay and orphan completion.
# RU: Actual three-VM routed result lifetime, independent replay и orphan completion.
import struct
import threading
import time

from network_proxy_fixture import Guest, connected_tcp
from network_proxy_faults import receive, exchange
from network_proxy_forwarding import forward_request, forwarded_gc
from network_proxy_results import result_reply


# EN: Borrow runtime/work/mode/locked and own A/B/C through clean final joins.
# client selects ordinary/orphan fixture; tag names logs; minimum is C GC evidence.
# RU: Borrow runtime/work/mode/locked; владеть A/B/C до clean final joins.
# client выбирает fixture, tag именует logs, minimum задаёт C GC evidence.
def peers(runtime, work, mode, locked, tag, client="routed-result-client", minimum=9):
    a, b = connected_tcp()
    c, d = connected_tcp()
    guests = []
    try:
        guests.append(Guest(runtime, work, "routed-server", mode, locked, a, tag + "-a"))
        guests.append(Guest(runtime, work, "relay-server", mode, locked, b, tag + "-b", c))
        guests.append(Guest(runtime, work, client, mode, locked, d, tag + "-c"))
        a.close()
        b.close()
        c.close()
        d.close()
        output = guests[2].finish("PASS routed live results:", budget=180)
        middle = guests[1].finish("local=0 source actors joined")
        guests[0].finish("result=0 value=42")
        forwarded_gc(output, minimum)
        forwarded_gc(middle, 2)
    finally:
        for endpoint in (a, b, c, d):
            endpoint.close()
        for guest in guests:
            guest.abort()


# EN: Independent caller retains exact native handles, checks effect/result replay,
# then drops both argument/result counts. Runtime/work/mode/locked select A/B.
# RU: Independent caller удерживает exact native handles, проверяет effect/result
# replay, затем Drop argument/result counts. Runtime/work/mode/locked выбирают A/B.
def oracle(runtime, work, mode, locked, tag):
    a, b = connected_tcp()
    c, peer = connected_tcp()
    guests = []
    try:
        peer.settimeout(30)
        guests.append(Guest(runtime, work, "routed-server", mode, locked, a, tag + "-a"))
        guests.append(Guest(runtime, work, "relay-server", mode, locked, b, tag + "-b", c))
        a.close()
        b.close()
        c.close()
        hello = receive(peer)
        function = struct.unpack_from(">3Q", hello, 88)
        _, view = result_reply(peer, forward_request(1, 11, target=function), 1)
        request = forward_request(2, 12, 2, 2, function, view, view)
        first, returned = result_reply(peer, request, 1)
        repeated, again = result_reply(peer, request, 1)
        if first != repeated or view != returned or again != returned:
            raise RuntimeError("routed result replay changed live identity")
        if exchange(peer, forward_request(2, 12, 2, 3, function, view, view))[0] != 6:
            raise RuntimeError("routed result changed payload reused request")
        exchange(peer, forward_request(3, 4, target=view))
        if exchange(peer, forward_request(4, 1, target=returned)) != (0, 14):
            raise RuntimeError("routed result lost original count or replayed effects")
        exchange(peer, forward_request(5, 4, target=returned))
        if exchange(peer, forward_request(6, 1, target=returned))[0] != 5:
            raise RuntimeError("routed result replay retained extra native owner")
        _, next_view = result_reply(peer, forward_request(7, 11, target=function), 1)
        for identity, sentinel, expected in ((8, -2, 3), (9, -3, 3), (10, -1, -1)):
            if (
                exchange(
                    peer, forward_request(identity, 12, 2, sentinel, function, next_view, next_view)
                )[0]
                != expected
            ):
                raise RuntimeError("routed result failure status changed")
        if exchange(peer, forward_request(11, 1, target=next_view)) != (0, 15):
            raise RuntimeError("routed result failure lost its single partial effect")
        exchange(peer, forward_request(12, 4, target=next_view))
        exchange(peer, forward_request(13, 5))
        guests[1].finish("local=0 source actors joined")
        guests[0].finish("result=0 value=15")
    finally:
        for endpoint in (a, b, c, peer):
            endpoint.close()
        for guest in guests:
            guest.abort()


# EN: Borrow server/caller sockets and work/log paths; signal completed result,
# hold reply until caller retirement in log, append errors and always close stream.
# RU: Borrow server/caller sockets и work/log paths; сообщить completed result,
# ждать retirement caller в log, записывать errors и всегда закрывать stream.
def orphan_relay(server, caller, work, log, errors, lost=False):
    try:
        caller.sendall(receive(server))
        while True:
            request = receive(caller)
            server.sendall(request)
            response = receive(server)
            operation = struct.unpack_from(">Q", request, 40)[0]
            if operation == 12:
                if (
                    struct.unpack_from(">q", response, 136)[0] != 0
                    or struct.unpack_from(">Q", response, 152)[0] != 1
                ):
                    raise RuntimeError("held routed result did not complete successfully")
                if lost:
                    return
                (work / "routed-result-held").write_text("actual B result completed\n")
                deadline = time.monotonic() + 15
                while "PASS routed result caller retired before reply" not in log.read_text():
                    if time.monotonic() >= deadline:
                        raise RuntimeError("routed result caller did not retire before reply")
                    time.sleep(0.005)
            caller.sendall(response)
            if operation == 5:
                return
    except Exception as error:
        errors.append(error)
    finally:
        for endpoint in (server, caller):
            try:
                endpoint.shutdown(2)
            except OSError:
                pass


# EN/RU: Own three runtime/work guests and delayed relay until orphan result GC and joins.
# Владеть тремя runtime/work guests и delayed relay до orphan result GC и joins.
def orphan(runtime, work, mode, locked, tag, lost=False):
    (work / "routed-result-held").unlink(missing_ok=True)
    a, b = connected_tcp()
    c, server = connected_tcp()
    d, caller = connected_tcp()
    guests, errors = [], []
    thread = None
    try:
        for endpoint in (server, caller):
            endpoint.settimeout(30)
        guests.append(Guest(runtime, work, "routed-server", mode, locked, a, tag + "-a"))
        guests.append(Guest(runtime, work, "relay-server", mode, locked, b, tag + "-b", c))
        guests.append(
            Guest(
                runtime,
                work,
                "routed-result-loss-client" if lost else "routed-result-orphan",
                mode,
                locked,
                d,
                tag + "-c",
            )
        )
        for endpoint in (a, b, c, d):
            endpoint.close()
        thread = threading.Thread(
            target=orphan_relay, args=(server, caller, work, guests[2].path, errors, lost)
        )
        thread.start()
        output = guests[2].finish(
            "PASS lost routed result:" if lost else "PASS orphan routed result:"
        )
        middle = guests[1].finish("local=0 source actors joined")
        guests[0].finish("result=0 value=12")
        if not lost:
            forwarded_gc(output, 1)
            forwarded_gc(middle, 2)
        thread.join(timeout=5)
        if thread.is_alive() or errors:
            raise RuntimeError("orphan routed result relay failed: " + repr(errors))
    finally:
        for endpoint in (a, b, c, d, server, caller):
            endpoint.close()
        for guest in guests:
            guest.abort()
        if thread is not None:
            thread.join(timeout=35)
            if thread.is_alive():
                raise RuntimeError("orphan result relay did not join")

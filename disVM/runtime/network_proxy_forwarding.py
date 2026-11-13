# Copyright (c) 2026 bhgv
# EN: Three actual VMs validate opaque forwarding authority and transitive owner cleanup.
# RU: Три actual VMs проверяют opaque forwarding authority и transitive owner cleanup.
import fcntl
import re
import struct

from network_proxy_faults import frame, exchange, receive
from network_proxy_results import result_reply

from network_proxy_fixture import Guest, connected_tcp


# EN/RU: Joined counters prove actual GC at both forwarding hops, not only Stop cleanup.
# Joined counters доказывают actual GC обоих forwarding hops, не только Stop cleanup.
def forwarded_gc(output, minimum):
    match = re.search(r"PROXYGC queues=0 tokens=0 pending=0 enqueued=(\d+) drained=(\d+)", output)
    if not match or int(match[1]) < minimum or int(match[2]) < minimum:
        raise RuntimeError("forwarded route GC did not reach every owner")


# EN: A owns storage, B serves the original function and forwarding registry,
# C owns imported views. B uses independent full-duplex standard sockets; stdout
# carries C protocol and stdin A protocol. Flags are never changed by native IO.
# RU: A владеет storage, B — original function/registry, C — imported views.
# B использует независимые full-duplex standard sockets; stdout несёт C protocol,
# stdin — A protocol. Native IO не меняет flags.
def three_vms(runtime, work, mode, locked, tag, middle_runtime=None):
    a, b = connected_tcp()
    c, d = connected_tcp()
    endpoints = (a, b, c, d)
    flags = [fcntl.fcntl(endpoint, fcntl.F_GETFL) for endpoint in endpoints]
    guests = []
    try:
        guests.append(Guest(runtime, work, "routed-server", mode, locked, a, tag + "-a"))
        guests.append(
            Guest(
                runtime if middle_runtime is None else middle_runtime,
                work,
                "relay-server",
                mode,
                locked,
                b,
                tag + "-b",
                c,
            )
        )
        guests.append(Guest(runtime, work, "relay-client", mode, locked, d, tag + "-c"))
        caller = guests[2].finish("PASS three VM routes:")
        middle = guests[1].finish("local=0 source actors joined")
        guests[0].finish("result=0 value=42")
        forwarded_gc(caller, 2)
        forwarded_gc(middle, 2)
        if [fcntl.fcntl(endpoint, fcntl.F_GETFL) for endpoint in endpoints] != flags:
            raise RuntimeError("full-duplex forwarding changed shared socket flags")
    finally:
        for endpoint in endpoints:
            endpoint.close()
        for guest in guests:
            guest.abort()


# EN/RU: Encode one independent C-to-B request with B's explicit epoch, never native layouts.
# Кодировать independent C-to-B request с explicit epoch B, без native layouts.
def forward_request(
    identity, operation, refs=0, value=0, target=(0, 0, 0), left=(0, 0, 0), right=(0, 0, 0)
):
    data = bytearray(frame(2, identity, operation, refs, value, target, left, right, low=44444))
    struct.pack_into(">Q", data, 16, 33333)
    return data


# EN/RU: Independent caller oracle; actual A/B VMs must enforce replay and exact view identity.
# Независимый caller oracle; actual A/B VMs обязаны enforce replay и exact view identity.
def forwarding_oracle(runtime, work, mode, locked, tag):
    a, b = connected_tcp()
    c, peer = connected_tcp()
    guests = []
    try:
        peer.settimeout(20)
        guests.append(Guest(runtime, work, "routed-server", mode, locked, a, tag + "-a"))
        guests.append(Guest(runtime, work, "relay-server", mode, locked, b, tag + "-b", c))
        a.close()
        b.close()
        c.close()
        hello = receive(peer)
        # B uses a different externally provisioned connection epoch from A.
        if hello[16:32] != (33333).to_bytes(8, "big") + (44444).to_bytes(8, "big"):
            raise RuntimeError("forwarding oracle received wrong B epoch")
        variable, function = struct.unpack_from(">3Q", hello, 64), struct.unpack_from(
            ">3Q", hello, 88
        )

        first = forward_request(1, 11, value=0, target=function)
        original, view = result_reply(peer, first, 1)
        repeated, again = result_reply(peer, first, 1)
        if original != repeated or view != again:
            raise RuntimeError("forwarding import replay created another view")
        wrong_session = (view[0] + 1, view[1], view[2])
        if (
            exchange(peer, forward_request(2, 10, 2, 999, function, wrong_session, wrong_session))[
                0
            ]
            != 5
        ):
            raise RuntimeError("foreign forwarding session accepted")
        invoke = forward_request(3, 10, 2, 2, function, view, view)
        if exchange(peer, invoke) != (0, 14) or exchange(peer, invoke) != (0, 14):
            raise RuntimeError("forwarded callback replayed a source effect")
        if exchange(peer, forward_request(3, 10, 2, 3, function, view, view))[0] != 6:
            raise RuntimeError("forwarded callback payload conflict accepted")
        wrong_generation = (view[0], view[1], view[2] + 1)
        if (
            exchange(
                peer, forward_request(4, 10, 2, 999, function, wrong_generation, wrong_generation)
            )[0]
            != 5
        ):
            raise RuntimeError("foreign forwarding generation accepted")
        retain = forward_request(5, 6, target=view)
        if exchange(peer, retain)[0] != 0 or exchange(peer, retain)[0] != 0:
            raise RuntimeError("forwarding Retain replay")
        exchange(peer, forward_request(6, 4, target=view))
        if exchange(peer, forward_request(7, 1, target=view)) != (0, 14):
            raise RuntimeError("forwarding original Drop revoked retained owner")
        exchange(peer, forward_request(8, 4, target=view))
        if exchange(peer, forward_request(9, 1, target=view))[0] != 5:
            raise RuntimeError("forwarding Retain replay leaked ownership")
        _, replacement = result_reply(peer, forward_request(10, 11, target=function), 1)
        if replacement == view:
            raise RuntimeError("replacement forwarding view reused exact identity")
        if exchange(peer, forward_request(11, 10, 2, 999, function, view, view))[0] != 5:
            raise RuntimeError("retired forwarding view selected replacement")
        if exchange(peer, forward_request(12, 10, 2, 1, function, replacement, replacement)) != (
            0,
            16,
        ):
            raise RuntimeError("replacement forwarding view lost source storage")
        exchange(peer, forward_request(13, 4, target=replacement))
        exchange(peer, forward_request(14, 5))
        guests[1].finish("local=0 source actors joined")
        guests[0].finish("result=0 value=16")
    finally:
        for endpoint in (a, b, c, peer):
            endpoint.close()
        for guest in guests:
            guest.abort()


# EN: Lose an acknowledged A effect on either hop. Source loss closes A/B; caller
# loss closes B/C before B receives its first Add reply, then lets B finish once.
# RU: Потерять acknowledged effect A на одном из hops. Source loss закрывает A/B;
# caller loss закрывает B/C до первого Add reply B, затем даёт B закончить один раз.
def source_relay(source, middle, caller, errors, mode):
    try:
        middle.sendall(receive(source))
        lost = False
        while True:
            request = receive(middle)
            source.sendall(request)
            response = receive(source)
            operation = struct.unpack_from(">Q", request, 40)[0]
            if operation == 9 and not lost:
                lost = True
                if mode == "source":
                    return
                caller.shutdown(2)
            middle.sendall(response)
            if operation == 5:
                break
    except Exception as error:
        errors.append(error)
    finally:
        for endpoint in (source, middle):
            try:
                endpoint.shutdown(2)
            except OSError:
                pass


# EN/RU: Three real VMs and an independent loss gate; join every process/thread on errors.
# Три actual VMs и independent loss gate; join всех process/thread при errors.
def forwarding_loss(runtime, work, mode, locked, tag, lost_hop):
    import threading

    a, source = connected_tcp()
    b, middle = connected_tcp()
    c, caller = connected_tcp()
    endpoints = (a, source, b, middle, c, caller)
    guests, errors = [], []
    thread = None
    try:
        for endpoint in (source, middle):
            endpoint.settimeout(25)
        guests.append(Guest(runtime, work, "routed-server", mode, locked, a, tag + "-a"))
        guests.append(Guest(runtime, work, "relay-server", mode, locked, b, tag + "-b", c))
        guests.append(
            Guest(
                runtime,
                work,
                "relay-" + lost_hop + "-loss-client",
                mode,
                locked,
                caller,
                tag + "-c",
            )
        )
        thread = threading.Thread(
            target=source_relay, args=(source, middle, caller, errors, lost_hop)
        )
        thread.start()
        guests[2].finish("PASS forwarding " + lost_hop + " loss:")
        guests[1].finish("local=0 source actors joined")
        guests[0].finish("value=11" if lost_hop == "source" else "value=12")
        thread.join(timeout=5)
        if thread.is_alive() or errors:
            raise RuntimeError("forwarding loss gate failed: " + repr(errors))
    finally:
        for endpoint in endpoints:
            endpoint.close()
        for guest in guests:
            guest.abort()
        if thread is not None:
            thread.join(timeout=30)
            if thread.is_alive():
                raise RuntimeError("forwarding loss relay did not join")


# EN/RU: Hold a completed B import until C's caller retires, then preserve exact records.
# Держать completed B import до caller retirement C, затем сохранять exact records.
def import_relay(server, caller, log, errors):
    import time

    try:
        caller.sendall(receive(server))
        held = False
        while True:
            request = receive(caller)
            server.sendall(request)
            response = receive(server)
            operation = struct.unpack_from(">Q", request, 40)[0]
            if operation == 11 and not held:
                deadline = time.monotonic() + 10
                while "PASS retired forwarded import caller before reply" not in log.read_text():
                    if time.monotonic() >= deadline:
                        raise RuntimeError(
                            "forwarded import caller never retired before completed reply"
                        )
                    time.sleep(0.005)
                held = True
            caller.sendall(response)
            if operation == 5:
                break
    except Exception as error:
        errors.append(error)
    finally:
        for endpoint in (server, caller):
            try:
                endpoint.shutdown(2)
            except OSError:
                pass


# EN/RU: Three actual VMs prove late imported-image GC propagates through B to A.
# Три actual VMs доказывают late imported-image GC через B до A.
def orphan_import(runtime, work, mode, locked, tag):
    import threading

    a, b = connected_tcp()
    c, server = connected_tcp()
    d, caller = connected_tcp()
    endpoints = (a, b, c, server, d, caller)
    for endpoint in (server, caller):
        endpoint.settimeout(25)
    guests, errors = [], []
    thread = None
    try:
        guests.append(Guest(runtime, work, "routed-server", mode, locked, a, tag + "-a"))
        guests.append(Guest(runtime, work, "relay-server", mode, locked, b, tag + "-b", c))
        guests.append(Guest(runtime, work, "relay-orphan-client", mode, locked, d, tag + "-c"))
        thread = threading.Thread(
            target=import_relay, args=(server, caller, guests[2].path, errors)
        )
        thread.start()
        output = guests[2].finish("PASS orphan forwarding import:")
        middle = guests[1].finish("local=0 source actors joined")
        guests[0].finish("result=0 value=42")
        forwarded_gc(output, 3)
        forwarded_gc(middle, 3)
        thread.join(timeout=5)
        if thread.is_alive() or errors:
            raise RuntimeError("orphan forwarding import relay failed: " + repr(errors))
    finally:
        for endpoint in endpoints:
            endpoint.close()
        for guest in guests:
            guest.abort()
        if thread is not None:
            thread.join(timeout=30)
            if thread.is_alive():
                raise RuntimeError("orphan forwarding relay did not join")

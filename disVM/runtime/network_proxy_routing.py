# Copyright (c) 2026 bhgv
# EN: Two actual VMs with a reply gate proving concurrent original callback owners.
# RU: Две actual VMs с reply gate, доказывающим concurrent original callback owners.
import re
import struct
import threading
import time

from network_proxy_fixture import Guest, connected_tcp
from network_proxy_faults import receive, exchange, frame, handles, server


# EN: Relay canonical records between real VMs. Hold the first Add response until
# two distinct original function Progs have entered callbacks in the client VM.
# This gate proves overlap before reply; sleep-only or sequential sums do not.
# RU: Передавать canonical records между actual VMs. Держать первый Add response
# до входа двух разных original function Progs в callbacks client VM. Gate доказывает
# overlap до reply; одни sleep либо sequential sums этого не доказывают.
def relay(server, client, log, errors, overlap, lose_reply):
    try:
        client.sendall(receive(server))
        while True:
            request = receive(client)
            server.sendall(request)
            response = receive(server)
            operation = struct.unpack_from(">Q", request, 40)[0]
            if operation == 9 and not overlap:
                deadline = time.monotonic() + 10
                while True:
                    owners = set(
                        re.findall(r"ROUTED callback entered owner=(\d+)", log.read_text())
                    )
                    if len(owners) >= 2:
                        overlap.append(True)
                        break
                    if time.monotonic() >= deadline:
                        raise RuntimeError("function owners did not overlap before routed reply")
                    time.sleep(0.005)
            if operation == 9 and lose_reply:
                return
            client.sendall(response)
            if operation == 5:
                break
    except Exception as error:
        errors.append(error)
    finally:
        for endpoint in (server, client):
            try:
                endpoint.shutdown(2)
            except OSError:
                pass


# EN/RU: Joined native diagnostics must show actual last-wrapper GC enqueue/drain.
# Joined native diagnostics должны показать actual last-wrapper GC enqueue/drain.
def check_ref_gc(output):
    count = re.search(r"PROXYGC queues=0 tokens=0 pending=0 enqueued=(\d+) drained=(\d+)", output)
    if not count or int(count[1]) < 1 or int(count[2]) < 1:
        raise RuntimeError("cross-owner ref GC did not reach actor")


# EN/RU: Own two real TCP connections, native guests and relay thread through final join.
# Владеть двумя actual TCP connections, native guests и relay thread до final join.
def routed_pair(runtime, work, mode, locked, tag, lose_reply=False):
    server_socket, server_relay = connected_tcp()
    client_socket, client_relay = connected_tcp()
    guests = []
    thread = None
    errors, overlap = [], []
    try:
        for endpoint in (server_relay, client_relay):
            endpoint.settimeout(20)
        guests.append(
            Guest(runtime, work, "routed-server", mode, locked, server_socket, tag + "-server")
        )
        guests.append(
            Guest(
                runtime,
                work,
                "routed-failure-client" if lose_reply else "routed-client",
                mode,
                locked,
                client_socket,
                tag + "-client",
            )
        )
        server_socket.close()
        client_socket.close()
        thread = threading.Thread(
            target=relay,
            args=(server_relay, client_relay, guests[1].path, errors, overlap, lose_reply),
        )
        thread.start()
        try:
            marker = (
                "PASS routed disconnect: uncertain effect no replay two callbacks unrelated owner join"
                if lose_reply
                else "PASS cross-owner proxy refs: original callback 8 clients atomic aliases Drop GC join"
            )
            output = guests[1].finish(marker)
        except RuntimeError:
            if errors:
                raise RuntimeError(str(errors[0]))
            raise
        thread.join(timeout=5)
        if thread.is_alive() or errors or not overlap:
            raise RuntimeError("routed reply gate failed: " + repr(errors))
        if not lose_reply:
            check_ref_gc(output)
        guests[0].finish("value=11" if lose_reply else "result=0 value=42")
    finally:
        for endpoint in (server_socket, client_socket, server_relay, client_relay):
            endpoint.close()
        for guest in guests:
            guest.abort()
        if thread is not None:
            thread.join(timeout=25)
            if thread.is_alive():
                raise RuntimeError("routed relay thread did not join")


# EN/RU: One original-owner atomic update and exact replay; stale updates cannot revive exports.
# Один original-owner atomic update и exact replay; stale updates не оживляют exports.
def add_replay(runtime, work, mode, locked, tag):
    with server(runtime, work, mode, locked, tag) as (peer, guest, hello):
        variable, unused = handles(hello)
        update = frame(2, 1, 9, value=3, target=variable)
        if exchange(peer, update) != (0, 13) or exchange(peer, update) != (0, 13):
            raise RuntimeError("atomic routed Add replayed its original effect")
        if exchange(peer, frame(2, 1, 9, value=4, target=variable))[0] != 6:
            raise RuntimeError("changed atomic update reused its request ID")
        if exchange(peer, frame(2, 2, 1, target=variable)) != (0, 13):
            raise RuntimeError("routed Add did not update original storage")
        exchange(peer, frame(2, 3, 4, target=variable))
        if exchange(peer, frame(2, 4, 9, value=9, target=variable))[0] != 5:
            raise RuntimeError("stale atomic update revived released storage")
        exchange(peer, frame(2, 5, 5))
        guest.finish("value=13")

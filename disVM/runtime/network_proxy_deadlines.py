# Copyright (c) 2026 bhgv
# EN: Actual queued caller expiry must not become a later original-owner effect.
# RU: Actual queued caller expiry не должен становиться поздним original-owner effect.
import struct
import threading
import time

from network_proxy_fixture import Guest, compile_one, connected_tcp
from network_proxy_faults import receive


# EN: Relay real owner responses, gating the first Read until the queued Write
# caller expires. Keep all subsequent frames intact; the owner detects effects.
# RU: Передавать real owner responses; первый Read держать до expiry queued Write.
# Последующие frames передавать без изменений; actual owner наблюдает effects.
def relay(server, client, work, log, errors):
    try:
        client.sendall(receive(server))
        first = True
        while True:
            request = receive(client)
            server.sendall(request)
            response = receive(server)
            if first:
                if struct.unpack_from(">Q", request, 40)[0] != 1:
                    raise RuntimeError("deadline gate did not receive first Read")
                (work / "queued-ready").write_text("first Read held\n")
                deadline = time.monotonic() + 15
                while "QUEUED expired caller retired" not in log.read_text():
                    if time.monotonic() >= deadline:
                        raise RuntimeError("queued caller did not retire before first reply")
                    time.sleep(0.005)
                first = False
            client.sendall(response)
            if struct.unpack_from(">Q", request, 40)[0] == 5:
                return
    except Exception as error:
        errors.append(error)
    finally:
        for endpoint in (server, client):
            try:
                endpoint.shutdown(2)
            except OSError:
                pass


# EN: Own two real VMs and a delayed-reply relay through joins. A private renewed
# deadline must produce exactly the observed 999 write, never timeout/crash proof.
# RU: Владеть двумя real VMs и delayed-reply relay до joins. Private renewed deadline
# обязан дать ровно observed 999 write, а не timeout/crash как доказательство.
def queued_deadline(runtime, work, mode, locked, tag, renewed=False):
    server, server_peer = connected_tcp()
    client, client_peer = connected_tcp()
    guests = []
    thread = None
    errors = []
    (work / "queued-ready").unlink(missing_ok=True)
    try:
        for endpoint in (server_peer, client_peer):
            endpoint.settimeout(20)
        guests.append(Guest(runtime, work, "routed-server", mode, locked, server, tag + "-a"))
        guests.append(
            Guest(runtime, work, "queued-deadline-client", mode, locked, client, tag + "-b")
        )
        server.close()
        client.close()
        thread = threading.Thread(
            target=relay, args=(server_peer, client_peer, work, guests[1].path, errors)
        )
        thread.start()
        if renewed:
            try:
                guests[1].finish()
            except RuntimeError as error:
                if guests[1].process.returncode != 1 or (
                    "expired queued write executed or Drop consumed original lease"
                    not in str(error)
                ):
                    raise
            else:
                raise RuntimeError("renewed queued deadline escaped")
        else:
            guests[1].finish("PASS queued deadline:")
        guests[0].finish("value=999" if renewed else "result=0 value=10")
        thread.join(timeout=5)
        # EOF is expected only when the negative client rejects the actual late effect.
        if thread.is_alive() or (errors and not renewed):
            raise RuntimeError("queued deadline relay failed: " + repr(errors))
    finally:
        for endpoint in (server, client, server_peer, client_peer):
            endpoint.close()
        for guest in guests:
            guest.abort()
        if thread is not None:
            thread.join(timeout=25)
            if thread.is_alive():
                raise RuntimeError("queued deadline relay did not join")


# EN/RU: Renew only queued request deadline in isolated bytecode; require actual late effect.
# Обновить лишь queued request deadline в isolated bytecode; требовать actual late effect.
def controls(runtime, root, work, compiler, include):
    path = root.parent / "limbo/proxynet.b"
    source = path.read_text()
    anchor = "refs, value, deadline, response)"
    if source.count(anchor) != 1:
        raise RuntimeError("queued request deadline anchor moved")
    private = work / "renewed-queue-deadline.b"
    private.write_text(source.replace(anchor, "refs, value, deadline + 60000, response)"))
    try:
        compile_one(root, work, compiler, include, private, "proxynet")
        queued_deadline(runtime, work, "--no-jit", False, "renewed-deadline", True)
    finally:
        compile_one(root, work, compiler, include, path, "proxynet")
    print("PASS queued deadline omission: actual late Write=999 rejected")


# EN/RU: Actual original owner queue must not execute a retired caller's expired Write.
# Actual original owner queue не исполняет expired Write retired caller.
def owner_deadline(runtime, work, mode, locked, tag, omitted=False):
    for name in ("owner-entered", "owner-release"):
        (work / name).unlink(missing_ok=True)
    with (work / "owner-input").open("wb"):
        pass
    with (work / "owner-input").open("rb") as empty:
        guest = Guest(runtime, work, "owner-deadline-client", mode, locked, empty, tag)
        try:
            if omitted:
                try:
                    guest.finish()
                except RuntimeError as error:
                    if guest.process.returncode != 1 or (
                        "expired original-owner Write executed: value=999" not in str(error)
                    ):
                        raise
                else:
                    raise RuntimeError("original-owner deadline omission escaped")
            else:
                guest.finish("PASS original owner deadline:")
        finally:
            guest.abort()


# EN/RU: Remove only original-owner expiry guard in private bytecode; detect actual Write=999.
# Удалить лишь original-owner expiry guard в private bytecode; обнаружить actual Write=999.
def owner_control(runtime, root, work, compiler, include):
    path = root.parent / "limbo/proxyowner.b"
    source = path.read_text()
    anchor = "if (request.deadline <= sys->millisec()) {"
    if source.count(anchor) != 1:
        raise RuntimeError("original-owner deadline control anchor moved")
    private = work / "owner-expiry-omission.b"
    private.write_text(source.replace(anchor, "if (0 && request.deadline <= sys->millisec()) {"))
    try:
        compile_one(root, work, compiler, include, private, "proxyowner")
        owner_deadline(runtime, work, "--no-jit", False, "owner-omission", True)
    finally:
        compile_one(root, work, compiler, include, path, "proxyowner")
    print("PASS original-owner expiry omission: actual late Write=999 rejected", flush=True)

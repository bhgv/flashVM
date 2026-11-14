# Copyright (c) 2026 bhgv
# EN: Delay actual owner/source handoffs after budget capture and prove deadlines are never renewed.
# RU: Задержать actual owner/source handoffs после budget capture; доказать отсутствие deadline renewal.
import socket
import threading

from network_proxy_fixture import Guest, connected_tcp, compile_one
from network_proxy_faults import frame, receive, handles, scalar
from disconnect_waiter_cases import negative


# EN/RU: Compile matched source clients and a short-budget actual server into borrowed work.
# Собрать matched source clients и short-budget actual server в borrowed work.
def prepare(root, work, compiler, include):
    path = root / "fixtures/proxy-network/handoff-route-client.b"
    compile_one(root, work, compiler, include, path, "handoff-write-client")
    source = path.read_text()
    if source.count("if (!exercise(0)) {") != 1:
        raise RuntimeError("handoff startup client anchor moved")
    private = work / "handoff-startup-client.b"
    private.write_text(source.replace("if (!exercise(0)) {", "if (!exercise(1)) {"))
    compile_one(root, work, compiler, include, private, "handoff-startup-client")
    source = (root / "fixtures/proxy-network/server.b").read_text()
    if source.count(", 5000, handler,") != 1:
        raise RuntimeError("handoff owner timeout anchor moved")
    private = work / "handoff-owner-server.b"
    private.write_text(source.replace(", 5000, handler,", ", 1000, handler,"))
    compile_one(root, work, compiler, include, private, "handoff-owner-server")


# EN: Borrow production text; insert only one sleeping handoff window, optionally
# restoring the old relative-budget call. Return private bytecode source.
# RU: Borrow production text; добавить лишь sleeping handoff window, при omitted
# вернуть old relative-budget call. Вернуть private bytecode source.
def variant(source, role, omitted):
    if role == "write":
        anchor = "(status, value) = net->calluntil(source, operation, image, empty, empty, 0, access.value, deadline);"
        replacement = (
            """if (access.operation == Proxyref->Write && access.value == 999) {
                        sys->sleep(1500);
                    }
                    """
            + (
                anchor.replace("calluntil", "call").replace("value, deadline", "value, remaining")
                if omitted
                else anchor
            )
            + """
                    if (access.operation == Proxyref->Write && access.value == 999) {
                        gate := sys->create("handoff-route-finished", Sys->OWRITE, 8r666);
                        if (gate == nil) {
                            raise "handoff completion publication";
                        }
                        gate = nil;
                    }"""
        )
    elif role == "startup":
        anchor = "(status, copy) := net->cloneuntil(source, setup.image, deadline);"
        replacement = "sys->sleep(1500);\n    " + (
            anchor.replace("cloneuntil", "clone").replace("image, deadline", "image, remaining")
            if omitted
            else anchor
        )
    else:
        anchor = (
            "(status, returned) = proxy->fetchuntil(endpoint,"
            if role == "result"
            else "(status, value) = proxy->calluntil(endpoint, operation,"
        )
        replacement = "sys->sleep(1500);\n                " + anchor
        if omitted:
            replacement = replacement.replace("fetchuntil", "fetch").replace("calluntil", "call")
            tail = (
                "int codec->get(bytes, Proxyframe->Value), kind, deadline);"
                if role == "result"
                else "int codec->get(bytes, Proxyframe->Value), deadline);"
            )
            start = source.index(anchor)
            suffix = source[start:]
            if suffix.count(tail) != 1:
                raise RuntimeError("owner deadline tail anchor moved")
            source = source[:start] + suffix.replace(tail, tail.replace("deadline", "remaining"))
    if source.count(anchor) != 1:
        raise RuntimeError("handoff delay anchor moved: " + role)
    return source.replace(anchor, replacement)


# EN/RU: Observe actual source wire order and relay every record through Stop; own neither VM.
# Наблюдать actual source wire order и передавать все records до Stop; не владеть VM.
def relay(server, client, operations, errors):
    try:
        client.sendall(receive(server))
        while True:
            record = receive(client)
            operations.append(scalar(record, 40))
            server.sendall(record)
            client.sendall(receive(server))
            if operations[-1] == 5:
                return
    except Exception as error:
        errors.append(error)
    finally:
        for endpoint in (server, client):
            try:
                endpoint.shutdown(2)
            except OSError:
                pass


# EN/RU: Require exact semantic failure after native/GC/root cleanup, not a crash or timeout.
# Требовать exact semantic failure после native/GC/root cleanup, не crash/timeout.
def refused(guest):
    try:
        guest.finish()
    except RuntimeError as error:
        if guest.process.returncode != 1 or "source handoff renewed original deadline" not in str(
            error
        ):
            raise
        output = guest.path.read_text()
        if "ThreadSanitizer:" in output or "runtime error:" in output:
            raise RuntimeError("handoff omission failed outside semantic boundary")
        negative(output)
    else:
        raise RuntimeError("handoff deadline renewal escaped detection")


# EN: Own two actual VMs and wire observer through expired Write/startup and clean
# clone/original ownership. Omission must prove actual A=999 or a late wire Retain.
# RU: Владеть двумя actual VMs и wire observer до expired Write/startup и clean
# clone/original ownership. Omission доказывает actual A=999 либо late wire Retain.
def source(runtime, work, mode, locked, tag, role, omitted):
    (work / "handoff-route-finished").unlink(missing_ok=True)
    a, server = connected_tcp()
    b, client = connected_tcp()
    guests, operations, errors = [], [], []
    thread = None
    try:
        server.settimeout(20)
        client.settimeout(20)
        guests.append(Guest(runtime, work, "routed-server", mode, locked, a, tag + "-a"))
        guests.append(
            Guest(runtime, work, "handoff-" + role + "-client", mode, locked, b, tag + "-b")
        )
        a.close()
        b.close()
        thread = threading.Thread(target=relay, args=(server, client, operations, errors))
        thread.start()
        if omitted:
            refused(guests[1])
        else:
            guests[1].finish("PASS source handoff:")
        guests[0].finish("value=999" if omitted and role == "write" else "value=10")
        thread.join(timeout=5)
        if thread.is_alive() or errors:
            raise RuntimeError("handoff wire observer failed: " + repr(errors))
        if role == "write" and ((2 in operations) != omitted):
            raise RuntimeError("handoff Write wire effect did not match original deadline")
        if role == "startup" and ((6 in operations) != omitted):
            raise RuntimeError("handoff startup Retain did not match original deadline")
    finally:
        for endpoint in (a, b, server, client):
            endpoint.close()
        for guest in guests:
            guest.abort()
        if thread is not None:
            thread.join(timeout=25)
            if thread.is_alive():
                raise RuntimeError("handoff wire observer did not join")


# EN/RU: Delay actual server-to-owner Write or typed result admission; verify original A value.
# Задержать actual server-to-owner Write либо typed result admission; проверить original A value.
def owner(runtime, work, mode, locked, tag, role, omitted):
    peer, child = socket.socketpair()
    peer.settimeout(15)
    guest = Guest(runtime, work, "handoff-owner-server", mode, locked, child, tag)
    child.close()
    try:
        variable, function = handles(receive(peer))
        operation = 2 if role == "owner" else (7 if role == "variable" else 8)
        record = (
            frame(2, 1, operation, value=999, target=variable)
            if role == "owner"
            else frame(2, 1, operation, 2, -889, function, variable, variable)
        )
        peer.sendall(record)
        peer.shutdown(socket.SHUT_WR)
        guest.finish("value=" + ("999" if role == "owner" else "11") if omitted else "value=10")
        if peer.recv(1):
            raise RuntimeError("expired owner handoff published wire reply after frame deadline")
    finally:
        peer.close()
        guest.abort()


# EN: Own one private handoff gap library at a time, run actual positive/omission
# paths and restore matched production bytecode on every terminal path.
# RU: Владеть одной private handoff gap library, исполнить actual positive/omission
# paths и восстановить matched production bytecode на каждом terminal path.
def check(runtime, root, work, compiler, include, mode, locked, role, omitted=False):
    name = "proxyroute" if role in ("write", "startup") else "proxyserver"
    path = root.parent / "limbo" / (name + ".b")
    private = work / (role + "-handoff-gap.b")
    private.write_text(variant(path.read_text(), role, omitted))
    try:
        compile_one(root, work, compiler, include, private, name)
        tag = role + mode[2:] + str(locked) + str(omitted)
        if role in ("write", "startup"):
            source(runtime, work, mode, locked, tag, role, omitted)
        elif role == "result":
            for kind in ("variable", "function"):
                owner(runtime, work, mode, locked, tag + kind, kind, omitted)
        else:
            owner(runtime, work, mode, locked, tag, role, omitted)
    finally:
        compile_one(root, work, compiler, include, path, name)
    print(
        "PASS handoff "
        + role
        + (" renewal omission detected" if omitted else " original deadline preserved"),
        flush=True,
    )

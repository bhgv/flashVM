# Copyright (c) 2026 bhgv
# EN: Gate actual Drop replies to prove independent source cleanup is concurrent.
# RU: Удержать actual Drop replies, чтобы доказать concurrent cleanup independent sources.
import struct
import threading

from network_proxy_fixture import Guest, connected_tcp, compile_one
from network_proxy_faults import receive


# EN/RU: Compile the matched three-VM cleanup coordinator into borrowed work.
# Собрать matched three-VM cleanup coordinator в borrowed work.
def prepare(root, work, compiler, include):
    path = root / "fixtures/proxy-network/parallel-cleanup-client.b"
    compile_one(root, work, compiler, include, path, "parallel-cleanup-client")


# EN: Forward one source, hold its first completed Drop until the other independent
# Drop arrives. Record the ordering before bounded fallback releases broken cleanup.
# RU: Передавать один source, удержать first completed Drop до другого independent
# Drop. Записать ordering до bounded fallback; loss теряет один completed reply.
def relay(server, client, index, lock, seen, released, ordered, errors, loss):
    try:
        client.sendall(receive(server))
        held = False
        while True:
            request = receive(client)
            server.sendall(request)
            response = receive(server)
            operation = struct.unpack_from(">Q", request, 40)[0]
            if operation == 4 and not held:
                if struct.unpack_from(">q", response, 136)[0] != 0:
                    raise RuntimeError("actual source clone Drop did not succeed")
                with lock:
                    seen.add(index)
                    if len(seen) == 2:
                        released.set()
                released.wait(timeout=3)
                with lock:
                    ordered.append(len(seen) == 2)
                held = True
                if loss and index == 0:
                    return
            client.sendall(response)
            if operation == 5:
                return
    except Exception as error:
        errors.append(error)
        released.set()
    finally:
        for endpoint in (server, client):
            try:
                endpoint.shutdown(2)
            except OSError:
                pass


# EN: Own two real source VMs, one coordinator and both relays until all roots join.
# serialized requires missing parallel-offer ordering; loss disconnects source zero
# after its completed clone Drop while source one continues to normal Stop.
# RU: Владеть двумя actual source VMs, coordinator и relays до join всех roots.
# serialized требует missing parallel-offer ordering; loss отключает source zero
# после completed clone Drop, пока source one доходит до normal Stop.
def run(runtime, work, mode, locked, tag, serialized=False, loss=False):
    sockets, guests, threads, errors = [], [], [], []
    lock, seen, released, ordered = threading.Lock(), set(), threading.Event(), []
    endpoints = []
    try:
        for index in range(2):
            a, server = connected_tcp()
            b, client = connected_tcp()
            sockets.extend((a, server, b, client))
            server.settimeout(30)
            client.settimeout(30)
            guests.append(
                Guest(runtime, work, "routed-server", mode, locked, a, tag + "-a" + str(index))
            )
            a.close()
            endpoints.append(b)
            threads.append(
                threading.Thread(
                    target=relay,
                    args=(server, client, index, lock, seen, released, ordered, errors, loss),
                )
            )
        guests.append(
            Guest(
                runtime,
                work,
                "parallel-cleanup-client",
                mode,
                locked,
                endpoints[0],
                tag + "-b",
                output=endpoints[1],
            )
        )
        for endpoint in endpoints:
            endpoint.close()
        for thread in threads:
            thread.start()
        guests[2].finish("PASS parallel cleanup:", budget=40)
        for guest in guests[:2]:
            guest.finish("value=10")
        for thread in threads:
            thread.join(timeout=5)
        if (
            any(thread.is_alive() for thread in threads)
            or errors
            or seen != {0, 1}
            or len(ordered) != 2
        ):
            raise RuntimeError("parallel cleanup relay failed: " + repr(errors))
        if serialized:
            if all(ordered):
                raise RuntimeError("serialized source cleanup escaped ordering detection")
        elif not all(ordered):
            raise RuntimeError("second cleanup Drop was not offered before first reply release")
    finally:
        released.set()
        for endpoint in sockets:
            endpoint.close()
        for guest in guests:
            guest.abort()
        for thread in threads:
            if thread.ident is not None:
                thread.join(timeout=35)
                if thread.is_alive():
                    raise RuntimeError("parallel cleanup relay did not join")


# EN/RU: Replace only cleanup spawn with direct call in private bytecode; require ordering failure.
# Заменить лишь cleanup spawn прямым call в private bytecode; требовать ordering failure.
def control(runtime, root, work, compiler, include):
    path = root.parent / "limbo/proxyregistry.b"
    source = path.read_text()
    anchor = "spawn closeref(view.source, deadline, completed);"
    if source.count(anchor) != 1:
        raise RuntimeError("parallel source cleanup anchor moved")
    private = work / "serial-cleanup-registry.b"
    private.write_text(source.replace(anchor, "closeref(view.source, deadline, completed);"))
    try:
        compile_one(root, work, compiler, include, private, "proxyregistry")
        run(runtime, work, "--no-jit", False, "serial-cleanup-omission", True)
    finally:
        compile_one(root, work, compiler, include, path, "proxyregistry")
    print(
        "PASS cleanup omission: actual second Drop missing before first reply release", flush=True
    )

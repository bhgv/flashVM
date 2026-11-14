# Copyright (c) 2026 bhgv
# EN: Routed result ownership omissions must produce exact actual-owner failures.
# RU: Routed result ownership omissions обязаны давать exact actual-owner failures.
import struct

from network_proxy_fixture import Guest, compile_one, connected_tcp
from network_proxy_faults import frame, receive
from relink_private import relink


# EN: Own actual A/B/C runtime/work guests until intended client rejection and clean
# server joins. tag/marker select evidence; expected is actual final A storage.
# RU: Владеть actual A/B/C runtime/work guests до intended client rejection и clean
# server joins. tag/marker выбирают evidence, expected задаёт actual final A storage.
def rejected(runtime, work, tag, marker, expected):
    a, b = connected_tcp()
    c, d = connected_tcp()
    guests = []
    try:
        guests.append(Guest(runtime, work, "routed-server", "--no-jit", False, a, tag + "-a"))
        guests.append(Guest(runtime, work, "relay-server", "--no-jit", False, b, tag + "-b", c))
        guests.append(
            Guest(runtime, work, "routed-result-client", "--no-jit", False, d, tag + "-c")
        )
        a.close()
        b.close()
        c.close()
        d.close()
        try:
            guests[2].finish()
        except RuntimeError as error:
            if guests[2].process.returncode != 1 or marker not in str(error):
                raise
        else:
            raise RuntimeError("routed result omission escaped: " + marker)
        guests[1].finish("local=0 source actors joined")
        guests[0].finish("value=" + str(expected))
    finally:
        for endpoint in (a, b, c, d):
            endpoint.close()
        for guest in guests:
            guest.abort()


# EN: Native reserve refusal in runtime/root must emit no routed result request.
# work owns fixture/logs; compiler/include compile matching capacity client.
# RU: Native reserve refusal в runtime/root не отправляет routed result request.
# work владеет fixture/logs; compiler/include собирают matching capacity client.
def capacity(runtime, root, work, compiler, include):
    source = (root / "fixtures/proxy-network/relay-capacity-client.b").read_text()
    anchor = "net->importref(endpoint, function, 0, 10000)"
    if source.count(anchor) != 1:
        raise RuntimeError("routed reserve fixture anchor moved")
    private = work / "routed-capacity.b"
    private.write_text(
        source.replace(anchor, "net->routedfetch(endpoint, function, local, local, 2, 1, 10000)")
    )
    compile_one(root, work, compiler, include, private, "routed-capacity")
    binary = relink(runtime, work, root / "test_proxy_gc_omissions.c", 5, ["dvm_proxy_lease_new"])
    peer, child = connected_tcp()
    peer.settimeout(20)
    guest = Guest(binary, work, "routed-capacity", "--no-jit", False, child, "routed-reserve")
    child.close()
    try:
        hello = bytearray(frame(1, target=(1, 0, 1), left=(1, 1, 1), low=44444))
        struct.pack_into(">Q", hello, 16, 33333)
        peer.sendall(hello)
        for expected in (1, 5):
            request = bytearray(receive(peer))
            if struct.unpack_from(">Q", request, 40)[0] != expected:
                raise RuntimeError("routed reserve refusal emitted callback bytes")
            request[5] = 3
            peer.sendall(request)
        guest.finish("PASS forwarded reserve:")
    finally:
        peer.close()
        guest.abort()


# EN: Borrow production root/runtime and matching compiler/include. Each private
# bytecode omission is restored in finally; work owns all variants. Return only
# after exact failures and completed effects/joins, never timeout/crash acceptance.
# RU: Borrow production root/runtime и matching compiler/include. Каждый private
# bytecode omission восстанавливается в finally; work владеет variants. Вернуть
# лишь после exact failures и completed effects/joins, без timeout/crash acceptance.
def controls(runtime, root, work, compiler, include):
    variants = (
        (
            "proxyregistry",
            "status := proxy->retainuntil(context.endpoint, view.owner, deadline);",
            "status := 0;",
            "routed result native ownership lost after original Drop",
            12,
        ),
        (
            "proxyregistry",
            "view.source == source",
            "1",
            "unregistered routed result wrapper escaped",
            10,
        ),
        (
            "proxynet",
            " || request.operation == Proxyowner->RoutedReturnVariable",
            "",
            "routed result binding lost live alias",
            12,
        ),
    )
    for index, (module, anchor, replacement, marker, value) in enumerate(variants):
        path = root.parent / "limbo" / (module + ".b")
        source = path.read_text()
        if source.count(anchor) != 1:
            raise RuntimeError("routed omission anchor moved: " + anchor)
        private = work / (module + "-routed-omission.b")
        private.write_text(source.replace(anchor, replacement))
        try:
            compile_one(root, work, compiler, include, private, module)
            rejected(runtime, work, "routed-omission-" + str(index), marker, value)
        finally:
            compile_one(root, work, compiler, include, path, module)
    capacity(runtime, root, work, compiler, include)
    print(
        "PASS routed result omissions: native Retain exact wrapper binding and pre-effect reserve",
        flush=True,
    )

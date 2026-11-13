# Copyright (c) 2026 bhgv
# EN: Three-VM forwarding omissions must expose exact authority/lifetime failures.
# RU: Three-VM forwarding omissions обязаны выявить exact authority/lifetime failures.
import struct

from network_proxy_fixture import Guest, connected_tcp, compile_one
from network_proxy_forwarding import three_vms
from network_proxy_faults import receive, frame
from relink_private import relink


# EN/RU: Own three actual VMs through intended client failure and clean A/B cleanup.
# Владеть тремя actual VMs до intended client failure и clean A/B cleanup.
def rejected(runtime, work, tag, marker, expected):
    a, b = connected_tcp()
    c, d = connected_tcp()
    guests = []
    try:
        guests.append(Guest(runtime, work, "routed-server", "--no-jit", False, a, tag + "-a"))
        guests.append(Guest(runtime, work, "relay-server", "--no-jit", False, b, tag + "-b", c))
        guests.append(Guest(runtime, work, "relay-client", "--no-jit", False, d, tag + "-c"))
        try:
            guests[2].finish()
        except RuntimeError as error:
            if guests[2].process.returncode != 1 or marker not in str(error):
                raise
        else:
            raise RuntimeError("forwarding omission escaped: " + marker)
        d.shutdown(2)
        guests[1].finish("local=0 source actors joined")
        guests[0].finish("value=" + str(expected))
    finally:
        for endpoint in (a, b, c, d):
            endpoint.close()
        for guest in guests:
            guest.abort()


# EN/RU: Refuse only native result reservations; independent peer must see Read, never Import.
# Отказать лишь native result reservations; independent peer видит Read, не Import.
def reserve_refusal(runtime, root, work, expired=False):
    binary = (
        runtime
        if expired
        else relink(runtime, work, root / "test_proxy_gc_omissions.c", 5, ["dvm_proxy_lease_new"])
    )
    peer, child = connected_tcp()
    peer.settimeout(20)
    guest = Guest(
        binary,
        work,
        "relay-deadline-client" if expired else "relay-capacity-client",
        "--no-jit",
        False,
        child,
        "forward-reserve",
    )
    child.close()
    try:
        hello = bytearray(frame(1, target=(1, 0, 1), left=(1, 1, 1), low=44444))
        struct.pack_into(">Q", hello, 16, 33333)
        peer.sendall(hello)
        request = receive(peer)
        if struct.unpack_from(">Q", request, 40)[0] != 1:
            raise RuntimeError("refused forwarding reservation emitted a wire import")
        request = bytearray(request)
        request[5] = 3
        peer.sendall(request)
        stop = bytearray(receive(peer))
        if struct.unpack_from(">Q", stop, 40)[0] != 5:
            raise RuntimeError("forward reserve refusal changed connection lifecycle")
        stop[5] = 3
        peer.sendall(stop)
        guest.finish(
            "PASS expired forwarding startup:"
            if expired
            else "PASS forwarded reserve: no wire import live output preserved join"
        )
    finally:
        peer.close()
        guest.abort()


# EN: Break bounded slot authority, raw-stub exclusion and shared view lifetime
# in isolated bytecode. Then remove actual B collector obligations in a private
# runtime and require the joined transitive-GC predicate to reject it.
# RU: Ломать slot authority, raw-stub exclusion и shared view lifetime в private
# bytecode. Затем убрать B collector obligations и проверить transitive-GC predicate.
def controls(runtime, root, work, compiler, include):
    path = root.parent / "limbo/proxyregistry.b"
    source = path.read_text()
    variants = (
        (
            "slot",
            "slot >= len context.sources",
            "slot < 0",
            "context.sources[slot]",
            "context.sources[0]",
            "unregistered route slot became accessible",
            10,
        ),
        (
            "stub",
            "if ((operation == Proxyowner->Invoke",
            "if (0 && (operation == Proxyowner->Invoke",
            "",
            "",
            "ordinary callback received a forwarding stub as local Cell",
            10,
        ),
        (
            "owner",
            "else if (--view.owners == 0)",
            "else if (--view.owners >= 0)",
            "",
            "",
            "forwarded original Drop revoked retained route owner",
            42,
        ),
    )
    for name, anchor, replacement, second, changed, marker, value in variants:
        if source.count(anchor) != 1 or (second and source.count(second) != 1):
            raise RuntimeError("forwarding control anchor moved: " + name)
        text = source.replace(anchor, replacement)
        if second:
            text = text.replace(second, changed)
        private = work / ("forward-" + name + ".b")
        private.write_text(text)
        try:
            compile_one(root, work, compiler, include, private, "proxyregistry")
            rejected(runtime, work, "forward-" + name, marker, value)
        finally:
            compile_one(root, work, compiler, include, path, "proxyregistry")
    path = root.parent / "limbo/proxynet.b"
    source = path.read_text()
    anchor = " || request.operation == Proxyowner->ImportRoute"
    if source.count(anchor) != 1:
        raise RuntimeError("forwarded result binding anchor moved")
    private = work / "forward-binding.b"
    private.write_text(source.replace(anchor, ""))
    try:
        compile_one(root, work, compiler, include, private, "proxynet")
        rejected(runtime, work, "forward-binding", "third VM route import ownership", 10)
    finally:
        compile_one(root, work, compiler, include, path, "proxynet")
    reserve_refusal(runtime, root, work)
    reserve_refusal(runtime, root, work, True)
    broken = relink(
        runtime, work, root / "test_proxy_gc_omissions.c", 2, ["dvm_proxy_lease_retire"]
    )
    try:
        three_vms(runtime, work, "--no-jit", False, "forward-gc", broken)
    except RuntimeError as error:
        if "forwarded route GC did not reach every owner" not in str(error):
            raise
    else:
        raise RuntimeError("missing transitive forwarding GC obligation escaped")
    print(
        "PASS forwarding controls: slot authority stub exclusion retained ownership binding quota expiry and transitive GC"
    )

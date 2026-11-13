# Copyright (c) 2026 bhgv
# EN: Actual routed ownership omissions, preserving production sources and objects.
# RU: Actual routed ownership omissions с сохранением production sources и objects.
from network_proxy_fixture import Guest, compile_one, connected_tcp
from network_proxy_routing import check_ref_gc
from relink_private import relink


# EN/RU: Join two real VMs; require exact intended guard failure or native GC rejection.
# Join двух actual VMs; требовать exact intended guard failure либо native GC rejection.
def pair(runtime, work, tag, missing_gc):
    left, right = connected_tcp()
    guests = []
    try:
        guests.append(
            Guest(runtime, work, "routed-server", "--no-jit", False, left, tag + "-server")
        )
        guests.append(
            Guest(runtime, work, "routed-client", "--no-jit", False, right, tag + "-client")
        )
        left.close()
        right.close()
        if missing_gc:
            output = guests[1].finish("PASS cross-owner proxy refs:")
            try:
                check_ref_gc(output)
            except RuntimeError as error:
                if "cross-owner ref GC did not reach actor" not in str(error):
                    raise
            else:
                raise RuntimeError("missing routed ref GC obligation escaped")
            expected = "value=42"
        else:
            try:
                guests[1].finish()
            except RuntimeError as error:
                if guests[
                    1
                ].process.returncode != 1 or "routed ref accepted another actor token" not in str(
                    error
                ):
                    raise
            else:
                raise RuntimeError("wrong routed actor token escaped")
            expected = "value=1009"
        guests[0].finish(expected)
    finally:
        left.close()
        right.close()
        for guest in guests:
            guest.abort()


# EN: Omit native queue identity in a private bytecode view, then omit the actual
# collector Release callback in a private linked runtime. Neither crash nor
# compiler/timeout failure is accepted as semantic detection.
# RU: Пропустить native queue identity в private bytecode, затем actual collector
# Release callback в private runtime. Crash/compiler/timeout не засчитываются.
def controls(runtime, root, work, compiler, include):
    source = root.parent / "limbo/proxyroute.b"
    text = source.read_text()
    anchor = "gc->belongs(queue, access.source.lease) && "
    if text.count(anchor) != 1:
        raise RuntimeError("routed actor identity control anchor moved")
    private = work / "route-identity-control.b"
    private.write_text(text.replace(anchor, ""))
    try:
        compile_one(root, work, compiler, include, private, "proxyroute")
        pair(runtime, work, "route-identity", False)
    finally:
        compile_one(root, work, compiler, include, source, "proxyroute")
    broken = relink(
        runtime, work, root / "test_proxy_gc_omissions.c", 2, ["dvm_proxy_lease_retire"]
    )
    pair(broken, work, "route-gc", True)
    print("PASS routed omissions: exact actor token and actual last-ref GC obligation detected")

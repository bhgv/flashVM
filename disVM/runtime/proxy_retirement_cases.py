# Copyright (c) 2026 bhgv
# EN: Actual owner/access completion gates validate bounded orphan thread lifetime.
# RU: Actual owner/access completion gates проверяют bounded orphan thread lifetime.
import re
import struct
import threading
import time

from network_proxy_fixture import Guest, connected_tcp, compile_one
from network_proxy_faults import receive


# EN/RU: Compile private gates from matched fixtures into work using compiler/include.
# Собрать private gates из matching fixtures в work через compiler/include.
def prepare(root, work, compiler, include):
    for name in ("owner-orphan-client", "owner-policy-client", "ref-orphan-client"):
        compile_one(
            root, work, compiler, include, root / "fixtures/proxy-network" / (name + ".b"), name
        )
    source = (work / "queued-handler.b").read_text()
    anchor = "        return left.value;\n    }\n    left.value += value;"
    if source.count(anchor) != 1:
        raise RuntimeError("orphan callback gate anchor moved")
    source = source.replace(anchor, "        left.value++;\n" + anchor)
    source = source.replace("owner-entered", "owner-orphan-entered").replace(
        "owner-release", "owner-orphan-release"
    )
    path = work / "owner-orphan-handler.b"
    path.write_text(source)
    compile_one(root, work, compiler, include, path, "owner-orphan-handler")
    source = (root / "fixtures/proxy-network/handler.b").read_text()
    source = source.replace(
        'include "proxyhandler.m";', 'include "proxyhandler.m";\ninclude "proxyowner.m";'
    )
    policy = """    if (value == -776) {
        owner := load Proxyowner Proxyowner->PATH;
        owner->init();
        fake := ref Proxyowner->Endpoint(chan[16] of ref Proxyowner->Request);
        absent := Proxyowner->Handle(big 0, 0, big 0);
        (denied, unused) := owner->call(fake, Proxyowner->Read, absent, absent, absent, 0, 0, 1000);
        if (denied != Proxyowner->OwnerError) {
            raise "nested callback acquired private supervisor authority";
        }
        return left.value;
    }
"""
    source = source.replace("    if (value == -999) {", policy + "    if (value == -999) {", 1)
    path = work / "owner-policy-handler.b"
    path.write_text(source)
    compile_one(root, work, compiler, include, path, "owner-policy-handler")


# EN: Borrow runtime/work and execution selectors; own one VM/log through all joins.
# fixture selects orphan or policy; denied requires exact intentional policy failure.
# RU: Borrow runtime/work и execution selectors; владеть VM/log до всех joins.
# fixture выбирает orphan/policy, denied требует exact intentional policy failure.
def local(runtime, work, mode, locked, tag, fixture, denied=False):
    for name in ("owner-orphan-entered", "owner-orphan-release"):
        (work / name).unlink(missing_ok=True)
    with (work / "local-input").open("wb"):
        pass
    with (work / "local-input").open("rb") as empty:
        guest = Guest(runtime, work, fixture, mode, locked, empty, tag)
        try:
            if denied:
                try:
                    guest.finish()
                except RuntimeError as error:
                    if (
                        guest.process.returncode != 1
                        or "nested owner call bypassed pre-spawn policy" not in str(error)
                    ):
                        raise
                    output = guest.path.read_text()
                    if not re.search(r"PROXYHOST opened=1 closed=1 exports=0 active=0", output):
                        raise RuntimeError("nested omission did not retire actual owner roots")
                else:
                    raise RuntimeError("private supervisor bypassed caller policy undetected")
            else:
                guest.finish(
                    (
                        "PASS owner policy:"
                        if fixture == "owner-policy-client"
                        else "PASS owner orphan:"
                    ),
                    budget=40,
                )
        finally:
            guest.abort()


# EN: Relay actual frames and hold a completed Add=11 until caller retirement.
# Borrow endpoints/work/log; append errors and always shut down both directions.
# RU: Relay actual frames, держать completed Add=11 до retirement caller.
# Borrow endpoints/work/log; записывать errors, всегда закрывать оба направления.
def relay(server, client, work, log, errors):
    try:
        client.sendall(receive(server))
        held = False
        while True:
            request = receive(client)
            server.sendall(request)
            response = receive(server)
            operation = struct.unpack_from(">Q", request, 40)[0]
            if operation == 9 and not held:
                if (
                    struct.unpack_from(">q", response, 136)[0] != 0
                    or struct.unpack_from(">q", response, 144)[0] != 11
                ):
                    raise RuntimeError("ref orphan did not complete its actual single Add")
                (work / "ref-orphan-held").write_text("A Add completed\n")
                deadline = time.monotonic() + 15
                while "PASS ref orphan caller retired before reply" not in log.read_text():
                    if time.monotonic() >= deadline:
                        raise RuntimeError("ref caller did not retire before completed reply")
                    time.sleep(0.005)
                held = True
            client.sendall(response)
            if operation == 5:
                return
    except Exception as error:
        errors.append(error)
    finally:
        for endpoint in (server, client):
            try:
                endpoint.shutdown(2)
            except OSError:
                pass


# EN/RU: Own two actual VMs and relay until one effect, actor cleanup and bounded joins.
# Владеть двумя actual VMs и relay до одного effect, actor cleanup и bounded joins.
def remote(runtime, work, mode, locked, tag):
    (work / "ref-orphan-held").unlink(missing_ok=True)
    a, server = connected_tcp()
    b, client = connected_tcp()
    guests, errors = [], []
    thread = None
    try:
        for endpoint in (server, client):
            endpoint.settimeout(30)
        guests.append(Guest(runtime, work, "routed-server", mode, locked, a, tag + "-a"))
        guests.append(Guest(runtime, work, "ref-orphan-client", mode, locked, b, tag + "-b"))
        a.close()
        b.close()
        thread = threading.Thread(target=relay, args=(server, client, work, guests[1].path, errors))
        thread.start()
        guests[1].finish("PASS ref orphan:", budget=40)
        guests[0].finish("result=0 value=11")
        thread.join(timeout=5)
        if thread.is_alive() or errors:
            raise RuntimeError("ref orphan relay failed: " + repr(errors))
    finally:
        for endpoint in (a, b, server, client):
            endpoint.close()
        for guest in guests:
            guest.abort()
        if thread is not None:
            thread.join(timeout=35)
            if thread.is_alive():
                raise RuntimeError("ref orphan relay did not join")


# EN/RU: Remove only caller-side policy in private bytecode; require semantic failure/clean owner.
# Удалить лишь caller-side policy в private bytecode; требовать semantic failure/clean owner.
def control(runtime, root, work, compiler, include):
    path = root.parent / "limbo/proxyowner.b"
    source = path.read_text()
    anchor = "if (host->canwait() == 0) {"
    if source.count(anchor) != 1:
        raise RuntimeError("owner caller policy anchor moved")
    private = work / "owner-policy-omission.b"
    private.write_text(source.replace(anchor, "if (0 && host->canwait() == 0) {"))
    try:
        compile_one(root, work, compiler, include, private, "proxyowner")
        local(runtime, work, "--no-jit", False, "policy-omission", "owner-policy-client", True)
    finally:
        compile_one(root, work, compiler, include, path, "proxyowner")
    print(
        "PASS caller policy omission: original callback cannot delegate nested RPC authority",
        flush=True,
    )

# Copyright (c) 2026 bhgv
# EN: Real connection loss must release all already offered network/ref callers promptly.
# RU: Actual connection loss должен быстро завершать все offered network/ref callers.
import re
import struct
import threading
import time

from network_proxy_fixture import Guest, connected_tcp, compile_one
from network_proxy_faults import receive
from resident_evidence import retirement_evidence
from native_gc_evidence import require_concurrent_gc


# EN/RU: Borrow matched compiler/include; compile both mailbox clients and retired-caller variants.
# Borrow matching compiler/include; собрать оба mailbox clients и retired-caller variants.
def prepare(root, work, compiler, include):
    for kind in ("net", "ref"):
        name = kind + "-disconnect-waiters"
        path = root / "fixtures/proxy-network" / (name + ".b")
        compile_one(root, work, compiler, include, path, name)
        source = path.read_text()
        anchor = '    gate = sys->create("disconnect-queue-ready", Sys->OWRITE, 8r666);'
        if source.count(anchor) != 1:
            raise RuntimeError("disconnect orphan gate anchor moved")
        source = source.replace(anchor, "    control->killwait(children[7]);\n" + anchor)
        for before, after in (
            ("while (count < 8 &&", "while (count < 7 &&"),
            ("timely := count == 8;", "timely := count == 7;"),
            ("for (index = count; index < 8;", "for (index = count; index < 7;"),
            ("eight offers woke", "seven replies and one retired caller joined"),
        ):
            if source.count(before) != 1:
                raise RuntimeError("disconnect orphan completion anchor moved")
            source = source.replace(before, after)
        private = work / (name + "-orphan.b")
        private.write_text(source)
        compile_one(root, work, compiler, include, private, name + "-orphan")


# EN: Borrow actual VM endpoints/work, append errors; forward through one successful Add,
# withhold its reply until seven more offers enter the mailbox, then close both directions.
# RU: Borrow actual VM endpoints/work, записать errors; передать один successful Add,
# удержать reply до семи queued offers, затем закрыть оба направления.
def relay(server, client, work, errors):
    try:
        client.sendall(receive(server))
        while True:
            request = receive(client)
            server.sendall(request)
            response = receive(server)
            if struct.unpack_from(">Q", request, 40)[0] == 9:
                if struct.unpack_from(">qq", response, 136) != (0, 11):
                    raise RuntimeError("disconnect relay did not confirm original single effect")
                (work / "disconnect-effect-held").write_text("actual A=11\n")
                deadline = time.monotonic() + 20
                while not (work / "disconnect-queue-ready").exists():
                    if time.monotonic() >= deadline:
                        raise RuntimeError("actual queued offers not observed")
                    time.sleep(0.005)
                return
            client.sendall(response)
    except Exception as error:
        errors.append(error)
    finally:
        for endpoint in (server, client):
            try:
                endpoint.shutdown(2)
            except OSError:
                pass


# EN: Own two actual VMs/relay until eight uncertain completions, A=11 and clean native
# joins. orphan retires one offered caller; denied requires exact semantic failure
# plus drained owner/GC/resident evidence.
# RU: Владеть двумя actual VMs/relay до восьми uncertain completions, A=11 и clean native
# joins. orphan завершает один offered caller; denied требует exact semantic failure
# и drained owner/GC/resident evidence.
def run(runtime, work, mode, locked, tag, kind, denied=False, orphan=False):
    for name in ("disconnect-effect-held", "disconnect-queue-ready"):
        (work / name).unlink(missing_ok=True)
    a, server = connected_tcp()
    b, client = connected_tcp()
    guests, errors = [], []
    thread = None
    try:
        for endpoint in (server, client):
            endpoint.settimeout(30)
        guests.append(Guest(runtime, work, "routed-server", mode, locked, a, tag + "-a"))
        guests.append(
            Guest(
                runtime,
                work,
                kind + "-disconnect-waiters" + ("-orphan" if orphan else ""),
                mode,
                locked,
                b,
                tag + "-b",
            )
        )
        a.close()
        b.close()
        thread = threading.Thread(target=relay, args=(server, client, work, errors))
        thread.start()
        if denied:
            try:
                guests[1].finish(budget=40)
            except RuntimeError as error:
                expected = "disconnect waiters did not observe terminal state"
                if guests[1].process.returncode != 1 or expected not in str(error):
                    raise
                output = guests[1].path.read_text()
                if "ThreadSanitizer:" in output or "runtime error:" in output:
                    raise RuntimeError("disconnect omission failed outside its semantic boundary")
                negative(output)
            else:
                raise RuntimeError("missing disconnect observation was not detected")
        else:
            guests[1].finish("PASS disconnect waiters:", budget=40)
        guests[0].finish("value=11")
        thread.join(timeout=5)
        if thread.is_alive() or errors:
            raise RuntimeError("disconnect relay failed: " + repr(errors))
    finally:
        for endpoint in (a, b, server, client):
            endpoint.close()
        for guest in guests:
            guest.abort()
        if thread is not None:
            thread.join(timeout=35)
            if thread.is_alive():
                raise RuntimeError("disconnect relay did not join")


# EN: Borrow failed-main diagnostics, require all child joins and native ownership drains.
# One queued failed-main record is expected; it is not a successful application exit.
# RU: Borrow failed-main diagnostics, требовать все child joins и native ownership drains.
# Ожидается одна queued запись failed main; это не successful application exit.
def negative(output):
    require_concurrent_gc(output)
    starts = re.findall(r"NATIVEGUEST start pid=(\d+) thread=(\d+)", output)
    retired = re.findall(r"NATIVEGUEST retired pid=(\d+) thread=(\d+)", output)
    retirement_evidence(output, len(starts))
    if (
        not starts
        or len({pid for pid, _ in starts}) != len(starts)
        or sorted(starts) != sorted(retired)
        or not re.search(r"NATIVEWORKERS threads=\d+ joined=1 failed=0", output)
        or not re.search(
            r"NATIVEDISPATCH claims=([1-9][0-9]*) finishes=\1 active=0 migrated=0", output
        )
        or not re.search(r"NATIVEQUEUE queued=1 leases=([1-9][0-9]*) returns=\1 inflight=0", output)
        or not re.search(
            r"NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0",
            output,
        )
        or not re.search(r"PROXYGC queues=0 tokens=0 pending=0", output)
    ):
        raise RuntimeError(
            "disconnect omission did not drain actual native owners\n" + output[-8000:]
        )


# EN: Remove only one watchdog's terminal-state observation in private bytecode;
# require the explicit missing-completion failure, restore matched library on every path.
# RU: Удалить только terminal-state observation одного watchdog в private bytecode;
# требовать explicit missing-completion failure, всегда восстановить matched library.
def control(runtime, root, work, compiler, include):
    for kind, name, anchor in (
        ("net", "proxynet", "while (!endpoint.state.closed) {"),
        ("ref", "proxyref", "while (!state.closed) {"),
    ):
        path = root.parent / "limbo" / (name + ".b")
        source = path.read_text()
        if source.count(anchor) != 1:
            raise RuntimeError("disconnect watchdog anchor moved")
        private = work / (name + "-disconnect-omission.b")
        private.write_text(source.replace(anchor, "while (1) {"))
        try:
            compile_one(root, work, compiler, include, private, name)
            run(runtime, work, "--no-jit", False, "disconnect-omission-" + kind, kind, True)
        finally:
            compile_one(root, work, compiler, include, path, name)
        print(
            "PASS disconnect watchdog omission: " + kind + " exact missing-completion failure",
            flush=True,
        )

# Copyright (c) 2026 bhgv
# EN: Force actual Ref Close/reentry ordering without weakening original source ownership.
# RU: Принудительно проверить actual Ref Close/reentry ordering при original source ownership.
from network_proxy_fixture import Guest, connected_tcp, compile_one
from disconnect_waiter_cases import negative


# EN/RU: Compile the actual source/client fixture with borrowed matched compiler/include/work.
# Собрать actual source/client fixture через borrowed matched compiler/include/work.
def prepare(root, work, compiler, include):
    path = root / "fixtures/proxy-network/ref-terminal-publication.b"
    compile_one(root, work, compiler, include, path, "ref-terminal-publication")


# EN/RU: Own both actual VMs through successful Close and exact reentry refusal/omission.
# Владеть обеими actual VMs до successful Close и exact reentry refusal/omission.
def run(runtime, work, mode, locked, tag, omitted):
    a, b = connected_tcp()
    guests = []
    try:
        guests.append(Guest(runtime, work, "routed-server", mode, locked, a, tag + "-a"))
        guests.append(Guest(runtime, work, "ref-terminal-publication", mode, locked, b, tag + "-b"))
        a.close()
        b.close()
        if omitted:
            try:
                guests[1].finish()
            except RuntimeError as error:
                expected = "ref terminal publication exposed reply before refusing reentry"
                if guests[1].process.returncode != 1 or expected not in str(error):
                    raise
                output = guests[1].path.read_text()
                if "ThreadSanitizer:" in output or "runtime error:" in output:
                    raise RuntimeError("ref publication omission failed outside semantic boundary")
                negative(output)
            else:
                raise RuntimeError("ref publication omission escaped detection")
        else:
            guests[1].finish("PASS ref terminal publication:")
        guests[0].finish("value=10")
    finally:
        a.close()
        b.close()
        for guest in guests:
            guest.abort()


# EN: Widen only the active Close reply/settled-state gap, optionally remove early
# refusal, and require C/JIT/poll success or exact omission. Always restore source library.
# RU: Расширить лишь gap active Close reply/settled-state, при omitted убрать early
# refusal, требовать C/JIT/poll success либо exact omission. Всегда восстановить library.
def check(runtime, root, work, compiler, include, omitted=False):
    path = root.parent / "limbo/proxyroute.b"
    source = path.read_text()
    anchor = "        if (terminal) {\n            state.closed = Proxyref->Closed;"
    if source.count(anchor) != 1:
        raise RuntimeError("ref terminal publication gap anchor moved")
    source = source.replace(
        anchor,
        "        if (terminal) {\n            sys->sleep(500);\n            state.closed = Proxyref->Closed;",
    )
    if omitted:
        anchor = "            state.closed = Proxyref->Completing;"
        if source.count(anchor) != 1:
            raise RuntimeError("ref terminal refusal anchor moved")
        source = source.replace(anchor, "            # Private omission: no early refusal.")
    private = work / "ref-terminal-route.b"
    private.write_text(source)
    try:
        compile_one(root, work, compiler, include, private, "proxyroute")
        for mode in (("--no-jit",) if omitted else ("--no-jit", "--jit")):
            for locked in ((False,) if omitted else (False, True)):
                run(
                    runtime,
                    work,
                    mode,
                    locked,
                    "ref-publication-" + mode[2:] + str(locked),
                    omitted,
                )
    finally:
        compile_one(root, work, compiler, include, path, "proxyroute")
    print(
        "PASS ref terminal publication: "
        + ("early-refusal omission detected" if omitted else "forced Close/Read fence"),
        flush=True,
    )

# Copyright (c) 2026 bhgv
# EN: Actual three-VM startup exception injection requires Unknown and complete actor retirement.
# RU: Actual three-VM startup exception injection требует Unknown и complete actor retirement.
from pathlib import Path
import tempfile, sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from network_proxy_fixture import compile_fixture, compile_one, Guest, connected_tcp

root = Path(__file__).resolve().parent
runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])


# EN/RU: Inject stage on matched private libraries; own three VMs through exact refusal and joins.
# Inject stage в matched private libraries; владеть тремя VM до exact refusal и joins.
def run(stage, mode, locked):
    with tempfile.TemporaryDirectory(prefix="registry actor failure ") as temporary:
        work = Path(temporary).resolve()
        compile_fixture(root, work, compiler, include)
        text = (root / "fixtures/proxy-network/parallel-cleanup-client.b").read_text()
        text = text.replace(
            "imported1 != 0 || imported2 != 0 || kind1 != 1 || kind2 != 1",
            "imported1 != -3 || imported2 != -3 || kind1 != 0 || kind2 != 0",
        )
        text = text.replace(
            "context.views[0].source != nil || context.views[1].source != nil",
            "context.views[0] != nil || context.views[1] != nil",
        )
        p = work / "failure-client.b"
        p.write_text(text)
        compile_one(root, work, compiler, include, p, "failure-client")
        text = (root.parent / "limbo/proxyroute.b").read_text()
        if stage == "early":
            at = text.rindex("    begin(setup);")
            text = text[:at] + text[at:].replace(
                "    begin(setup);", '    raise "private startup failure";', 1
            )
        else:
            anchor = "released = run(source, copy, timeout, queue, requests, state);"
            if text.count(anchor) != 1:
                raise RuntimeError("post-ready failure anchor moved")
            text = text.replace(anchor, 'raise "private post-ready failure";')
            registry = (root.parent / "limbo/proxyregistry.b").read_text()
            anchor = (
                "    # EN/RU: Do not publish a stub for a source already known to have retired."
            )
            if registry.count(anchor) != 1:
                raise RuntimeError("retired import gate anchor moved")
            registry = registry.replace(
                anchor,
                "    while (!source.state.closed && sys->millisec() < deadline) {\n        sys->sleep(5);\n    }\n"
                + anchor,
            )
            private_registry = work / "retired-source-registry.b"
            private_registry.write_text(registry)
            compile_one(root, work, compiler, include, private_registry, "proxyregistry")
        p = work / "failure-route.b"
        p.write_text(text)
        compile_one(root, work, compiler, include, p, "proxyroute")
        sockets = []
        guests = []
        try:
            for index in range(2):
                a, b = connected_tcp()
                sockets.extend((a, b))
                guests.append(
                    Guest(runtime, work, "routed-server", mode, locked, a, "a" + str(index))
                )
                a.close()
            guests.append(
                Guest(
                    runtime,
                    work,
                    "failure-client",
                    mode,
                    locked,
                    sockets[1],
                    "b",
                    output=sockets[3],
                )
            )
            sockets[1].close()
            sockets[3].close()
            guests[2].finish("PASS parallel cleanup:", budget=20)
            for guest in guests[:2]:
                guest.finish("value=10")
            print(
                "PASS actor failure "
                + stage
                + " "
                + mode
                + str(locked)
                + ": Unknown, no dead stub, all three VMs joined"
            )
        finally:
            for endpoint in sockets:
                endpoint.close()
            for guest in guests:
                guest.abort()


for stage in ("early", "retired"):
    for mode in ("--no-jit", "--jit"):
        for locked in (False, True):
            run(stage, mode, locked)

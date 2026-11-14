# Copyright (c) 2026 bhgv
# EN: Actual three-VM startup exception injection requires Unknown and complete actor retirement.
# RU: Actual three-VM startup exception injection требует Unknown и complete actor retirement.
from pathlib import Path
import tempfile, sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from network_proxy_fixture import compile_fixture, compile_one, Guest, connected_tcp

root = Path(__file__).resolve().parent
runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
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
    at = text.rindex("    begin(setup);")
    text = text[:at] + text[at:].replace(
        "    begin(setup);", '    raise "private startup failure";', 1
    )
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
                Guest(runtime, work, "routed-server", "--no-jit", False, a, "a" + str(index))
            )
            a.close()
        guests.append(
            Guest(
                runtime,
                work,
                "failure-client",
                "--no-jit",
                False,
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
            "PASS startup exceptions: Unknown, reused actor slots, all three actual VMs and roots joined"
        )
    finally:
        for endpoint in sockets:
            endpoint.close()
        for guest in guests:
            guest.abort()

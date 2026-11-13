# Copyright (c) 2026 bhgv
# EN: Sustained three-VM import/drop proves reusable bounded actor bookkeeping.
# RU: Sustained three-VM import/drop доказывает reusable bounded actor bookkeeping.
from pathlib import Path
import sys
import tempfile

from network_proxy_fixture import Guest, compile_fixture, compile_one, connected_tcp
from network_proxy_forwarding import forwarded_gc
from network_proxy_deadlines import controls as deadline_controls, owner_control


# EN: Own three actual VMs through forty imports on one connection. A changes
# 10 -> 50; B stays zero. Omitted reaping must fail exactly after sixteen effects.
# RU: Владеть тремя actual VMs до сорока imports на одной connection. A меняется
# 10 -> 50; B остаётся zero. Omitted reaping обязан отказать после sixteen effects.
def churn(runtime, work, mode, locked, tag, missing_reap=False):
    a, b = connected_tcp()
    c, d = connected_tcp()
    guests = []
    try:
        guests.append(Guest(runtime, work, "routed-server", mode, locked, a, tag + "-a"))
        guests.append(Guest(runtime, work, "relay-server", mode, locked, b, tag + "-b", c))
        guests.append(Guest(runtime, work, "relay-churn-client", mode, locked, d, tag + "-c"))
        a.close()
        b.close()
        c.close()
        d.close()
        if missing_reap:
            try:
                guests[2].finish()
            except RuntimeError as error:
                if guests[2].process.returncode != 1 or (
                    "forwarding churn capacity did not recover: round=16 status=4" not in str(error)
                ):
                    raise
            else:
                raise RuntimeError("missing actor reaping escaped sustained import")
        else:
            guests[2].finish("PASS forwarding churn: 40 imports")
        middle = guests[1].finish("local=0 source actors joined")
        guests[0].finish("value=26" if missing_reap else "result=0 value=50")
        forwarded_gc(middle, 16 if missing_reap else 40)
    finally:
        for endpoint in (a, b, c, d):
            endpoint.close()
        for guest in guests:
            guest.abort()


# EN/RU: Omit only completed-slot reuse in isolated bytecode and require actual quota failure.
# Пропустить лишь completed-slot reuse в isolated bytecode; требовать actual quota failure.
def control(runtime, root, work, compiler, include):
    path = root.parent / "limbo/proxyregistry.b"
    source = path.read_text()
    anchor = "if (context.actors[index] != nil) {\n            pid := 0;"
    if source.count(anchor) != 1:
        raise RuntimeError("actor reaping control anchor moved")
    private = work / "no-actor-reuse.b"
    private.write_text(
        source.replace(anchor, "if (0 && context.actors[index] != nil) {\n            pid := 0;")
    )
    try:
        compile_one(root, work, compiler, include, private, "proxyregistry")
        churn(runtime, work, "--no-jit", False, "no-reuse", True)
    finally:
        compile_one(root, work, compiler, include, path, "proxyregistry")
    print("PASS actor reaping omission: sixteen effects then exact capacity refusal", flush=True)


# EN/RU: Compile matched production modules; cover C/JIT/poll and optional private omission.
# Собрать matching production modules; проверить C/JIT/poll и optional private omission.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    positive = sys.argv[4:] == ["--positive-only"]
    only_controls = sys.argv[4:] == ["--controls-only"]
    if sys.argv[4:] and not positive and not only_controls:
        raise RuntimeError("unsupported churn arguments")
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="forwarding churn ") as temporary:
        work = Path(temporary)
        compile_fixture(root, work, compiler, include)
        if not only_controls:
            for mode in ("--no-jit", "--jit"):
                for locked in (False, True):
                    tag = mode[2:] + str(locked)
                    churn(runtime, work, mode, locked, tag)
                    print("PASS sustained forwarding " + tag, flush=True)
        if not positive:
            control(runtime, root, work, compiler, include)
            deadline_controls(runtime, root, work, compiler, include)
            owner_control(runtime, root, work, compiler, include)


if __name__ == "__main__":
    main()

# Copyright (c) 2026 bhgv
# EN: Two actual DisVMs, concurrent remote refs, replay and transport-fault acceptance.
# RU: Две настоящие DisVM, concurrent remote refs, replay и transport-fault acceptance.
from pathlib import Path
import struct
import sys
import re
import tempfile

from network_proxy_results import result_replay, result_disconnect, orphan_result
from network_proxy_routing import routed_pair, add_replay
from network_proxy_forwarding import three_vms, forwarding_oracle, forwarding_loss, orphan_import
from network_proxy_controls import controls
from network_proxy_fixture import Guest, compile_fixture, connected_tcp
from network_proxy_faults import (
    admitted_disconnect,
    client_refusal,
    refusal,
    release_replay,
    retain_replay,
    replay,
    trickle,
    orphan,
    idle_client,
    output_disconnect,
    greeting_refusal,
)


# EN/RU: Run both actual VMs through a trusted inherited TCP stream and join them on every path.
# Запустить обе VM через trusted inherited TCP stream и join на любом пути.
def two_vms(runtime, work, mode, locked, tag, client_name="client"):
    left, right = connected_tcp()
    guests = []
    try:
        guests.append(Guest(runtime, work, "server", mode, locked, left, tag + "-server"))
        guests.append(Guest(runtime, work, client_name, mode, locked, right, tag + "-client"))
        left.close()
        right.close()
        if client_name == "routed-client":
            output = guests[1].finish(
                "PASS cross-owner proxy refs: original callback 8 clients atomic aliases Drop GC join"
            )
            count = re.search(
                r"PROXYGC queues=0 tokens=0 pending=0 enqueued=(\d+) drained=(\d+)", output
            )
            if not count or int(count[1]) < 1 or int(count[2]) < 1:
                raise RuntimeError("cross-owner ref GC did not reach actor")
            guests[0].finish("result=0 value=42")
        elif client_name == "result-quota-client":
            guests[1].finish(
                "PASS remote result quota: completed effect preserved release recovery terminal roots"
            )
            guests[0].finish("result=0 value=12")
        elif client_name == "result-client":
            guests[1].finish(
                "PASS callback result images: new variables functions aliases 8 clients errors GC"
            )
            guests[0].finish("result=0 value=20")
        elif client_name == "gc-client":
            output = guests[1].finish(
                "PASS network image GC: original variable/function and last clone released"
            )
            match = re.search(
                r"PROXYGC queues=0 tokens=0 pending=0 enqueued=(\d+) drained=(\d+)", output
            )
            if not match or int(match[1]) < 4 or int(match[2]) < 4:
                raise RuntimeError("actual image finalizers did not reach the broker\n" + output)
            guests[0].finish("result=0 value=14")
        else:
            guests[1].finish(
                "PASS network client: 8 clients actual ref aliases epochs callbacks drop join"
            )
            guests[0].finish("result=0 value=99")
    finally:
        left.close()
        right.close()
        for guest in guests:
            guest.abort()


# EN/RU: Replace one aligned unsigned scalar in independent-oracle request bytes.
# Заменить один aligned unsigned scalar в independent-oracle request bytes.
def field(offset, value):
    return lambda data: struct.pack_into(">Q", data, offset, value)


# EN: Compile matching production libraries, then test C/JIT and both polling
# paths. Faults include real callback entry before disconnect, not synthetic pins.
# RU: Собрать matching production libraries; проверить C/JIT и оба poll paths.
# Faults включают настоящий callback entry до disconnect, не synthetic pins.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    positive = sys.argv[4:] == ["--positive-only"]
    only_controls = sys.argv[4:] == ["--controls-only"]
    if sys.argv[4:] and not positive and not only_controls:
        raise RuntimeError("unsupported network proxy test arguments")
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="network owner proxy ") as temporary:
        work = Path(temporary)
        compile_fixture(root, work, compiler, include)
        if not only_controls:
            for mode in ("--no-jit", "--jit"):
                for locked in (False, True):
                    tag = mode[2:] + ("-locked" if locked else "-fast")
                    with open(work / "scalar-input", "wb") as empty_input:
                        empty_input.write(b"")
                    with open(work / "scalar-input", "rb") as empty_input:
                        scalar = Guest(
                            runtime,
                            work,
                            "scalar-results",
                            mode,
                            locked,
                            empty_input,
                            tag + "-scalars",
                        )
                        try:
                            scalar.finish("PASS guarded native scalar results")
                        finally:
                            scalar.abort()
                    with open(work / "scalar-input", "rb") as empty_input:
                        native = Guest(
                            runtime,
                            work,
                            "lease-native",
                            mode,
                            locked,
                            empty_input,
                            tag + "-native-lease",
                        )
                        try:
                            native.finish(
                                "PASS native leases: aliases quota staging foreign refusal GC reuse close"
                            )
                        finally:
                            native.abort()
                    two_vms(runtime, work, mode, locked, tag)
                    two_vms(runtime, work, mode, locked, tag + "-gc", "gc-client")
                    two_vms(runtime, work, mode, locked, tag + "-result", "result-client")
                    routed_pair(runtime, work, mode, locked, tag + "-routed")
                    three_vms(runtime, work, mode, locked, tag + "-three-vms")
                    orphan_import(runtime, work, mode, locked, tag + "-orphan-import")
                    forwarding_oracle(runtime, work, mode, locked, tag + "-forward-oracle")
                    for hop in ("source", "caller"):
                        forwarding_loss(runtime, work, mode, locked, tag + "-loss-" + hop, hop)
                    routed_pair(runtime, work, mode, locked, tag + "-routed-loss", True)
                    add_replay(runtime, work, mode, locked, tag + "-add-replay")
                    two_vms(
                        runtime, work, mode, locked, tag + "-result-quota", "result-quota-client"
                    )
                    output_disconnect(runtime, work, mode, locked, tag + "-output-loss")
                    idle_client(runtime, work, mode, locked, tag + "-idle-client")
                    replay(runtime, work, mode, locked, tag + "-replay")
                    release_replay(runtime, work, mode, locked, tag + "-release")
                    retain_replay(runtime, work, mode, locked, tag + "-retain-replay")
                    result_replay(runtime, work, mode, locked, tag + "-result-replay")
                    result_disconnect(runtime, work, mode, locked, tag + "-result-disconnect")
                    orphan_result(runtime, work, mode, locked, tag + "-orphan-result")
                    admitted_disconnect(runtime, work, mode, locked, tag + "-admitted")
                    for name in ("orphan-io", "orphan-server"):
                        orphan(runtime, work, mode, locked, tag + name, name)
                    for index, mutation in enumerate(
                        (field(24, 67891), field(64, 0), field(136, 7))
                    ):
                        greeting_refusal(
                            runtime, work, mode, locked, tag + f"-greeting-{index}", mutation
                        )
                    for index, mutation in enumerate(
                        (
                            field(8, 185),
                            field(32, 0),
                            field(48, 3),
                            field(72, 256),
                            field(136, 1),
                            field(152, 1),
                            field(160, 1),
                            lambda data: data.__setitem__(4, 2),
                            lambda data: data.__setitem__(6, 1),
                            lambda data: data.__delitem__(slice(70, None)),
                        )
                    ):
                        refusal(runtime, work, mode, locked, tag + f"-malformed-{index}", mutation)
                    refusal(
                        runtime,
                        work,
                        mode,
                        locked,
                        tag + "-old-epoch",
                        lambda data: None,
                        name="new-incarnation",
                    )
                    for index, mutation in enumerate(
                        (
                            field(32, 99),
                            field(24, 67891),
                            field(136, 999),
                            field(152, 1),
                            field(160, 1),
                            lambda data: data.clear(),
                        )
                    ):
                        client_refusal(
                            runtime, work, mode, locked, tag + f"-reply-{index}", mutation
                        )
                    for index, mutation in enumerate(
                        (
                            field(152, 0),
                            field(152, 2),
                            field(160, 0),
                            field(160, 2),
                            field(168, 256),
                            field(176, 0),
                            field(144, 1),
                            lambda data: data.__delitem__(slice(170, None)),
                        )
                    ):
                        client_refusal(
                            runtime,
                            work,
                            mode,
                            locked,
                            tag + f"-result-reply-{index}",
                            mutation,
                            "result-failure-client",
                        )
            trickle(runtime, work, "--no-jit", False, "trickle")
        if not positive:
            controls(runtime, root, work, compiler, include)
    if not only_controls:
        print(
            "PASS network proxies: two real VMs over TCP; 8 clients, original ref aliases, "
            "callback variable/function results, orphan result GC, quota, bounded replay/release, stale epochs, framing/reply faults and admitted disconnect; C/JIT both polls"
        )


if __name__ == "__main__":
    main()

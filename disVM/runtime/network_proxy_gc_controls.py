# Copyright (c) 2026 bhgv
# EN: Actual two-VM omissions must expose missing peer ownership/GC Release.
# RU: Actual two-VM omissions выявляют отсутствие peer ownership/GC Release.
import subprocess

from network_proxy_fixture import Guest, connected_tcp
from relink_private import relink


# EN/RU: One private runtime mutation; require explicit intended client failure, not a crash.
# Одна private runtime mutation; требовать intended client failure, не crash.
def omission(runtime, root, work, variant, symbol, marker):
    broken = relink(runtime, work, root / "test_proxy_gc_omissions.c", variant, [symbol])
    left, right = connected_tcp()
    guests = []
    try:
        guests.append(
            Guest(broken, work, "server", "--no-jit", False, left, f"gc-control-{variant}-server")
        )
        guests.append(
            Guest(
                broken, work, "gc-client", "--no-jit", False, right, f"gc-control-{variant}-client"
            )
        )
        left.close()
        right.close()
        try:
            guests[1].finish()
        except RuntimeError as error:
            if guests[1].process.returncode != 1 or marker not in str(error):
                raise
        else:
            raise RuntimeError("image GC omission escaped: " + marker)
        guests[0].finish("value=10")
    finally:
        left.close()
        right.close()
        for guest in guests:
            guest.abort()


# EN/RU: Preserve production sources; compile matched private wrapper objects only.
# Сохранить production sources; собирать лишь matching private wrapper objects.
def controls(runtime, root, work):
    omission(
        runtime,
        root,
        work,
        1,
        "dvm_remote_managed_retain",
        "GC of original images revoked distinct retained owner",
    )
    omission(
        runtime,
        root,
        work,
        2,
        "dvm_proxy_lease_retire",
        "remote image GC did not acknowledge expected releases",
    )
    print("PASS image GC omissions: missing Retain and collector Release detected")

    for variant, symbol in ((3, "dvm_runtime_movp_copy"), (4, "ptradd")):
        binary = relink(runtime, work, root / "test_proxy_gc_omissions.c", variant, [symbol])
        result = subprocess.run(
            [str(binary), "--stats", "--no-jit", str(work / "lease-refusal.dis")],
            cwd=work,
            capture_output=True,
            text=True,
            timeout=30,
        )
        output = result.stdout + result.stderr
        if (
            result.returncode
            or "ThreadSanitizer:" in output
            or "PASS native image construction refusal rooted and cleaned" not in output
            or "PROXYGC queues=0 tokens=0 pending=0" not in output
        ):
            raise RuntimeError(
                f"native image construction refusal {variant} failed\n" + output[-18000:]
            )
    print("PASS image constructors: root/publication refusal preserves output and native ownership")

# Copyright (c) 2026 bhgv
# EN: Private compiled omissions must fail the actual network owner semantics.
# RU: Приватные compiled omissions обязаны нарушать настоящую network owner semantics.

from network_proxy_gc_controls import controls as gc_controls
from relink_private import relink
import subprocess

from network_proxy_fixture import Guest, compile_one, connected_tcp
from network_proxy_faults import replay, client_refusal


# EN/RU: Compile a private source view into fixture work; production source stays unchanged.
# Собрать private source view в fixture work; production source не меняется.
def replacement(root, work, compiler, include, name, source):
    path = work / (name + "-control.b")
    path.write_text(source)
    compile_one(root, work, compiler, include, path, name)


# EN/RU: Require exactly the intended semantic rejection, never crash/compile/timeout evidence.
# Требовать ровно ожидаемый semantic rejection, не crash/compile/timeout evidence.
def rejected(operation, marker):
    try:
        operation()
    except RuntimeError as error:
        if marker in str(error):
            return
        raise
    raise RuntimeError("private network omission escaped: " + marker)


# EN: Run actual client/server with a private image-guard omission. marker names
# the intended client error, expected proves the server's last scalar value.
# Join both guests; crashes/timeouts do not count as semantic detection.
# RU: Запустить client/server с private image-guard omission. marker задаёт
# ожидаемую ошибку client, expected — последнее scalar value server. Join обоих;
# crashes/timeouts не считаются semantic detection.
def stale_image(runtime, work, marker="network stale incarnation admitted", expected="value=999"):
    left, right = connected_tcp()
    guests = []
    try:
        guests.append(Guest(runtime, work, "server", "--no-jit", False, left, "bad-epoch-server"))
        guests.append(Guest(runtime, work, "client", "--no-jit", False, right, "bad-epoch-client"))
        left.close()
        right.close()
        rejected(lambda: guests[1].finish(), marker)
        guests[0].finish(expected)
    finally:
        left.close()
        right.close()
        for guest in guests:
            guest.abort()


# EN: Break one guard at a time; restore every matching library in finally.
# Negative variants are actual Dis bytecode on native threads, not model mocks.
# RU: Ломать один guard за раз и восстанавливать matching library в finally.
# Negative variants — настоящий Dis bytecode на native threads, не model mocks.
def controls(runtime, root, work, compiler, include):
    path = root.parent / "limbo/proxynet.b"
    source = path.read_text()
    lease_check = (
        "gc->matches(image.lease, image.owner.session, image.owner.slot, image.owner.generation)"
    )
    anchor = (
        "return image.high == endpoint.high && image.low == endpoint.low && " + lease_check + ";"
    )
    if source.count(anchor) != 1:
        raise RuntimeError("client epoch control anchor moved")
    try:
        replacement(
            root,
            work,
            compiler,
            include,
            "proxynet",
            source.replace(anchor, "return " + lease_check + ";"),
        )
        stale_image(runtime, work)
    finally:
        compile_one(root, work, compiler, include, path, "proxynet")
    try:
        replacement(
            root,
            work,
            compiler,
            include,
            "proxynet",
            source.replace(lease_check, "gc->active(image.lease)"),
        )
        stale_image(runtime, work, "network image owner did not match its native lease", "value=42")
    finally:
        compile_one(root, work, compiler, include, path, "proxynet")
    anchor = "return (valid, returned);"
    if source.count(anchor) != 1:
        raise RuntimeError("result reply validation control anchor moved")
    try:
        replacement(
            root,
            work,
            compiler,
            include,
            "proxynet",
            source.replace(anchor, "return (1, returned);"),
        )
        rejected(
            lambda: client_refusal(
                runtime,
                work,
                "--no-jit",
                False,
                "result-kind-control",
                lambda data: data.__setitem__(slice(152, 160), bytes(8)),
                "result-failure-client",
            ),
            "corrupt or missing network reply was trusted",
        )
    finally:
        compile_one(root, work, compiler, include, path, "proxynet")
    path = root.parent / "limbo/proxyserver.b"
    source = path.read_text()
    anchor = "        if (id == last) {"
    if source.count(anchor) != 1:
        raise RuntimeError("server replay control anchor moved")
    mutation = (
        "        if (id == last && codec->same(bytes, previous, 0)) {\n            last--;\n        }\n"
        + anchor
    )
    try:
        replacement(root, work, compiler, include, "proxyserver", source.replace(anchor, mutation))
        rejected(
            lambda: replay(runtime, work, "--no-jit", False, "bad-replay"),
            "exact completed request replayed an owner effect",
        )
    finally:
        compile_one(root, work, compiler, include, path, "proxyserver")
    print("PASS network omissions: epoch, exact lease owner and repeated owner effect rejected")

    gc_controls(runtime, root, work)

    flags = runtime.parent / "CMakeFiles/disvm.dir/flags.make"
    if "-fsanitize=thread" in flags.read_text():
        broken = relink(
            runtime, work, root / "test_sys_scalar_result.c", 0, ["dvm_sys_scalar_result"]
        )
        output = subprocess.run(
            [str(broken), "--stats", "--no-jit", str(work / "scalar-results.dis")],
            cwd=work,
            capture_output=True,
            text=True,
            timeout=60,
        )
        text = output.stdout + output.stderr
        if (
            output.returncode not in (0, 66)
            or "PASS guarded native scalar results" not in text
            or "PASS Dis application completed" not in text
            or "WARNING: ThreadSanitizer: data race" not in text
            or "__wrap_dvm_sys_scalar_result" not in text
        ):
            raise RuntimeError(
                "raw builtin result omission did not expose actual race\n" + text[-18000:]
            )
        print("PASS raw native scalar result stores expose actual TSan race")

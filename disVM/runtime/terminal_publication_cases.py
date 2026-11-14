# Copyright (c) 2026 bhgv
# EN: Force the terminal publication interleaving observed in native AMD64 CI.
# RU: Принудительно воспроизвести terminal publication interleaving из native AMD64 CI.
import socket

from network_proxy_fixture import Guest, compile_one
from network_proxy_faults import frame, receive
from disconnect_waiter_cases import negative


# EN/RU: Compile scalar/result clients into borrowed work using matched compiler/include.
# Собрать scalar/result clients в borrowed work через matched compiler/include.
def prepare(root, work, compiler, include):
    source = root / "fixtures/proxy-network/terminal-publication-client.b"
    compile_one(root, work, compiler, include, source, "terminal-publication-scalar")
    text = source.read_text()
    if text.count("if (!exercise(0)) {") != 1:
        raise RuntimeError("terminal publication result anchor moved")
    private = work / "terminal-publication-result.b"
    private.write_text(text.replace("if (!exercise(0)) {", "if (!exercise(1)) {"))
    compile_one(root, work, compiler, include, private, "terminal-publication-result")


# EN: Borrow source, widen only the reply-to-settled window; optionally omit the
# early offer-refusal phase. Return private source, refusing moved anchors.
# RU: Borrow source, расширить лишь reply-to-settled window; при omitted убрать
# ранний offer-refusal phase. Вернуть private source, отказать при смене anchors.
def variant(source, omitted):
    anchor = "        request.reply <-= result;\n        if (terminal) {"
    if source.count(anchor) != 1:
        raise RuntimeError("terminal publication window anchor moved")
    source = source.replace(
        anchor,
        "        request.reply <-= result;\n        if (terminal) {\n            sys->sleep(500);",
    )
    if omitted:
        anchor = "            endpoint.state.closed = Completing;"
        if source.count(anchor) != 1:
            raise RuntimeError("terminal offer-refusal phase anchor moved")
        source = source.replace(
            anchor, "            # Private omission: do not refuse reentry yet."
        )
    return source


# EN/RU: Corrupt one independent peer reply; require immediate refusal and no extra wire bytes.
# Повредить один independent peer reply; требовать immediate refusal и отсутствие новых wire bytes.
def run(runtime, work, mode, locked, tag, kind, denied=False):
    peer, child = socket.socketpair()
    peer.settimeout(15)
    guest = Guest(runtime, work, "terminal-publication-" + kind, mode, locked, child, tag)
    child.close()
    try:
        peer.sendall(frame(1, target=(1, 0, 1), left=(1, 1, 1)))
        response = bytearray(receive(peer))
        response[0] = 0
        response[5] = 3
        peer.sendall(response)
        if denied:
            try:
                guest.finish()
            except RuntimeError as error:
                expected = "terminal publication exposed reply before refusing reentry"
                if guest.process.returncode != 1 or expected not in str(error):
                    raise
                output = guest.path.read_text()
                if "ThreadSanitizer:" in output or "runtime error:" in output:
                    raise RuntimeError("terminal omission failed outside its semantic boundary")
                negative(output)
            else:
                raise RuntimeError("terminal reentry refusal omission escaped detection")
        else:
            guest.finish("PASS terminal publication:")
        if peer.recv(1):
            raise RuntimeError("terminal publication emitted another wire request")
    finally:
        peer.close()
        guest.abort()


# EN: Own private widened-window bytecode until all C/JIT/poll scalar/result cases
# finish, or until exact omission failure. Always restore matched production library.
# RU: Владеть private widened-window bytecode до всех C/JIT/poll scalar/result cases
# либо exact omission failure. Всегда восстановить matched production library.
def check(runtime, root, work, compiler, include, omitted=False):
    path = root.parent / "limbo/proxynet.b"
    private = work / "terminal-publication-net.b"
    private.write_text(variant(path.read_text(), omitted))
    try:
        compile_one(root, work, compiler, include, private, "proxynet")
        for mode in (("--no-jit",) if omitted else ("--no-jit", "--jit")):
            for locked in ((False,) if omitted else (False, True)):
                for kind in ("scalar", "result"):
                    run(
                        runtime,
                        work,
                        mode,
                        locked,
                        "publication-" + mode[2:] + str(locked) + kind,
                        kind,
                        omitted,
                    )
    finally:
        compile_one(root, work, compiler, include, path, "proxynet")
    print(
        "PASS terminal publication: "
        + ("offer-refusal omission detected" if omitted else "forced scalar/result reentry fence"),
        flush=True,
    )

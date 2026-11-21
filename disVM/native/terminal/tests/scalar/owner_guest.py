# Copyright (c) 2026 bhgv
# EN: Run generated caller and owner against isolated DisVM processes over Styx.
# RU: Запустить сгенерированные клиент и владельца в отдельных DisVM через Styx.
import os
from pathlib import Path
import re
import socket
import subprocess
import sys
import tempfile
import time

from guest import build_peer, run

sys.path.insert(0, str(Path(__file__).resolve().parents[4] / "runtime"))
from native_gc_evidence import require_concurrent_gc


# EN: Check retirement of a single-root VM, including file and worker cleanup.
# RU: Проверить завершение однопроцессной VM, включая файлы и рабочие потоки.
def retirement(output):
    require_concurrent_gc(output)
    counters = (
        r"NATIVEFILES created=(\d+) closed=\1 live=0 operations=\d+ waits=0 pending=0",
        r"NATIVECLOSE submitted=(\d+) completed=\1 pending=0 peak=\d+ joined=1",
        r"NATIVEDISPATCH claims=(\d+) finishes=\1 active=0 migrated=0",
        r"NATIVEWORKERS threads=\d+ joined=1 failed=0",
    )
    if "PASS Dis application completed" not in output or not all(
        re.search(pattern, output) for pattern in counters
    ):
        raise RuntimeError("scalar VM did not retire cleanly:\n" + output)


# EN: Give each peer its own inherited carrier end and private working directory.
# RU: Передать каждой стороне свой конец соединения и свой рабочий каталог.
def peer(binary, directory, image, mode, carrier, owner, calls, disconnect, mounted):
    env = os.environ.copy()
    env.pop("DVM_TEST_SCALAR_OWNER", None)
    env.pop("DVM_TEST_SCALAR_DISCONNECT", None)
    if mounted:
        env["DVM_TEST_SCALAR_MOUNTED_OWNER"] = "1"
    if mounted == "wide":
        env["DVM_TEST_SCALAR_BIG_OWNER"] = "1"
    if mounted == "tuple":
        env["DVM_TEST_SCALAR_TUPLE_OWNER"] = "1"
    if mounted == "refpair":
        env["DVM_TEST_SCALAR_REF_PAIR_OWNER"] = "1"
    if mounted == "list":
        env["DVM_TEST_SCALAR_LIST_OWNER"] = "1"
    if mounted == "ref":
        env["DVM_TEST_SCALAR_REF_OWNER"] = "1"
    if mounted == "mixed":
        env["DVM_TEST_SCALAR_MIXED_OWNER"] = "1"
    if disconnect:
        env["DVM_TEST_SCALAR_DISCONNECT"] = "1"
    env["DVM_TEST_SCALAR_CALL_COUNT"] = str(calls)
    if owner:
        env["DVM_TEST_SCALAR_OWNER"] = "1"
        env["DVM_TEST_SCALAR_DELAY_OWNER"] = "1"
    env["DVM_TEST_SCALAR_FD"] = str(carrier.fileno())
    return subprocess.Popen([str(binary), "--stats", mode, image], cwd=directory, env=env,
                            pass_fds=(carrier.fileno(),), stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)


# EN: Relay private test markers between roots in order, bounding the wait.
# RU: Передать тестовые маркеры между корнями по порядку с ограничением ожидания.
def relay_ref_pair(owner, caller):
    deadline = time.monotonic() + 3
    for source, target, name in (
        (owner, caller, "refpair-entered"),
        (caller, owner, "refpair-edited"),
    ):
        while (source / name).read_bytes() != b"1":
            if time.monotonic() >= deadline:
                return
            time.sleep(0.005)
        (target / name).write_bytes(b"1")


# EN: Run both sides to completion, bounding failures and always reaping both.
# RU: Дождаться обеих сторон, ограничить время отказа и завершить оба процесса.
def exchange(binary, owner, caller, mode, marker, calls=1, disconnect=False, mounted=False):
    children = []
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        right = socket.create_connection(listener.getsockname(), timeout=5)
        left, _ = listener.accept()
    for carrier in (left, right):
        carrier.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    try:
        children.append(peer(binary, owner, "owner.dis", mode, left, True, calls, disconnect, mounted))
        image = {"wide": "wide.dis", "mixed": "mixed.dis", "tuple": "tuple.dis",
                 "ref": "ref.dis", "list": "blend.dis", "refpair": "refpair.dis"}.get(mounted, "mounted.dis" if mounted else "client.dis")
        children.append(peer(binary, caller, image,
                             mode, right, False, calls, disconnect, mounted))
        left.close()
        right.close()
        if mounted == "refpair":
            relay_ref_pair(owner, caller)
        owner_output, _ = children[0].communicate(timeout=15)
        caller_output, _ = children[1].communicate(timeout=15)
        if any(child.returncode for child in children) or marker not in caller_output:
            raise RuntimeError(f"{mode}: Styx pair failed\n{owner_output}{caller_output}")
        retirement(owner_output)
        retirement(caller_output)
        print(marker, mode, flush=True)
    finally:
        left.close()
        right.close()
        for child in children:
            if child.poll() is None:
                child.kill()
            child.communicate()


# EN: Generate both ends from the original interface and compile the actual owner.
# RU: Сгенерировать обе стороны по исходному интерфейсу и собрать реализацию владельца.
def main():
    runtime, compiler, sdk = [Path(arg).resolve() for arg in sys.argv[1:4]]
    disvm = Path(__file__).resolve().parents[5] / "disVM"
    example = disvm / "examples/remote-scalar"
    with tempfile.TemporaryDirectory(prefix="scalar-pair-") as name:
        work = Path(name)
        caller, owner = work / "caller", work / "owner"
        for directory in (caller, owner):
            (directory / "scalar").mkdir(parents=True)
        (caller / "blocked").mkdir()
        (caller / "library").mkdir()
        (caller / "alias.dis").mkdir()
        (caller / "peer").mkdir()
        for folder, source, output in (
            (caller, example / "styx-pair.b", "client.dis"),
            (caller, example / "styx-mounted.b", "mounted.dis"),
            (caller, example / "styx-wide-owner.b", "wide.dis"),
            (caller, example / "styx-mixed-owner.b", "mixed.dis"),
            (caller, example / "styx-tuple-owner.b", "tuple.dis"),
            (owner, example / "tuple-owner.b", "tuple-owner.dis"),
            (caller, example / "styx-ref-owner.b", "ref.dis"),
            (owner, example / "ref-owner.b", "ref-owner.dis"),
            (caller, example / "styx-list-owner.b", "blend.dis"),
            (owner, example / "blend-owner.b", "blend-owner.dis"),
            (caller, example / "styx-refpair-owner.b", "refpair.dis"),
            (owner, example / "refpair-owner.b", "refpair-owner.dis"),
            (owner, example / "wide-owner.b", "wide-owner.dis"),
            (owner, disvm / "limbo/scalarowner.b", "owner.dis"),
            (owner, example / "implementation.b", "implementation.dis"),
            (owner, example / "privatevalue.b", "privatevalue.dis"),
            (caller, example / "privatevalue.b", "privatevalue.dis"),
        ):
            run([str(compiler), "-I", str(disvm / "upstream/module"), "-I", str(disvm / "module"),
                 "-I", str(example), "-o", str(folder / output), str(source)], work)
        (caller / "original-prefix").write_bytes((owner / "implementation.dis").read_bytes()[:14])
        (owner / "provider-secret").write_text("41")
        (caller / "provider-secret").write_text("900")
        binary = build_peer(runtime, sdk, work, True, compiler)
        flag = caller / "expect-error"
        missing = caller / "expect-dependency-error"
        dependency = owner / "privatevalue.dis"
        hidden = owner / "privatevalue.hidden"
        (caller / "expect-dis-owner").touch()
        marker_paths = [root / marker for root in (owner, caller)
                        for marker in ("refpair-entered", "refpair-edited")]
        for mode in ("--no-jit", "--jit"):
            exchange(binary, owner, caller, mode, "PASS mounted Styx original Dis",
                     calls=0, mounted=True)
            exchange(binary, owner, caller, mode, "PASS mounted original big Dis", calls=0, mounted="wide")
            exchange(binary, owner, caller, mode, "PASS mounted original mixed Dis state", calls=0, mounted="mixed")
            exchange(binary, owner, caller, mode, "PASS mounted original tuple Dis", calls=0, mounted="tuple")
            exchange(binary, owner, caller, mode, "PASS mounted original ref Dis", calls=0, mounted="ref")
            exchange(binary, owner, caller, mode, "PASS mounted original argument list Dis", calls=0, mounted="list")
            for marker in marker_paths:
                marker.write_bytes(b"0")
            exchange(binary, owner, caller, mode, "PASS mounted original ref pair Dis", calls=0, mounted="refpair")
            flag.unlink(missing_ok=True)
            exchange(binary, owner, caller, mode, "PASS owner-local Styx Compute sequence", calls=0)
            disconnect = caller / "expect-disconnect"
            disconnect.touch()
            try:
                exchange(binary, owner, caller, mode, "PASS owner-local Styx before carrier EOF",
                         calls=0, disconnect=True)
            finally:
                disconnect.unlink()
            flag.touch()
            exchange(binary, owner, caller, mode, "PASS owner-local Styx Compute exception")
            flag.unlink()
            missing.touch()
            dependency.rename(hidden)
            try:
                exchange(binary, owner, caller, mode, "PASS owner-local Styx dependency refusal")
            finally:
                hidden.rename(dependency)
                missing.unlink()


if __name__ == "__main__":
    main()

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
    loaded_exports = {
        "wide": ("wide.dis", "wide-owner.dis", "echo"),
        "tuple": ("tuple.dis", "tuple-owner.dis", "transform"),
        "ref": ("ref.dis", "ref-owner.dis", "revise"),
        "list": ("blend.dis", "blend-owner.dis", "blend"),
        "legacy-import": ("blend.dis", "blend-owner.dis", "blend"),
        "import-shape-mismatch": ("blend.dis", "blend-owner.dis", "blend"),
        "refpair": ("refpair.dis", "refpair-owner.dis", "exchange"),
        "load-mismatch": ("blend.dis", "blend-scalar-owner.dis", "blend"),
    }
    if mounted in ("list", "legacy-import", "import-shape-mismatch"):
        env["DVM_TEST_SCALAR_COMPILED_TYPES"] = "1"
    if mounted in loaded_exports:
        publication, implementation, export = loaded_exports[mounted]
        env["DVM_TEST_SCALAR_LOAD_NAME"] = publication
        env["DVM_TEST_SCALAR_LOAD_FILE"] = implementation
        env["DVM_TEST_SCALAR_LOAD_EXPORT"] = export
    if mounted in ("loaded-set", "loaded-set-reject"):
        env["DVM_TEST_SCALAR_LOADED_SET"] = "reject" if mounted.endswith("reject") else "accept"
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
                 "ref": "ref.dis", "list": "blend.dis", "refpair": "refpair.dis", "load-mismatch": "wrong-profile.dis"}.get(mounted, "mounted.dis" if mounted else "client.dis")
        if mounted in ("loaded-set", "loaded-set-reject"):
            image = "set.dis"
        if mounted == "legacy-import":
            image = "legacy-import.dis"
        if mounted == "import-shape-mismatch":
            image = "mismatched-import.dis"
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


# EN: Write one fixture with a changed import type or a mismatched trailer signature.
# RU: Записать тестовый образ с изменённым типом или несовпадающей сигнатурой импорта.
def mismatch_import_shape(source, target, change_signature=False):
    image = bytearray(source.read_bytes())
    index = image.rindex(b"STYXCS2!") + 8
    while image[index:index+4] != b"\xff" * 4:
        index += 10 + image[index+8] + image[index+9]
    index += 4
    matches = 0
    while image[index:index+4] != b"\xff" * 4:
        args, result = image[index+12:index+14]
        if image[index+8:index+12] == bytes.fromhex("ff407097"):
            if image[index+14:index+14+args] != b"(i(ib)b)":
                raise RuntimeError("unexpected compiled caller Blend shape")
            if change_signature:
                image[index+8] ^= 1
            else:
                image[index+15] = ord("b")
            matches += 1
        index += 14 + args + result
    if matches != 1 or index + 4 != len(image):
        raise RuntimeError("no exact compiled Blend import shape to mutate")
    target.write_bytes(image)


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
            (owner, example / "blend-scalar-owner.b", "blend-scalar-owner.dis"),
            (caller, example / "styx-loaded-profile.b", "wrong-profile.dis"),
            (caller, example / "styx-refpair-owner.b", "refpair.dis"),
            (owner, example / "refpair-owner.b", "refpair-owner.dis"),
            (owner, example / "wide-owner.b", "wide-owner.dis"),
            (caller, example / "styx-loaded-set.b", "set.dis"),
            (owner, example / "loaded-set-owner.b", "loaded-set-owner.dis"),
            (owner, disvm / "limbo/scalarowner.b", "owner.dis"),
            (owner, example / "implementation.b", "implementation.dis"),
            (owner, example / "privatevalue.b", "privatevalue.dis"),
            (caller, example / "privatevalue.b", "privatevalue.dis"),
        ):
            run([str(compiler), "-I", str(disvm / "upstream/module"), "-I", str(disvm / "module"),
                 "-I", str(example), "-o", str(folder / output), str(source)], work)
        # Exercise the old profile with the exact pre-table prefix of the same compiler image.
        # Проверить старый профиль на точном префиксе образа до таблицы.
        legacy_owner = owner / "blend-scalar-owner.dis"
        legacy_image = legacy_owner.read_bytes()
        legacy_owner.write_bytes(legacy_image[:legacy_image.rindex(b"STYXCS1!")])
        corrupted = bytearray((owner / "blend-owner.dis").read_bytes())
        trailer = corrupted.rindex(b"STYXCS1!")
        corrupted[trailer + 12] ^= 1
        (caller / "invalid-shape.dis").write_bytes(corrupted)
        mismatch_import_shape(caller / "blend.dis", caller / "mismatched-import.dis")
        client = (caller / "blend.dis").read_bytes()
        (caller / "legacy-import.dis").write_bytes(client[:client.rindex(b"STYXCS2!")])
        invalid_import = caller / "invalid-import.dis"
        mismatch_import_shape(caller / "blend.dis", invalid_import, change_signature=True)
        refused = subprocess.run([str(runtime), "--no-jit", invalid_import.name], cwd=caller,
                                 capture_output=True, text=True, timeout=10)
        if refused.returncode == 0 or "invalid compiled Styx type table" not in refused.stderr + refused.stdout:
            raise RuntimeError("loader did not reject an import signature mismatch")
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
            exchange(binary, owner, caller, mode, "PASS mounted original argument list Dis", calls=0,
                     mounted="legacy-import")
            shape_refusal = caller / "expect-import-shape-refusal"
            shape_refusal.touch()
            try:
                exchange(binary, owner, caller, mode, "PASS mounted Dis rejects local import shape mismatch",
                         calls=0, mounted="import-shape-mismatch")
            finally:
                shape_refusal.unlink()
            exchange(binary, owner, caller, mode, "PASS Styx profile from loaded export", calls=0, mounted="load-mismatch")
            for marker in marker_paths:
                marker.write_bytes(b"0")
            exchange(binary, owner, caller, mode, "PASS mounted original ref pair Dis", calls=0, mounted="refpair")
            exchange(binary, owner, caller, mode, "PASS mounted loaded export set shares state", calls=0, mounted="loaded-set")
            refusal = caller / "expect-set-refusal"
            refusal.touch()
            try:
                exchange(binary, owner, caller, mode, "PASS loaded export set refused as a whole",
                         calls=0, mounted="loaded-set-reject")
            finally:
                refusal.unlink()
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

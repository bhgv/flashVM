# Copyright (c) 2026 bhgv
# EN: Run generated caller and owner against isolated DisVM processes over Styx.
# RU: Запустить сгенерированные клиент и владельца в отдельных DisVM через Styx.
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile

from guest import build_peer, run


# EN: Give each peer its own inherited carrier end and private working directory.
# RU: Передать каждой стороне свой конец соединения и свой рабочий каталог.
def peer(binary, directory, image, mode, carrier, owner):
    env = os.environ.copy()
    env.pop("DVM_TEST_SCALAR_OWNER", None)
    if owner:
        env["DVM_TEST_SCALAR_OWNER"] = "1"
        env["DVM_TEST_SCALAR_DELAY_OWNER"] = "1"
    env["DVM_TEST_SCALAR_FD"] = str(carrier.fileno())
    return subprocess.Popen([str(binary), mode, image], cwd=directory, env=env,
                            pass_fds=(carrier.fileno(),), stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)


# EN: Run both sides to completion, bounding failures and always reaping both.
# RU: Дождаться обеих сторон, ограничить время отказа и завершить оба процесса.
def exchange(binary, owner, caller, mode, marker):
    children = []
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        right = socket.create_connection(listener.getsockname(), timeout=5)
        left, _ = listener.accept()
    for carrier in (left, right):
        carrier.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    try:
        children.append(peer(binary, owner, "owner.dis", mode, left, True))
        children.append(peer(binary, caller, "client.dis", mode, right, False))
        left.close()
        right.close()
        owner_output, _ = children[0].communicate(timeout=15)
        caller_output, _ = children[1].communicate(timeout=15)
        if any(child.returncode for child in children) or marker not in caller_output:
            raise RuntimeError(f"{mode}: Styx pair failed\n{owner_output}{caller_output}")
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
    generator, runtime, compiler, sdk = [Path(arg).resolve() for arg in sys.argv[1:5]]
    disvm = Path(__file__).resolve().parents[5] / "disVM"
    example = disvm / "examples/remote-scalar"
    with tempfile.TemporaryDirectory(prefix="scalar-pair-") as name:
        work = Path(name)
        caller, owner = work / "caller", work / "owner"
        for directory in (caller, owner):
            (directory / "scalar").mkdir(parents=True)
        (caller / "blocked").mkdir()
        (caller / "library").mkdir()
        run([str(generator), "--styx-owner", str(example / "compute.m"), "compute.m",
             "implementation.dis", "/scalar/request", "/scalar/reply", "4294967295", "64",
             str(owner / "generated.b")], work)
        for folder, source, output in (
            (caller, example / "styx-pair.b", "client.dis"),
            (owner, owner / "generated.b", "owner.dis"),
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
        for mode in ("--no-jit", "--jit"):
            flag.unlink(missing_ok=True)
            exchange(binary, owner, caller, mode, "PASS owner-local Styx Compute value")
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

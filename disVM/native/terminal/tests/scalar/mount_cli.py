# Copyright (c) 2026 bhgv
# EN: Run stock DisVM and a separate provider over a delegated carrier.
# RU: Запустить штатный DisVM и отдельного провайдера по выданному транспорту.
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time

from guest import build_peer


# EN: Verify the command in both VM modes using a fresh carrier for each.
# RU: Проверить команду в обоих режимах VM, каждый раз с новым транспортом.
def main():
    runtime, compiler, provider = map(lambda name: Path(name).resolve(), sys.argv[1:])
    disvm = Path(__file__).resolve().parents[4]
    example = disvm / "examples/remote-scalar"
    with tempfile.TemporaryDirectory(prefix="styx-cli-") as directory:
        work = Path(directory)
        (work / "alias.dis").touch()
        for source, marker, options, peer_options in (
            ("styx-cli.b", "PASS bidirectional Styx CLI mount", [], []),
            ("styx-run.b", "PASS mounted Styx launch submission",
             ["--launch", "implementation.dis", ".unibus", "17"], ["launch"]),
        ):
            image = work / "caller.dis"
            subprocess.run([compiler, "-I", disvm / "upstream/module", "-I", disvm / "module",
                            "-I", example, "-o", image, example / source], cwd=work, check=True)
            for mode in ("--no-jit", "--jit"):
                caller, owner = socket.socketpair()
                with caller, owner, subprocess.Popen(
                    [provider, str(owner.fileno()), *peer_options], pass_fds=(owner.fileno(),), cwd=work
                ) as peer:
                    owner.close()
                    try:
                        result = subprocess.run(
                            [runtime, "--styx-mount-fd", str(caller.fileno()), *options, "--", mode, image],
                            pass_fds=(caller.fileno(),), cwd=work, capture_output=True, text=True, timeout=20)
                        caller.close()
                        if result.returncode or marker not in result.stdout or peer.wait(timeout=10):
                            raise RuntimeError(result.stdout + result.stderr)
                        print(marker, mode, flush=True)
                    finally:
                        if peer.poll() is None:
                            peer.kill()

        owner_image = work / "owner.dis"
        subprocess.run([compiler, "-I", disvm / "upstream/module", "-I", disvm / "module",
                        "-o", owner_image, example / "styx-owner.b"], cwd=work, check=True)
        for mode in ("--no-jit", "--jit"):
            caller, owner = socket.socketpair()
            with caller, owner, subprocess.Popen(
                [runtime, "--styx-serve-fd", str(owner.fileno()), "--", mode, owner_image],
                pass_fds=(owner.fileno(),), cwd=work, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
            ) as peer:
                owner.close()
                try:
                    result = subprocess.run(
                        [runtime, "--styx-mount-fd", str(caller.fileno()), "--", mode, image],
                        pass_fds=(caller.fileno(),), cwd=work, capture_output=True, text=True, timeout=20)
                    caller.close()
                    out, err = peer.communicate(timeout=10)
                    marker = "PASS mounted Styx owner Dis init"
                    if result.returncode or peer.returncode or marker not in out or                             "PASS mounted Styx launch submission" not in result.stdout:
                        raise RuntimeError(result.stdout + result.stderr + out + err)
                    print(marker, mode, flush=True)
                finally:
                    if peer.poll() is None:
                        peer.kill()

        input_file = work / "caller-stdin"
        input_file.write_bytes(b"live")
        for mode in ("--no-jit", "--jit"):
            output = work / "caller-stdout"
            caller, owner = socket.socketpair()
            with caller, owner, input_file.open("rb") as input_stream, output.open("wb") as output_stream, subprocess.Popen(
                [runtime, "--styx-serve-fd", str(owner.fileno()), "--context", "--", mode, owner_image],
                pass_fds=(owner.fileno(),), cwd=work, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
            ) as peer:
                owner.close()
                try:
                    result = subprocess.run(
                        [runtime, "--styx-mount-fd", str(caller.fileno()), "--", mode, image],
                        pass_fds=(caller.fileno(),), cwd=work, stdin=input_stream, stdout=output_stream,
                        stderr=subprocess.PIPE, text=True, timeout=40)
                    caller.close()
                    out, err = peer.communicate(timeout=10)
                    marker = "PASS scoped owner reads live caller FD"
                    actual = output.read_text()
                    if result.returncode or peer.returncode or marker not in actual or \
                            "PASS mounted Styx launch submission" not in actual:
                        raise RuntimeError(actual + result.stderr + out + err)
                    print(marker, mode, flush=True)
                finally:
                    if peer.poll() is None:
                        peer.kill()

        chain(runtime, compiler, provider, disvm, example, work)


# EN: Create an owned TCP connection for one link between the peers.
# RU: Создать TCP-соединение для одного звена между участниками.
def tcp_pair():
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        listener.listen()
        client = socket.create_connection(listener.getsockname())
        try:
            server, _ = listener.accept()
            for carrier in (client, server):
                carrier.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        except BaseException:
            client.close()
            raise
        return client, server


# EN: Run A, the executing DisVM B, and the separate Styx library peer C.
# RU: Запустить A, исполняющую DisVM B и отдельного провайдера библиотеки C.
def chain(runtime, compiler, provider, disvm, example, work):
    caller_image = work / "chain-client.dis"
    subprocess.run([compiler, "-I", disvm / "upstream/module", "-I", disvm / "module",
                    "-o", caller_image, example / "styx-chain-client.b"], cwd=work, check=True)
    library_dir = work / "library-peer"
    library_dir.mkdir()
    (library_dir / "scalar").mkdir()
    library_peer = build_peer(runtime, provider.parents[1], library_dir, True, compiler)
    for source, target in ((disvm / "limbo/scalarowner.b", "owner.dis"),
                           (example / "wide-owner.b", "wide-owner.dis")):
        subprocess.run([compiler, "-I", disvm / "upstream/module", "-I", disvm / "module",
                        "-I", example, "-o", library_dir / target, source], cwd=library_dir, check=True)
    owner_dir = work / "execution-peer"
    owner_dir.mkdir()
    deps = owner_dir / "deps"
    deps.mkdir()
    subprocess.run([compiler, "-I", example, "-o", deps / "wide.dis",
                    example / "styx-chain-local.b"], cwd=owner_dir, check=True)
    owner_image = owner_dir / "chain-owner.dis"
    subprocess.run([compiler, "-I", disvm / "upstream/module", "-I", disvm / "module",
                    "-I", example, "-o", owner_image, example / "styx-chain-owner.b"], cwd=work, check=True)
    stdin = work / "chain-stdin"
    stdout = work / "chain-stdout"
    stderr = work / "chain-stderr"
    extra = work / "chain-fd3"
    extra.write_bytes(b"skipmore")
    for path in ("/", "/remote/../libs", "/.prog", "/.fd/sub", "/.vfs", "/.var", "relative"):
        invalid = subprocess.run(
            [runtime, "--styx-serve-fd", "3", "--library-fd", "4",
             "--library-at", path, "--", owner_image],
            capture_output=True, timeout=5
        )
        if invalid.returncode != 2 or b"usage:" not in invalid.stderr:
            raise RuntimeError(f"invalid library mount path accepted: {path}")
    for mode, payload in (("--no-jit", b"live"), ("--jit", b"live"), ("--no-jit", b"fail"),
                          ("--no-jit", b"drop"), ("--jit", b"drop")):
        stdin.write_bytes(payload)
        a, b = tcp_pair()
        lib, c = tcp_pair()
        with a, b, lib, c, stdin.open("rb") as incoming, stdout.open("wb") as outgoing, \
                stderr.open("wb") as errors, extra.open("rb") as fourth, subprocess.Popen(
            [library_peer, mode, "owner.dis"], pass_fds=(c.fileno(),), cwd=library_dir,
            env=dict(os.environ, DVM_TEST_SCALAR_FD=str(c.fileno()), DVM_TEST_SCALAR_OWNER="1",
                     DVM_TEST_SCALAR_MOUNTED_OWNER="1", DVM_TEST_SCALAR_LOAD_NAME="wide.dis",
                     DVM_TEST_SCALAR_LOAD_FILE="wide-owner.dis", DVM_TEST_SCALAR_LOAD_EXPORT="echo")
        ) as library, subprocess.Popen(
            [runtime, "--styx-serve-fd", str(b.fileno()), "--context", "--library-fd",
             str(lib.fileno()), "--library-at", "/remote-libs", "--", mode, owner_image],
            pass_fds=(b.fileno(), lib.fileno()), cwd=owner_dir, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        ) as owner:
            fourth.seek(4)
            b.close()
            lib.close()
            c.close()
            try:
                with subprocess.Popen(
                    [runtime, "--styx-mount-fd", str(a.fileno()), "--fd3", str(fourth.fileno()), "--", mode, caller_image],
                    pass_fds=(a.fileno(), fourth.fileno()), cwd=work, stdin=incoming, stdout=outgoing, stderr=errors
                ) as caller:
                    try:
                        if payload == b"drop":
                            deadline = time.monotonic() + 5
                            while b"chain accepted before disconnect\n" not in stdout.read_bytes():
                                if owner.poll() is not None or caller.poll() is not None or time.monotonic() >= deadline:
                                    raise RuntimeError("B did not enter the accepted launch")
                                time.sleep(0.01)
                            owner.kill()
                        caller_status = caller.wait(timeout=20)
                    finally:
                        if caller.poll() is None:
                            caller.kill()
                a.close()
                out, err = owner.communicate(timeout=10)
                library_status = library.wait(timeout=10)
                if payload == b"drop":
                    diagnostic = stderr.read_bytes()
                    if (not caller_status or not owner.returncode or
                            b"chain completion transport failed" not in diagnostic or
                            b"chain owner reported failure" in diagnostic or
                            b"PASS mounted Styx launch completion" in stdout.read_bytes()):
                        raise RuntimeError(repr((stdout.read_bytes(), diagnostic, out, err)))
                    print("PASS A distinguishes disconnect after accepted launch", mode, flush=True)
                    continue
                if payload == b"fail":
                    if (not caller_status or not owner.returncode or
                            b"chain owner reported failure" not in stderr.read_bytes() or
                            b"chain input mismatch" not in err):
                        raise RuntimeError(repr((stdout.read_bytes(), stderr.read_bytes(), out, err)))
                    print("PASS A receives B's launch failure", mode, flush=True)
                    continue
                if (caller_status or owner.returncode or library_status or fourth.tell() != 8 or
                        b"chain stdout\n" not in stdout.read_bytes() or
                        b"PASS mounted Styx launch completion\n" not in stdout.read_bytes() or
                        b"chain stderr\n" not in stderr.read_bytes()):
                    raise RuntimeError(repr((stdout.read_bytes(), stderr.read_bytes(), out, err)))
                print("PASS A-B-C Styx launch and library call", mode, flush=True)
            finally:
                for peer in (owner, library):
                    if peer.poll() is None:
                        peer.kill()


if __name__ == "__main__":
    main()

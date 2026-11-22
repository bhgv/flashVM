# Copyright (c) 2026 bhgv
# EN: Run stock DisVM and a separate provider over a delegated carrier.
# RU: Запустить штатный DisVM и отдельного провайдера по выданному транспорту.
import os
from pathlib import Path
import select
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
        library_cli(runtime, compiler, disvm, example, work)
        persistent_cli(runtime, compiler, disvm, example, work)


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


# EN: Start a separate Dis owner on the delegated library connection.
# RU: Запустить отдельного владельца Dis по выданному соединению библиотеки.
def start_library(peer, directory, mode, carrier):
    fd = carrier.fileno()
    return subprocess.Popen(
        [peer, mode, "owner.dis"], pass_fds=(fd,), cwd=directory,
        env=dict(os.environ, DVM_TEST_SCALAR_FD=str(fd), DVM_TEST_SCALAR_OWNER="1",
                 DVM_TEST_SCALAR_MOUNTED_OWNER="1", DVM_TEST_SCALAR_LOAD_NAME="wide.dis",
                 DVM_TEST_SCALAR_LOAD_FILE="wide-owner.dis", DVM_TEST_SCALAR_LOAD_EXPORT="echo")
    )


# EN: Export both compiled interfaces on one stock bidirectional library session.
# RU: Опубликовать оба интерфейса на одной штатной двусторонней сессии.
def start_chain_library(runtime, directory, mode, carrier):
    fd = carrier.fileno()
    return subprocess.Popen(
        [runtime, "--styx-library-serve-fd", str(fd),
         "--export", "wide.dis", "typed-wide.dis", "echo",
         "--export", "managed.dis", "managed-target.dis", "changegraph",
         "--", mode, "owner.dis"],
        pass_fds=(fd,), cwd=directory, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )


# EN: Pass distinct FD numbers for the same open socket and require refusal before wire I/O.
# RU: Передать разные номера одного открытого сокета и проверить отказ до обмена.
def reject_carrier_alias(runtime, owner_image):
    client, owner = tcp_pair()
    peer, library = tcp_pair()
    with client, owner, peer, library, library.dup() as copy, owner.dup() as launch_copy:
        for descriptor in (copy, launch_copy):
            refused = subprocess.run(
                [runtime, "--styx-serve-fd", str(owner.fileno()),
                 "--library-fd", str(library.fileno()), "--library-at", "/first",
                 "--library-fd", str(descriptor.fileno()), "--library-at", "/second",
                 "--", owner_image],
                pass_fds=(owner.fileno(), library.fileno(), descriptor.fileno()),
                capture_output=True, timeout=5
            )
            if refused.returncode != 2 or b"must be distinct connected sockets" not in refused.stderr:
                raise RuntimeError(f"duplicate carrier accepted: {refused.stderr!r}")
            if select.select((client, peer), (), (), 0)[0]:
                raise RuntimeError("refused carrier emitted Styx bytes")


# EN: Run A, the executing DisVM B, and the separate Styx library peer C.
# RU: Запустить A, исполняющую DisVM B и отдельного провайдера библиотеки C.
def chain(runtime, compiler, provider, disvm, example, work):
    caller_image = work / "chain-client.dis"
    (work / ".vfs").mkdir()
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
    managed = example / "managed-fid"
    for source, target in ((example / "wide-owner.b", "typed-wide.dis"),
                           (managed / "target.b", "managed-target.dis")):
        subprocess.run([compiler, "-D", "RG", "-I", disvm / "upstream/module", "-I", disvm / "module",
                        "-I", example, "-I", managed, "-o", library_dir / target, source],
                       cwd=library_dir, check=True)
    owner_dir = work / "execution-peer"
    owner_dir.mkdir()
    deps = owner_dir / "deps"
    deps.mkdir()
    subprocess.run([compiler, "-I", example, "-o", deps / "wide.dis",
                    example / "styx-chain-local.b"], cwd=owner_dir, check=True)
    owner_image = owner_dir / "chain-owner.dis"
    subprocess.run([compiler, "-D", "RG", "-I", disvm / "upstream/module", "-I", disvm / "module",
                    "-I", example, "-I", managed, "-o", owner_image, example / "styx-chain-owner.b"], cwd=work, check=True)
    stdin = work / "chain-stdin"
    stdout = work / "chain-stdout"
    stderr = work / "chain-stderr"
    extra = work / "chain-fd3"
    extra.write_bytes(b"skipmore")
    shared = work / "chain-shared-fd"
    for path in ("/", "/remote/../libs", "/.prog", "/.fd/sub", "/.vfs", "/.var", "relative"):
        invalid = subprocess.run(
            [runtime, "--styx-serve-fd", "3", "--library-fd", "4",
             "--library-at", path, "--", owner_image],
            capture_output=True, timeout=5
        )
        if invalid.returncode != 2 or b"usage:" not in invalid.stderr:
            raise RuntimeError(f"invalid library mount path accepted: {path}")
    for fd, path in (("04", "/second"), ("5", "/first"), ("5", "/first/sub"), ("03", "/second")):
        invalid = subprocess.run(
            [runtime, "--styx-serve-fd", "3", "--library-fd", "4", "--library-at", "/first",
             "--library-fd", fd, "--library-at", path, "--", owner_image],
            capture_output=True, timeout=5
        )
        if invalid.returncode != 2 or b"usage:" not in invalid.stderr:
            raise RuntimeError(f"conflicting library delegation accepted: {fd}, {path}")
    reject_carrier_alias(runtime, owner_image)
    for mode, payload, service in (("--no-jit", b"live", "--context"), ("--jit", b"live", "--context"),
                                   ("--no-jit", b"fail", "--context"), ("--no-jit", b"drop", "--context"),
                                   ("--jit", b"drop", "--context"), ("--no-jit", b"live", "--persistent"),
                                   ("--jit", b"live", "--persistent")):
        stdin.write_bytes(payload)
        shared.write_bytes(b"skipAB")
        a, b = tcp_pair()
        lib, c = tcp_pair()
        alternate, d = tcp_pair()
        with a, b, lib, c, alternate, d, stdin.open("rb") as incoming, stdout.open("wb") as outgoing, \
                stderr.open("wb") as errors, extra.open("rb") as fourth, shared.open("r+b") as aliases, \
                start_chain_library(runtime, library_dir, mode, c) as library, \
                start_library(library_peer, library_dir, mode, d) as other, subprocess.Popen(
            [runtime, "--styx-serve-fd", str(b.fileno()), service, "--library-bidir-fd",
             str(lib.fileno()), "--library-at", "/remote-libs", "--library-fd",
             str(alternate.fileno()), "--library-at", "/other-libs", "--", mode, owner_image],
            pass_fds=(b.fileno(), lib.fileno(), alternate.fileno()), cwd=owner_dir,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE
        ) as owner:
            fourth.seek(4)
            aliases.seek(4)
            b.close()
            lib.close()
            c.close()
            alternate.close()
            d.close()
            try:
                with subprocess.Popen(
                    [runtime, "--styx-mount-fd", str(a.fileno()), "--fd3", str(fourth.fileno()),
                     "--fd", "7", str(aliases.fileno()), "--fd", "8", str(aliases.fileno()), "--", mode, caller_image],
                    pass_fds=(a.fileno(), fourth.fileno(), aliases.fileno()), cwd=work, stdin=incoming, stdout=outgoing, stderr=errors
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
                library_output, _ = library.communicate(timeout=10)
                library_status = library.returncode
                other_status = other.wait(timeout=10)
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
                        raise RuntimeError(repr((stdout.read_bytes(), stderr.read_bytes(), out, err, library_output, shared.read_bytes())))
                    print("PASS A receives B's launch failure", mode, flush=True)
                    continue
                if (caller_status or owner.returncode or library_status or other_status or fourth.tell() != 8 or
                        aliases.tell() != 8 or shared.read_bytes() != b"skipABCD" or
                        b"chain stdout\n" not in stdout.read_bytes() or
                        b"PASS mounted Styx launch completion\n" not in stdout.read_bytes() or
                        b"chain stderr\n" not in stderr.read_bytes()):
                    raise RuntimeError(repr((stdout.read_bytes(), stderr.read_bytes(), out, err, library_output, shared.read_bytes())))
                print("PASS A-B-C Styx launch and library call", mode, service, flush=True)
            finally:
                for peer in (owner, library, other):
                    if peer.poll() is None:
                        peer.kill()


# EN: Refuse ambiguous paths before emitting any data on the delegated stream.
# RU: Отклонить неоднозначные пути до отправки данных по выданному потоку.
def reject_mount_path(runtime, work):
    caller, peer = tcp_pair()
    with caller, peer:
        for path in ("relative", "/", "/.", "/route/../other", "/route//other", "/.fd/alias", "/.prog"):
            result = subprocess.run(
                [runtime, "--styx-mount-fd", str(caller.fileno()), "--at", path, "--", "unused.dis"],
                pass_fds=(caller.fileno(),), cwd=work, capture_output=True, timeout=5)
            if result.returncode != 2 or b"canonical absolute Styx mount path" not in result.stderr:
                raise RuntimeError(f"accepted invalid Styx mount path {path!r}: {result.stderr!r}")
            if select.select((peer,), (), (), 0)[0]:
                raise RuntimeError(f"invalid Styx mount path {path!r} touched the carrier")


# EN: Boot both stock CLIs and load two exports through one delegated carrier.
# RU: Запустить два штатных CLI и загрузить два экспорта по одному транспорту.
def library_cli(runtime, compiler, disvm, example, work):
    owner_root, caller_root = work / "library-owner", work / "library-caller"
    owner_root.mkdir()
    caller_root.mkdir()
    (caller_root / "libraries").mkdir()
    (caller_root / "libraries/local-marker").touch()
    (caller_root / "shared-launch.dis").touch()
    (caller_root / "named-launch.dis").touch()
    reject_mount_path(runtime, caller_root)
    for source, image in (
        (disvm / "limbo/scalarowner.b", owner_root / "scalarowner.dis"),
        (example / "scoped-calls-owner.b", owner_root / "first-owner.dis"),
        (example / "scoped-calls-owner.b", owner_root / "second-owner.dis"),
        (example / "styx-library-cli.b", caller_root / "library-cli.dis"),
        (example / "styx-shared-launch-owner.b", owner_root / "launch.dis"),
        (example / "styx-named-launch-owner.b", owner_root / "named.dis"),
    ):
        subprocess.run([compiler, "-D", "R", "-I", disvm / "upstream/module", "-I", disvm / "module",
                        "-I", example, "-o", image, source], cwd=work, check=True)
    library_cli_pair(runtime, owner_root, caller_root, "--no-jit", missing=True)
    for mode in ("--no-jit", "--jit"):
        library_cli_pair(runtime, owner_root, caller_root, mode)
    library_cli_pair(runtime, owner_root, caller_root, "--no-jit", fail=True)


# EN: Relay only test markers between the independent owner and caller roots.
# RU: Передать только тестовые маркеры между независимыми корнями сторон.
def relay_library_markers(owner_root, caller_root, markers, client):
    deadline = time.monotonic() + 7
    for name in markers:
        while (owner_root / name).read_bytes() != b"1":
            if time.monotonic() >= deadline or client.poll() is not None:
                return
            time.sleep(0.005)
        (caller_root / name).write_bytes(b"1")


# EN: Run both commands on one carrier with separate local roots.
# RU: Запустить обе команды на одном транспорте с раздельными локальными корнями.
def library_cli_pair(runtime, owner_root, caller_root, mode, missing=False, fail=False):
    for flag in (owner_root / "launch-fail", caller_root / "expect-launch-fail"):
        if fail:
            flag.touch()
        else:
            flag.unlink(missing_ok=True)
    markers = ("single-ref-pending", "single-ref-cancelled")
    for root in (owner_root, caller_root):
        for mark in markers:
            (root / mark).write_bytes(b"")
    caller, owner = tcp_pair()
    export = "missing" if missing else "update"
    for marker in ("parallel-entered", "parallel-released"):
        (owner_root / marker).write_bytes(b"")
    (owner_root / "launch-marker").write_bytes(b"")
    with caller, owner, subprocess.Popen(
        [runtime, "--styx-library-serve-fd", str(owner.fileno()), "--execute", "launch.dis",
         "--execute-as", "second.dis", "named.dis",
         "--export", "first.dis", "first-owner.dis", "update",
         "--export", "second.dis", "second-owner.dis", export, "--", mode, "scalarowner.dis"],
        pass_fds=(owner.fileno(),), cwd=owner_root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True
    ) as server, subprocess.Popen(
        [runtime, "--styx-mount-fd", str(caller.fileno()), "--at", "/transport", "--", mode, "library-cli.dis"],
        pass_fds=(caller.fileno(),), cwd=caller_root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True
    ) as client:
        caller.close()
        owner.close()
        try:
            if not missing:
                relay_library_markers(owner_root, caller_root, markers, client)
            reported, _ = client.communicate(timeout=10)
            output, _ = server.communicate(timeout=10)
            if missing:
                if not client.returncode or not server.returncode or "cannot publish loaded export second-owner.dis missing" not in output:
                    raise RuntimeError(reported + output)
            elif (client.returncode or server.returncode or "PASS two libraries" not in reported or
                  "PASS named Styx executable on the same mount" not in reported or
                  (owner_root / "launch-marker").read_bytes() != b"ran\n"):
                barriers = {p.name: p.read_bytes() for p in owner_root.glob("parallel-*")}
                raise RuntimeError(reported + output + repr(barriers))
            else:
                print(reported.strip(), mode, flush=True)
        finally:
            for child in (client, server):
                if child.poll() is None:
                    child.kill()


# EN: Reject duplicate and non-basename names before using the carrier.
# RU: Отклонить дубликаты и имена с путём до обращения к транспорту.
def reject_program_names(runtime, owner_image):
    caller, owner = tcp_pair()
    with caller, owner:
        for names in (("../second.dis",), ("second.dis", "second.dis")):
            entries = [part for name in names for part in ("--execute-as", name, owner_image)]
            result = subprocess.run(
                [runtime, "--styx-serve-fd", str(owner.fileno()), "--persistent", *entries, "--", owner_image],
                pass_fds=(owner.fileno(),), capture_output=True, timeout=5,
            )
            if result.returncode != 2 or b"usage:" not in result.stderr:
                raise RuntimeError(f"accepted invalid program publication {names!r}: {result.stderr!r}")
            if select.select((caller,), (), (), 0)[0]:
                raise RuntimeError("refused publication emitted Styx bytes")


# EN: Exercise the embedded listener with the configured image outside the owner cwd.
# RU: Проверить встроенный приёмник с настроенным образом вне cwd владельца.
def persistent_cli(runtime, compiler, disvm, example, work):
    owner_root, caller_root = work / "program-owner", work / "program-caller"
    owner_root.mkdir()
    caller_root.mkdir()
    owner_image = work / "standalone-owner.dis"
    client_image = caller_root / "persistent-cli.dis"
    for source, image in (("styx-shared-launch-owner.b", owner_image),
                          ("styx-named-launch-owner.b", work / "second-owner.dis"),
                          ("styx-persistent-cli.b", client_image)):
        subprocess.run([compiler, "-I", disvm / "upstream/module", "-I", disvm / "module",
                        "-I", example, "-o", image, example / source], cwd=work, check=True)
    (caller_root / "shared-launch.dis").touch()
    (caller_root / "named-launch.dis").touch()
    (caller_root / ".vfs").mkdir()
    reject_program_names(runtime, owner_image)
    for mode in ("--no-jit", "--jit"):
        marks = ("launch-marker", "parallel-entered", "parallel-released")
        for mark in marks:
            (owner_root / mark).write_bytes(b"")
        caller, owner = tcp_pair()
        with caller, owner, subprocess.Popen(
            [runtime, "--styx-serve-fd", str(owner.fileno()), "--persistent",
             "--execute-as", "second.dis", work / "second-owner.dis", "--", mode, owner_image],
            cwd=owner_root, pass_fds=(owner.fileno(),), stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True,
        ) as server:
            owner.close()
            try:
                guest = subprocess.run(
                    [runtime, "--styx-mount-fd", str(caller.fileno()), "--at",
                     "/transport", "--", mode, client_image],
                    cwd=caller_root, pass_fds=(caller.fileno(),), capture_output=True, text=True, timeout=15,
                )
                caller.close()
                try:
                    output, _ = server.communicate(timeout=10)
                except subprocess.TimeoutExpired:
                    server.kill()
                    output, _ = server.communicate()
                    raise RuntimeError(f"persistent owner stuck after disconnect\n{guest.stdout}{guest.stderr}{output}") from None
                marker = "PASS standalone persistent Styx launch"
                if (guest.returncode or server.returncode or marker not in guest.stdout or
                    "PASS named Styx executable on the same mount" not in guest.stdout or any(
                    not (owner_root / mark).read_bytes() for mark in marks
                )):
                    raise RuntimeError(guest.stdout + guest.stderr + output)
                print(marker, mode, flush=True)
            finally:
                if server.poll() is None:
                    server.kill()


if __name__ == "__main__":
    main()

# Copyright (c) 2026 bhgv
# EN: Run the same scoped Dis launch and library fixture locally or on three hosts.
# RU: Запустить один и тот же сценарий Dis локально или на трёх машинах.
import argparse
from concurrent.futures import ThreadPoolExecutor
from contextlib import ExitStack
from pathlib import Path
import socket
import subprocess
import tempfile


EXAMPLE = Path(__file__).resolve().parent
DISVM = EXAMPLE.parents[1]
TIMEOUT = 60


# EN: Build only the images required by the local role, using its own Limbo.
# RU: Собрать только образы локальной роли её собственным Limbo.
def compile_images(compiler, root, images):
    includes = [DISVM / "upstream/module", DISVM / "module", EXAMPLE, EXAMPLE / "managed-fid"]
    command = [compiler, "-D", "RG"]
    for directory in includes:
        command.extend(("-I", directory))
    for source, image in images:
        output = root / image
        output.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run([*command, "-o", output, source], check=True, timeout=TIMEOUT)


def endpoint(value):
    host, colon, port = value.rpartition(":")
    if not colon or not host or len(port) > 5 or not port.isascii() or not port.isdigit() or int(port) > 65535:
        raise argparse.ArgumentTypeError("expected HOST:PORT, for example 127.0.0.1:9030")
    return host.removeprefix("[").removesuffix("]"), int(port)


def listen(address):
    family = socket.AF_INET6 if ":" in address[0] else socket.AF_INET
    listener = socket.create_server(address, family=family)
    listener.settimeout(TIMEOUT)
    return listener


def carrier(sock):
    try:
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    except OSError:
        sock.close()
        raise
    return sock


def accept(listener):
    return carrier(listener.accept()[0])


def connect(address):
    if not address[1]:
        raise ValueError("a remote endpoint must have a nonzero port")
    return carrier(socket.create_connection(address, timeout=TIMEOUT))


# EN: The VM gets the connected descriptors. Release the script's copies at
# exec so the other roles can observe EOF, including after a child failure.
# RU: Передать VM подключённые дескрипторы и закрыть копии скрипта после exec,
# чтобы другие роли получили EOF в том числе при отказе потомка.
def delegate(command, streams, root, *, files=(), stdin=None):
    descriptors = tuple(stream.fileno() for stream in (*streams, *files))
    with tempfile.TemporaryFile(dir=root) as out, tempfile.TemporaryFile(dir=root) as err:
        try:
            child = subprocess.Popen(
                command, cwd=root, pass_fds=descriptors, stdin=stdin, stdout=out, stderr=err,
            )
        finally:
            for stream in streams:
                stream.close()
        with child:
            try:
                child.wait(timeout=TIMEOUT)
            except subprocess.TimeoutExpired:
                child.kill()
                child.wait()
                raise
        out.seek(0)
        err.seek(0)
        stdout = out.read().decode(errors="replace")
        stderr = err.read().decode(errors="replace")
    if child.returncode:
        raise RuntimeError(f"{command[1]} exited {child.returncode}\n{stdout}{stderr}")
    return stdout, stderr


def run_c(options, main_listener, alternate_listener):
    with tempfile.TemporaryDirectory(prefix="styx-c-") as temp:
        root = Path(temp)
        compile_images(options.limbo, root, (
            (DISVM / "limbo/scalarowner.b", "owner.dis"),
            (EXAMPLE / "wide-owner.b", "wide.dis"),
            (EXAMPLE / "styx-chain-fd.b", "chain-fd.dis"),
            (EXAMPLE / "managed-fid/target.b", "managed.dis"),
            (EXAMPLE / "styx-named-launch-owner.b", "nested-root.dis"),
            (EXAMPLE / "styx-chain-leaf.b", "nested-leaf.dis"),
        ))

        (root / "leaf-owner").write_bytes(b"on C")

        def serve(stream, managed):
            exports = ["--export", "wide.dis", "wide.dis", "echo"]
            programs = []
            if managed:
                programs = ["--execute", "nested-root.dis", "--execute-as", "leaf.dis", "nested-leaf.dis"]
                exports.extend(("--export", "managed.dis", "managed.dis", "changegraph",
                                "--export", "chain-fd.dis", "chain-fd.dis", "forward"))
            delegate([options.runtime, "--styx-library-serve-fd", str(stream.fileno()),
                      *programs, *exports, "--", options.mode, "owner.dis"], (stream,), root)

        with ExitStack() as sockets, ThreadPoolExecutor(max_workers=2) as workers:
            first = sockets.enter_context(accept(main_listener))
            second = sockets.enter_context(accept(alternate_listener))
            main = workers.submit(serve, first, True)
            alternate = workers.submit(serve, second, False)
            main.result()
            alternate.result()


def run_b(options, listener, main_library, alternate_library):
    with tempfile.TemporaryDirectory(prefix="styx-b-") as temp, ExitStack() as sockets:
        root = Path(temp)
        compile_images(options.limbo, root, (
            (EXAMPLE / "styx-chain-owner.b", "chain-owner.dis"),
            (EXAMPLE / "styx-chain-forward.b", "chain-fd.dis"),
            (DISVM / "limbo/scalarowner.b", "dispatcher.dis"),
            (EXAMPLE / "styx-chain-local.b", "deps/wide.dis"),
        ))
        (root / "leaf-alias.dis").touch()
        (root / ".var").mkdir()
        (root / ".var/unpublished").write_bytes(b"private on B")
        main = sockets.enter_context(connect(main_library))
        alternate = sockets.enter_context(connect(alternate_library))
        launch = sockets.enter_context(accept(listener))
        delegate([options.runtime, "--styx-library-serve-fd", str(launch.fileno()),
                  "--execute", "chain-owner.dis", "--export", "chain-fd.dis", "chain-fd.dis", "forward",
                  "--library-bidir-fd", str(main.fileno()), "--library-at", "/remote-libs",
                  "--library-bidir-fd", str(alternate.fileno()), "--library-at", "/other-libs",
                  "--", options.mode, "dispatcher.dis"], (launch, main, alternate), root)


def run_a(options, owner):
    with tempfile.TemporaryDirectory(prefix="styx-a-") as temp:
        root = Path(temp)
        compile_images(options.limbo, root, ((EXAMPLE / "styx-chain-client.b", "chain-client.dis"),))
        (root / ".vfs").mkdir()
        (root / "alias.dis").touch()
        (root / "enable-nested-checks").touch()
        (root / ".var").mkdir()
        (root / "stdin").write_bytes(b"live")
        (root / "fd3").write_bytes(b"skipmore")
        (root / "ordinary-input").write_bytes(b"skiplive")
        (root / "fd").mkdir()
        shared = root / "shared"
        shared.write_bytes(b"skipAB")
        with (root / "stdin").open("rb") as incoming, (root / "fd3").open("rb") as fourth, \
                shared.open("r+b") as aliases, connect(owner) as launch:
            fourth.seek(4)
            aliases.seek(4)
            stdout, stderr = delegate(
                [options.runtime, "--styx-mount-fd", str(launch.fileno()), "--fd3", str(fourth.fileno()),
                 "--fd", "7", str(aliases.fileno()), "--fd", "8", str(aliases.fileno()),
                 "--", options.mode, "chain-client.dis"],
                (launch,), root, files=(fourth, aliases), stdin=incoming,
            )
            if (fourth.tell() != 8 or aliases.tell() != 10 or shared.read_bytes() != b"skipABCDEF" or
                    "chain stdout\n" not in stdout or "chain stderr\n" not in stderr or
                    "chain leaf stdout\n" not in stdout or "chain leaf stderr\n" not in stderr or
                    "PASS mounted Styx launch completion\n" not in stdout or
                    "PASS nested Styx launch cancellation\n" not in stdout or
                    "PASS nested Styx shared variables\n" not in stdout or
                    "PASS nested Styx libraries and ordinary FDs\n" not in stdout or
                    (root / "ordinary-output").read_bytes() != b"live"):
                raise RuntimeError(f"wrong result or descriptor state\n{stdout}{stderr}")


def run_local(options):
    with listen(("127.0.0.1", 0)) as b, listen(("127.0.0.1", 0)) as c, \
            listen(("127.0.0.1", 0)) as alternate, ThreadPoolExecutor(max_workers=2) as workers:
        library = workers.submit(run_c, options, c, alternate)
        owner = workers.submit(run_b, options, b, c.getsockname(), alternate.getsockname())
        try:
            run_a(options, b.getsockname())
        except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as error:
            failures = [f"A: {error}"]
            for role, peer in (("B", owner), ("C", library)):
                try:
                    peer.result()
                except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as peer_error:
                    failures.append(f"{role}: {peer_error}")
            raise RuntimeError("\n".join(failures)) from None
        owner.result()
        library.result()


def main():
    parser = argparse.ArgumentParser(description="Run the A → B → C DisVM fixture.")
    parser.add_argument("--runtime", required=True, type=lambda path: Path(path).resolve())
    parser.add_argument("--limbo", required=True, type=lambda path: Path(path).resolve())
    parser.add_argument("--jit", action="store_const", const="--jit", default="--no-jit", dest="mode")
    roles = parser.add_subparsers(dest="role", required=True)
    roles.add_parser("local", help="Run all roles over loopback TCP.")
    a = roles.add_parser("a", help="Connect the caller to B.")
    a.add_argument("--owner", required=True, type=endpoint)
    b = roles.add_parser("b", help="Listen for A and connect both of C's library streams.")
    b.add_argument("--listen", required=True, type=endpoint)
    b.add_argument("--library", required=True, type=endpoint)
    b.add_argument("--alternate", required=True, type=endpoint)
    c = roles.add_parser("c", help="Listen for B's two library connections.")
    c.add_argument("--listen", required=True, type=endpoint)
    c.add_argument("--alternate-listen", required=True, type=endpoint)
    options = parser.parse_args()
    try:
        if options.role == "local":
            run_local(options)
        elif options.role == "a":
            run_a(options, options.owner)
        elif options.role == "b":
            with listen(options.listen) as owner:
                print(f"B listening on {owner.getsockname()}", flush=True)
                run_b(options, owner, options.library, options.alternate)
        else:
            with listen(options.listen) as main_library, listen(options.alternate_listen) as alternate:
                print(f"C listening on {main_library.getsockname()} and {alternate.getsockname()}", flush=True)
                run_c(options, main_library, alternate)
    except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as error:
        parser.exit(1, f"{options.role}: {error}\n")
    print(f"PASS Styx chain {options.role} {options.mode}", flush=True)


if __name__ == "__main__":
    main()

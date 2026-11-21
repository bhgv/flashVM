# Copyright (c) 2026 bhgv
# EN: Build ABI-compatible Dis clients and run them against the real Styx peers.
# RU: Собрать совместимые клиенты Dis и запустить их с настоящими сторонами Styx.
import os
from pathlib import Path
from runpy import run_path
import re
import shlex
import subprocess
import socket
import sys
import tempfile

ordered_tools = run_path(str(Path(__file__).resolve().parents[2] / "order/files/tests/guest.py"))
flags = ordered_tools["flags"]
run = ordered_tools["run"]


# EN: Relink one runtime with fixture bootstrap and source-matched SDK.
# RU: Связать runtime с тестовым bootstrap и совпадающим SDK.
def build_peer(runtime, sdk, work, paired, compiler):
    here = Path(__file__).resolve().parent
    native = sdk / "terminal/CMakeFiles/terminal-scalar-route.dir"
    runtime_target = runtime.parent / "CMakeFiles/disvm.dir"
    interface = here.parents[3] / "examples/remote-scalar/compute.m"
    table = run([str(compiler), "-t", "Compute", str(interface)], work)
    signature = re.search(r'"compute",\s*(0x[0-9a-fA-F]+),', table.stdout)
    if signature is None:
        raise RuntimeError("could not read compiled Compute signature")
    rt = flags(runtime_target)
    styx = flags(native)
    link = shlex.split((runtime_target / "link.txt").read_text())
    native_link = shlex.split((native / "link.txt").read_text())
    objects = []
    transport_source = "pair.c" if paired else "route.c"
    transport = work / "transport.o"
    run([native_link[0], *styx["C_DEFINES"], *styx["C_INCLUDES"], *styx["C_FLAGS"],
         "-DSTYX_SCALAR_GUEST", "-c", str(here / transport_source), "-o", str(transport)], work)
    objects.append(str(transport))
    reverse = work / "reverse.o"
    run([native_link[0], *styx["C_DEFINES"], *styx["C_INCLUDES"], *styx["C_FLAGS"],
         "-c", str(here / "reverse.c"), "-o", str(reverse)], work)
    objects.append(str(reverse))
    for source in (("guest_mount.c", "guest_stdio.c") if paired else ("guest_mount.c", "guest_stdio.c", "guest.c")):
        obj = work / (source + ".o")
        run([link[0], *rt["C_DEFINES"], *rt["C_INCLUDES"], *rt["C_FLAGS"],
             "-DSCALAR_FIXTURE_SIGNATURE=" + signature[1],
             "-c", str(here / source), "-o", str(obj)], work)
        objects.append(str(obj))
    binary = work / "disvm-scalar"
    link = [arg for arg in link if not arg.startswith("-Wl,--dependency-file=")]
    link[link.index("-o") + 1] = str(binary)
    libraries = [str((sdk / "terminal" / arg).resolve()) for arg in native_link if arg.endswith(".a")]
    link += [*objects, "-Wl,--wrap=main", "-Wl,--start-group", *libraries,
             "-Wl,--end-group", *[arg for arg in native_link if arg.startswith("-l") or arg == "-pthread"]]
    run(link, runtime.parent)
    return binary


# EN: Link the test mounts into the source-matched runtime; own temporary products.
# RU: Связать тестовые точки монтирования с совпадающим runtime и удалить временные продукты.
def main():
    runtime, compiler, sdk = map(lambda arg: Path(arg).resolve(), sys.argv[1:4])
    here = Path(__file__).resolve().parent
    root = here.parents[4]
    disvm = root / "disVM"
    example = disvm / "examples/remote-scalar"
    with tempfile.TemporaryDirectory(prefix="scalar-guest-") as name:
        work = Path(name)
        (work / "scalar").mkdir()
        (work / "library").mkdir()
        run([str(compiler), "-I", str(disvm / "upstream/module"), "-I", str(disvm / "module"),
             "-I", str(example), "-o", str(work / "client.dis"), str(example / "styx-client.b")], work)

        binary = build_peer(runtime, sdk, work, False, compiler)
        for mode in ("--no-jit", "--jit"):
            for case in ("value", "error", "missing"):
                markers = {"error": work / "expect-error", "missing": work / "expect-missing"}
                for key, flag in markers.items():
                    if case == key:
                        flag.touch()
                    else:
                        flag.unlink(missing_ok=True)
                env = os.environ.copy()
                if case == "missing":
                    env["DVM_TEST_SCALAR_DROP_REPLY"] = "1"
                result = run([str(binary), mode, str(work / "client.dis")], work, env)
                marker = "PASS Styx Compute " + case
                if marker not in result.stdout:
                    raise RuntimeError(marker + " missing\n" + result.stdout + result.stderr)
                print(marker, mode, flush=True)

        (work / "alias.dis").mkdir()
        (work / "peer").mkdir()
        (work / "blocked").mkdir()
        for name in (".vfs", ".var"):
            (work / name).mkdir()
        run([str(compiler), "-I", str(disvm / "upstream/module"), "-I", str(disvm / "module"),
             "-I", str(example), "-o", str(work / "mounted.dis"), str(example / "styx-mounted.b")], work)
        for mode in ("--no-jit", "--jit"):
            result = run([str(binary), mode, str(work / "mounted.dis")], work,
                         dict(os.environ, DVM_TEST_SCALAR_MOUNTED="1"))
            marker = "PASS typed Styx mounted function"
            if marker not in result.stdout:
                raise RuntimeError(marker + " missing\n" + result.stdout + result.stderr)
            print(marker, mode, flush=True)

        run([str(compiler), "-I", str(disvm / "upstream/module"), "-I", str(disvm / "module"),
             "-I", str(example), "-o", str(work / "caller.dis"), str(example / "styx-caller.b")], work)
        for mode in ("--no-jit", "--jit"):
            result = run([str(binary), mode, str(work / "caller.dis")], work,
                         dict(os.environ, DVM_TEST_SCALAR_MOUNTED="1", DVM_TEST_SCALAR_CALLER="1"))
            marker = "PASS typed Styx caller PID"
            if marker not in result.stdout:
                raise RuntimeError(marker + " missing\n" + result.stdout + result.stderr)
            print(marker, mode, flush=True)
            if "PASS captured caller program tree" not in result.stdout:
                raise RuntimeError("caller program tree check missing\n" + result.stdout + result.stderr)
            print("PASS captured caller program tree", mode, flush=True)

        redirected = work / "redirected-stdin"
        redirected.write_bytes(b"leftnext")
        with redirected.open("rb") as stream:
            result = subprocess.run([str(binary), "--no-jit", str(work / "mounted.dis")],
                                    cwd=work, stdin=stream, capture_output=True, text=True, timeout=30,
                                    env=dict(os.environ, DVM_TEST_SCALAR_MOUNTED="1",
                                             DVM_TEST_SCALAR_HOST_STDIN="1"))
        marker = "PASS redirected host stdin shares its file position"
        if result.returncode or marker not in result.stdout:
            raise RuntimeError(marker + " missing\n" + result.stdout + result.stderr)
        print(marker, flush=True)

        if sys.platform.startswith("linux"):
            client, peer = socket.socketpair()
            with client, peer:
                peer.sendall(b"left")
                result = subprocess.run([str(binary), "--no-jit", str(work / "mounted.dis")],
                                        cwd=work, stdin=client, capture_output=True, text=True, timeout=30,
                                        env=dict(os.environ, DVM_TEST_SCALAR_MOUNTED="1",
                                                 DVM_TEST_SCALAR_HOST_SOCKET="1"))
                marker = "PASS socket stdio preserves blocking flags"
                if result.returncode or marker not in result.stdout:
                    raise RuntimeError(marker + " missing\n" + result.stdout + result.stderr)
                peer.settimeout(2)
                if peer.recv(4, socket.MSG_WAITALL) != b"next":
                    raise RuntimeError("socket stdio reply missing")
            print(marker, flush=True)


if __name__ == "__main__":
    main()

# Copyright (c) 2026 bhgv
# EN: Build ABI-compatible Dis clients and run them against the real Styx peers.
# RU: Собрать совместимые клиенты Dis и запустить их с настоящими сторонами Styx.
import os
from pathlib import Path
from runpy import run_path
import re
import shlex
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
    for source in (("guest_mount.c",) if paired else ("guest_mount.c", "guest.c")):
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
    generator, runtime, compiler, sdk = map(lambda arg: Path(arg).resolve(), sys.argv[1:5])
    here = Path(__file__).resolve().parent
    root = here.parents[4]
    disvm = root / "disVM"
    example = disvm / "examples/remote-scalar"
    with tempfile.TemporaryDirectory(prefix="scalar-guest-") as name:
        work = Path(name)
        (work / "scalar").mkdir()
        (work / "library").mkdir()
        run([str(generator), "--styx-client", str(example / "compute.m"), "compute.m",
             "/scalar/request", "/scalar/reply", "4294967295", "64", str(work / "shim.b")], work)
        for source, image in [(work / "shim.b", "staging.dis"),
                              (disvm / "limbo/styxscalar.b", "styxscalar.dis"),
                              (example / "styx-client.b", "client.dis")]:
            run([str(compiler), "-I", str(disvm / "upstream/module"), "-I", str(disvm / "module"),
                 "-I", str(example), "-o", str(work / image), str(source)], work)

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
                (work / "adapter.dis").write_bytes((work / "staging.dis").read_bytes())
                result = run([str(binary), mode, str(work / "client.dis")], work, env)
                marker = "PASS Styx Compute " + case
                if marker not in result.stdout:
                    raise RuntimeError(marker + " missing\n" + result.stdout + result.stderr)
                print(marker, mode, flush=True)


if __name__ == "__main__":
    main()

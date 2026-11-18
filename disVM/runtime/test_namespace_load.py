# Copyright (c) 2026 bhgv
# EN: Real namespace-aware loader, native cache provenance and rooted modules.
# RU: Настоящий namespace-aware loader, native cache provenance и rooted modules.
from pathlib import Path
import os
import selectors
import shutil
import subprocess
import sys
import tempfile

from test_owner_location import compile_image
from test_debug_mailbox import verify
from relink_private import relink
from test_native_kill import wait_marker


# EN: Hold a genuine opened image, mutate namespace and atomically replace its
# host path. Require cached old code from the old FD and fresh code on next open.
# CONTROL1 must report the specific stale result with ordinary source retirement.
# RU: Держать открытый образ, изменить namespace и атомарно заменить host path.
# Требовать старый code из старого FD и новый code при следующем open. CONTROL1
# обязан показать конкретный stale result со штатным retirement.
def gate(binary, package, work, mode, locked, control, warm=True):
    shutil.copyfile(package / "original-a.dis", package / "source-a/provider.dis")
    read_fd, write_fd = os.pipe()
    process = subprocess.Popen(
        [
            str(binary),
            "--native-spawn",
            mode,
            "--stats",
            *(["--gc-locked-poll"] if locked else []),
            str(package / f"gate-{int(warm)}.dis"),
        ],
        cwd=work,
        env=dict(os.environ, DVM_NAMESPACE_GATE=str(read_fd), DVM_NAMESPACE_WARM=str(int(warm))),
        pass_fds=(read_fd,),
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    os.close(read_fd)
    output, errors = bytearray(), bytearray()
    try:
        with selectors.DefaultSelector() as poll:
            poll.register(process.stdout, selectors.EVENT_READ, output)
            poll.register(process.stderr, selectors.EVENT_READ, errors)
            wait_marker(poll, output, errors, b"NAMESPACELOADOPEN pid=")
            process.stdin.write(b"1")
            process.stdin.flush()
            wait_marker(poll, output, errors, b"NAMESPACELOADREBIND peer=22\n", True)
            replacement = package / "replacement.dis"
            shutil.copyfile(package / "source-b/provider.dis", replacement)
            # Equal mtime/length makes this an actual identity check, not a size/timestamp accident.
            # Равные mtime/length требуют настоящей проверки identity.
            stamp = (package / "source-a/provider.dis").stat()
            os.utime(replacement, ns=(stamp.st_atime_ns, stamp.st_mtime_ns))
            os.replace(replacement, package / "source-a/provider.dis")
            process.stdin.write(b"2")
            process.stdin.flush()
            wait_marker(poll, output, errors, b"NAMESPACELOADFRESH value=", True)
            os.write(write_fd, b"1")
        tail, diagnostics = process.communicate(timeout=45)
        output.extend(tail)
        errors.extend(diagnostics)
    except BaseException:
        process.kill()
        process.communicate()
        raise
    finally:
        os.close(write_fd)
    text = (output + errors).decode(errors="replace")
    expected = 11 if control == 1 else 22
    if (
        process.returncode
        or f"NAMESPACELOADGATE first=11 next=22 old=11 fresh={expected}" not in text
        or "ThreadSanitizer:" in text
    ):
        raise RuntimeError("namespace frozen-input/cache check failed\n" + text[-16000:])
    verify(text, 1)
    if f"NAMESPACELOADCACHE warm={int(warm)}" not in text:
        raise RuntimeError("real cold/warm module lookup was not observed")
    labels = {
        0: "PASS frozen module input",
        1: "DETECTED zero identity stale cache",
        2: "PASS nonrepresentable identity cache refusal",
    }
    print(f"{labels[control]} {mode} locked={locked} warm={warm}")


# EN: Build the public module package with two distinct providers. Borrow all
# paths, return package root; no installed modules or host libraries are used.
# RU: Собрать public package с двумя providers. Paths заимствованы, вернуть
# package root; без установленных модулей и host libraries.
def prepare(compiler, include, root, work):
    example = root.parent / "examples/namespace-load"
    package = work / "package"
    for name in ("source-a", "source-b", "visible"):
        (package / name).mkdir(parents=True, exist_ok=True)
    for name in ("a", "b"):
        compile_image(
            compiler,
            include,
            root,
            example / f"provider-{name}.b",
            package / f"source-{name}/provider.dis",
            (example,),
        )
    shutil.copyfile(package / "source-a/provider.dis", package / "visible/provider.dis")
    shutil.copyfile(package / "source-a/provider.dis", package / "bufio.dis")
    compile_image(compiler, include, root, example / "main.b", package / "main.dis", (example,))
    source = (root / "fixtures/namespace-load/gate.b").read_text()
    if source.count("Warm: con 1;") != 1:
        raise RuntimeError("namespace cache fixture source boundary changed")
    for warm in (0, 1):
        selected = work / "gate-selected.b"
        selected.write_text(source.replace("Warm: con 1;", f"Warm: con {warm};", 1))
        compile_image(compiler, include, root, selected, package / f"gate-{warm}.dis", (example,))
    shutil.copyfile(package / "source-a/provider.dis", package / "original-a.dis")
    shutil.copyfile(package / "source-a/provider.dis", package / "source-a/retained.dis")
    if (package / "original-a.dis").stat().st_size != (
        package / "source-b/provider.dis"
    ).stat().st_size:
        raise RuntimeError("identity fixture requires equal image lengths")
    return package


# EN: Run both dispatch/poll paths from a cwd outside the entry package, proving
# initial package compatibility, namespace separation and joined native ownership.
# RU: Проверить оба dispatch/poll из cwd вне entry package; подтвердить начальный
# package lookup, namespace separation и завершённое native-владение.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    marker = "NAMESPACELOAD workers=8 loads=192 providers=11,22 root=11 old=11 bundled=22"
    with tempfile.TemporaryDirectory(prefix="namespace module load ") as temporary:
        work = Path(temporary)
        package = prepare(compiler, include, root, work)
        for mode in ("--no-jit", "--jit"):
            for locked in (False,):
                result = subprocess.run(
                    [
                        str(runtime),
                        "--native-spawn",
                        mode,
                        "--stats",
                        "--quantum",
                        "7",
                        *(["--gc-locked-poll"] if locked else []),
                        str(package / "main.dis"),
                    ],
                    cwd=work,
                    capture_output=True,
                    text=True,
                    timeout=60,
                )
                output = result.stdout + result.stderr
                if result.returncode or marker not in output or "ThreadSanitizer:" in output:
                    raise RuntimeError("namespace loader acceptance failed\n" + output[-16000:])
                verify(output, 9)
                print(f"PASS namespace loader {mode} locked={locked}")
        for control in (0, 1, 2):
            binary = relink(
                runtime,
                work,
                root / "vfs/test_module_load.c",
                control,
                ["dvm_namespace_module_open", "fvm_port_file_info", "dvm_module_registry_lookup"],
            )
            for mode in ("--no-jit", "--jit"):
                for locked in (False,):
                    for warm in ((False, True) if not control else (True,)):
                        gate(binary, package, work, mode, locked, control, warm)


if __name__ == "__main__":
    main()

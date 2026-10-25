#!/usr/bin/env python3
"""Run retained Inferno RC and full GC bodies against a documented test pool."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from module_parser import function


# Keep algorithm bodies byte-for-byte while isolating allocator/OS test boundaries.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workers", action="store_true")
    parser.add_argument("--flashvm-root", type=Path)
    args = parser.parse_args()
    if args.workers and args.flashvm_root is None:
        parser.error("--workers requires --flashvm-root")
    root = Path(__file__).resolve().parents[1]
    upstream = root / "upstream"
    gc = (upstream / "libinterp/gc.c").read_text()
    gc = "\n".join(line for line in gc.splitlines() if not line.startswith("#include"))
    heap = "\n".join(
        function(upstream, "libinterp/heap.c", name, result)
        for name, result in [
            ("freeptrs", "void"),
            ("freeheap", "void"),
            ("destroy", "void"),
            ("dtype", "Type*"),
            ("freetype", "void"),
            ("initmem", "void"),
        ]
    )
    code = (
        (root / "tests/inferno_gc.c")
        .read_text()
        .replace("PRODUCTION_GC", gc)
        .replace("PRODUCTION_HEAP", heap)
    )
    with tempfile.TemporaryDirectory(prefix="disvm-inferno-gc-") as directory:
        output = Path(directory)
        source = output / "gc.c"
        source.write_text(code)
        command = shlex.split(os.environ.get("CC", "cc"))
        command += shlex.split(os.environ.get("CFLAGS", "-O1 -g"))
        command += ["-I" + str(root / "host/include")]
        command += [
            "-I" + str(upstream / include)
            for include in ["Linux/arm/include", "include", "libinterp"]
        ]
        if args.workers:
            fvm = args.flashvm_root.resolve()
            command += [
                "-DDVM_GC_WORKERS=1",
                "-DDVM_NATIVE_REGISTERS=1",
                "-pthread",
                "-I" + str(root / "include"),
                "-I" + str(upstream / "emu/port"),
                "-I" + str(fvm / "ports"),
            ]
            command += [
                str(root / "host" / name)
                for name in [
                    "native_context.c",
                    "native_registers.c",
                    "native_world.c",
                    "gc_workers.c",
                ]
            ]
            command += [str(fvm / "ports/linux" / name) for name in ["thread.c", "channel_sync.c"]]
        command += [str(source), "-o", str(output / "test")]
        subprocess.run(command, check=True)
        subprocess.run([str(output / "test")], check=True, timeout=20)


if __name__ == "__main__":
    main()

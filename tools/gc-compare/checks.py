#!/usr/bin/env python3
"""Run focused real FlashVM root/finalizer/cycle/string and OOM regressions."""
import argparse
import json
import os
from pathlib import Path
import subprocess
from run import ROOT, libraries

CHECKS = [
    "gc.c",
    "gc_deep_graph.c",
    "gc_resurrection.c",
    "gc_release_resurrection.c",
    "gc_roots.c",
    "gc_nested_scopes.c",
    "gc_native_reentry.c",
    "gc_native_roots.c",
    "gc_traced_strings.c",
    "gc_string_pool.c",
    "gc_stats.c",
    "gc_finalizer_snapshot.c",
    "gc_traced_string_transport.c",
    "gc_finalizer_lookup_allocation.cpp",
    "gc_traced_string_allocation.cpp",
    "gc_release_graph_allocation.cpp",
]


# Reuse real compiled VM archives; compiler/link/test failures abort the comparison checks.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--flashvm-build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    build = args.flashvm_build.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    links = libraries(build)
    results = []
    for name in CHECKS:
        source = ROOT / "flashVM/tests" / name
        obj = args.output / (source.stem + ".o")
        binary = args.output.resolve() / source.stem
        compiler = (
            os.environ.get("CXX", "c++") if source.suffix == ".cpp" else os.environ.get("CC", "cc")
        )
        standard = "-std=c++98" if source.suffix == ".cpp" else "-std=c89"
        subprocess.run(
            [
                compiler,
                standard,
                "-O1",
                "-I" + str(ROOT / "flashVM/include"),
                "-I" + str(build / "generated"),
                "-c",
                str(source),
                "-o",
                str(obj),
            ],
            check=True,
        )
        subprocess.run(
            [os.environ.get("CXX", "c++"), str(obj), *links, "-pthread", "-o", str(binary)],
            check=True,
        )
        result = subprocess.run(
            [str(binary)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            timeout=120,
        )
        results.append(
            {
                "test": name,
                "returncode": result.returncode,
                "stdout": result.stdout,
                "stderr": result.stderr,
            }
        )
        if result.returncode:
            raise RuntimeError(name + " failed: " + result.stdout + result.stderr)
    (args.output / "checks.json").write_text(json.dumps(results, indent=2) + "\n")
    print("PASS " + str(len(results)) + " real FlashVM GC regressions")


if __name__ == "__main__":
    main()

"""Optional real-header acceptance and native export-name parity; no runtime ABI claim."""

import argparse
from pathlib import Path
import re
import subprocess


# Inspect a real upstream .m through the same bounded native CLI used by fixture tests.
def inspect(tool, path, valid=True):
    result = subprocess.run([str(tool), str(path)], capture_output=True, timeout=20)
    if not (result.returncode == (0 if valid else 1)):
        raise AssertionError((path.name, result.stderr))
    if not (b"ERROR: AddressSanitizer" not in result.stderr):
        raise AssertionError(result.stderr)
    if not (b"runtime error:" not in result.stderr):
        raise AssertionError(result.stderr)
    if not valid:
        if not (b"E-6" in result.stderr and (not result.stdout)):
            raise AssertionError(result.stderr)
        return []
    rows = []
    for line in result.stdout.decode().splitlines()[2:]:
        fields = line.split("\t")
        if not (len(fields) == 10 and fields[0] == "decl"):
            raise AssertionError(line)
        rows.append(fields)
    return rows


# Match direct .m function names to checked-in native Runtab names, not signatures/layout/maps.
def native_exports(root, tool, stem, module):
    rows = inspect(tool, root / "module" / f"{stem}.m")
    scopes = [r[1] for r in rows if r[2:4] == ["module", module] and r[4] == "-"]
    if not (len(scopes) == 1):
        raise AssertionError(scopes)
    functions = [r[3] for r in rows if r[2] == "function" and r[4] == scopes[0]]
    header = (root / "libinterp" / f"{stem}mod.h").read_text()
    native = re.findall(r'^\s*"([^"\n]+)",', header, re.MULTILINE)
    if not (len(functions) == len(set(functions))):
        raise AssertionError(
            "condition check failed: compiler/limbo/frontend/tests/interface_upstream.py:34"
        )
    if not (len(native) == len(set(native))):
        raise AssertionError(
            "condition check failed: compiler/limbo/frontend/tests/interface_upstream.py:35"
        )
    if not (set(functions) == set(native)):
        raise AssertionError((stem, set(functions) ^ set(native)))
    return len(functions)


# Use an explicitly selected checkout; no network, includes or compiler execution is implicit.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", required=True, type=Path)
    parser.add_argument("--tool", required=True, type=Path)
    args = parser.parse_args()
    paths = sorted((args.upstream / "module").glob("*.m"))
    if not (paths):
        raise AssertionError("No upstream module headers")
    accepted = 0
    for path in paths:
        # runt.m is an implementation include aggregator, not an explicit interface catalog.
        valid = path.name != "runt.m"
        inspect(args.tool, path, valid)
        accepted += valid
    exports = native_exports(args.upstream, args.tool, "sys", "Sys")
    exports += native_exports(args.upstream, args.tool, "math", "Math")
    print(
        f"{accepted} real interfaces accepted; {len(paths)-accepted} deliberate rejection; "
        f"{exports} Sys/Math native export names matched"
    )


if __name__ == "__main__":
    main()

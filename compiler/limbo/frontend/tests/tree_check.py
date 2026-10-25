"""Validate full grammar tree topology, byte spans, precedence and syntax parity."""

from pathlib import Path
import subprocess
import sys
import tempfile
from check import CASES


# Parse the native inspector's numeric views without depending on internal tree storage.
def inspect(tool, path, accepted):
    result = subprocess.run([tool, str(path)], capture_output=True, timeout=15)
    if not (result.returncode == (0 if accepted else 1)):
        raise AssertionError(result.stderr)
    if not (b"ERROR: AddressSanitizer" not in result.stderr):
        raise AssertionError(result.stderr)
    if not (b"runtime error:" not in result.stderr):
        raise AssertionError(result.stderr)
    if not accepted:
        if not (not result.stdout):
            raise AssertionError(
                "condition check failed: compiler/limbo/frontend/tests/tree_check.py:17"
            )
        return []
    lines = result.stdout.decode().splitlines()
    source_size = int(lines[0].split("\t")[1])
    count = int(lines[1].split("\t")[1])
    nodes = []
    for i, line in enumerate(lines[2:]):
        tag, index, kind, begin, end, parent, child, sibling = line.split("\t")
        if not (tag == "node" and int(index) == i):
            raise AssertionError(
                "condition check failed: compiler/limbo/frontend/tests/tree_check.py:25"
            )
        nodes.append(
            (
                kind,
                int(begin),
                int(end),
                *(None if value == "-" else int(value) for value in (parent, child, sibling)),
            )
        )
    if not (len(nodes) == count and count):
        raise AssertionError(
            "condition check failed: compiler/limbo/frontend/tests/tree_check.py:34"
        )
    if not (nodes[0][0] == "Limbo" and nodes[0][3] is None and (nodes[0][5] is None)):
        raise AssertionError(
            "condition check failed: compiler/limbo/frontend/tests/tree_check.py:35"
        )
    if not (source_size == len(path.read_bytes())):
        raise AssertionError(
            "condition check failed: compiler/limbo/frontend/tests/tree_check.py:36"
        )
    seen = {0}
    for i, node in enumerate(nodes):
        kind, begin, end, parent, child, sibling = node
        if not (0 <= begin <= end <= source_size):
            raise AssertionError(node)
        previous = i
        while child is not None:
            if not (previous < child < count and child not in seen):
                raise AssertionError((i, child))
            seen.add(child)
            nested = nodes[child]
            if not (nested[3] == i and begin <= nested[1] <= nested[2] <= end):
                raise AssertionError(
                    "condition check failed: compiler/limbo/frontend/tests/tree_check.py:46"
                )
            previous = child
            child = nested[5]
    if not (len(seen) == count):
        raise AssertionError(
            "condition check failed: compiler/limbo/frontend/tests/tree_check.py:49"
        )
    return nodes


# Compare every existing fixture, then inspect operator precedence with authored UTF-8 bytes.
def main():
    tool = sys.argv[1]
    kinds = set()
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "program.b"
        for name, source, valid in CASES:
            path.write_text(source, encoding="utf-8")
            nodes = inspect(tool, path, valid)
            kinds.update(node[0] for node in nodes)
        for source in (
            "",
            "# only comment\n",
            "\ufeff# comment\r\n",
            "implement A;\r\n",
        ):
            path.write_bytes(source.encode())
            inspect(tool, path, True)
        source = "# 中文\nimplement A; f(){ x:=1+2*3**4; }\n"
        path.write_bytes(source.encode())
        nodes = inspect(tool, path, True)
        fragments = {
            (node[0], source.encode()[node[1] : node[2]].decode().strip()) for node in nodes
        }
        if not (("Additive", "1+2*3**4") in fragments):
            raise AssertionError(
                "condition check failed: compiler/limbo/frontend/tests/tree_check.py:77"
            )
        if not (("Product", "2*3**4") in fragments):
            raise AssertionError(
                "condition check failed: compiler/limbo/frontend/tests/tree_check.py:78"
            )
        if not (("Power", "3**4") in fragments):
            raise AssertionError(
                "condition check failed: compiler/limbo/frontend/tests/tree_check.py:79"
            )
        for bad in (b"\xff", b"\x00", b"\xe0\x80\x80"):
            path.write_bytes(bad)
            inspect(tool, path, False)
        # A resource-bound tree parse may fail while syntax-only acceptance remains unchanged.
        path.write_bytes(b"f(){" + b"a;" * 30000 + b"}")
        result = subprocess.run([tool, str(path)], capture_output=True, timeout=20)
        if not (result.returncode == 1 and b"E-5" in result.stderr):
            raise AssertionError(result.stderr)
        if not (not result.stdout):
            raise AssertionError(
                "condition check failed: compiler/limbo/frontend/tests/tree_check.py:87"
            )
        path.write_bytes(b"implement Recovery;")
        inspect(tool, path, True)
    print(
        f"{len(CASES)} shared cases; {len(kinds)} production kinds; precedence/topology/limits passed"
    )


if __name__ == "__main__":
    main()

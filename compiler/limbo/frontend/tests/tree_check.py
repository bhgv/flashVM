"""Validate full grammar tree topology, byte spans, precedence and syntax parity."""

from pathlib import Path
import subprocess
import sys
import tempfile
from check import CASES


# Parse the native inspector's numeric views without depending on internal tree storage.
def inspect(tool, path, accepted):
    result = subprocess.run([tool, str(path)], capture_output=True, timeout=15)
    assert result.returncode == (0 if accepted else 1), result.stderr
    assert b"ERROR: AddressSanitizer" not in result.stderr, result.stderr
    assert b"runtime error:" not in result.stderr, result.stderr
    if not accepted:
        assert not result.stdout
        return []
    lines = result.stdout.decode().splitlines()
    source_size = int(lines[0].split("\t")[1])
    count = int(lines[1].split("\t")[1])
    nodes = []
    for i, line in enumerate(lines[2:]):
        tag, index, kind, begin, end, parent, child, sibling = line.split("\t")
        assert tag == "node" and int(index) == i
        nodes.append(
            (
                kind,
                int(begin),
                int(end),
                *(
                    None if value == "-" else int(value)
                    for value in (parent, child, sibling)
                ),
            )
        )
    assert len(nodes) == count and count
    assert nodes[0][0] == "Limbo" and nodes[0][3] is None and nodes[0][5] is None
    assert source_size == len(path.read_bytes())
    seen = {0}
    for i, node in enumerate(nodes):
        kind, begin, end, parent, child, sibling = node
        assert 0 <= begin <= end <= source_size, node
        previous = i
        while child is not None:
            assert previous < child < count and child not in seen, (i, child)
            seen.add(child)
            nested = nodes[child]
            assert nested[3] == i and begin <= nested[1] <= nested[2] <= end
            previous = child
            child = nested[5]
    assert len(seen) == count
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
            (node[0], source.encode()[node[1] : node[2]].decode().strip())
            for node in nodes
        }
        assert ("Additive", "1+2*3**4") in fragments
        assert ("Product", "2*3**4") in fragments
        assert ("Power", "3**4") in fragments
        for bad in (b"\xff", b"\x00", b"\xe0\x80\x80"):
            path.write_bytes(bad)
            inspect(tool, path, False)
        # A resource-bound tree parse may fail while syntax-only acceptance remains unchanged.
        path.write_bytes(b"f(){" + b"a;" * 30000 + b"}")
        result = subprocess.run([tool, str(path)], capture_output=True, timeout=20)
        assert result.returncode == 1 and b"E-5" in result.stderr, result.stderr
        assert not result.stdout
        path.write_bytes(b"implement Recovery;")
        inspect(tool, path, True)
    print(
        f"{len(CASES)} shared cases; {len(kinds)} production kinds; precedence/topology/limits passed"
    )


if __name__ == "__main__":
    main()

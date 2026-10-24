"""Independent scope, grouped-name and source-span expectations for the C catalog CLI."""

from pathlib import Path
import subprocess
import sys
import tempfile


# Run one explicit interface snapshot and require exact catalog rows on successful acceptance.
def inspect(tool, source, accepted=True):
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "module.m"
        path.write_bytes(source.encode())
        result = subprocess.run([tool, str(path)], capture_output=True, timeout=15)
    assert result.returncode == (0 if accepted else 1), result.stderr
    assert b"ERROR: AddressSanitizer" not in result.stderr, result.stderr
    assert b"runtime error:" not in result.stderr, result.stderr
    if not accepted:
        assert not result.stdout
        return []
    lines = result.stdout.decode().splitlines()
    assert int(lines[0].split("\t")[1]) == len(source.encode())
    rows = []
    for line in lines[2:]:
        tag, index, kind, name, scope, node, begin, end, first, last = line.split("\t")
        assert tag == "decl" and int(index) == len(rows)
        assert int(node) >= 1
        assert 0 <= int(first) <= int(last) <= len(source.encode())
        assert source.encode()[int(begin) : int(end)].decode() == name
        rows.append((kind, name, None if scope == "-" else int(scope)))
    assert int(lines[1].split("\t")[1]) == len(rows)
    return rows


# Cover every syntactic category, exact Unicode names and repeated names in independent modules.
def main():
    tool = sys.argv[1]
    source = (
        '# lead\ninclude "absent.m"; M:module{'
        "A:adt{field:int;}; f:fn(x:array of byte):int; "
        "x # grouped\n , y:chan of string; C:con 2; T:type int; "
        "E:exception(int); I:import value; N:module{f:fn();}; }; "
        "Other:module{f:fn();}; 中文:int;"
    )
    assert inspect(tool, source) == [
        ("include", "", None),
        ("module", "M", None),
        ("adt", "A", 1),
        ("function", "f", 1),
        ("value", "x", 1),
        ("value", "y", 1),
        ("constant", "C", 1),
        ("alias", "T", 1),
        ("exception", "E", 1),
        ("import", "I", 1),
        ("module", "N", 1),
        ("function", "f", 10),
        ("module", "Other", None),
        ("function", "f", 12),
        ("value", "中文", None),
    ]
    assert inspect(tool, source.replace("\n", "\r\n")) == inspect(tool, source)
    assert inspect(tool, "") == []
    assert inspect(tool, "# comment only\n") == []
    assert inspect(tool, "M:module{x:Unknown;};")[1] == ("value", "x", 0)
    for source in (
        "x:int;x:con 2;",
        "x,x:int;",
        "M:module{x:int;x:fn();};",
        "implement M;",
        "f(){}",
        "x:=3;",
        "A,B:module{};",
        "@",
    ):
        inspect(tool, source, False)
    print("interface categories/scopes/grouping/UTF-8/CRLF/rejections passed")


if __name__ == "__main__":
    main()

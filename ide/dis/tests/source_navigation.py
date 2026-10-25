"""Connect a real FIR source diagnostic to the portable Limbo document service.

All native compiler/emulator dependencies are explicit. The generated constants
are test transport only; deployed IDE code remains Limbo and uses typed channels.
"""

import argparse
import json
import os
from pathlib import Path
import subprocess


# Preserve real compiler stderr and drive the checked-in, commented Limbo client.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-compiler", required=True, type=Path)
    parser.add_argument("--limbo", required=True, type=Path)
    parser.add_argument("--module-dir", required=True, type=Path)
    parser.add_argument("--emu", required=True, type=Path)
    parser.add_argument("--build", required=True, type=Path)
    parser.add_argument("--mode", required=True, choices=("0", "1"))
    args = parser.parse_args()
    source_root = Path(__file__).resolve().parent.parent
    example = source_root / "examples/compiler-location"
    root = args.build.resolve() / ("source-navigation-" + args.mode)
    root.mkdir(parents=True, exist_ok=True)
    for name in ("dev", "chan", "prog", "fd", "net", "net.alt", "env"):
        (root / name).mkdir(exist_ok=True)
    temporary = root / "tmp"
    temporary.mkdir(exist_ok=True)
    environment = os.environ.copy()
    for name in ("TMPDIR", "TMP", "TEMP"):
        environment[name] = str(temporary)
    source_path = root / "source.as"
    source_path.write_bytes((example / "source.as").read_bytes())
    source = source_path.read_text(encoding="utf-8")
    output = root / "rejected.fir"
    output.write_bytes(b"previous successful artifact")
    result = subprocess.run(
        [
            str(args.source_compiler.resolve()),
            "as1",
            "--input",
            str(source_path),
            str(output),
            "--diagnostics-json",
        ],
        capture_output=True,
        timeout=10,
        env=environment,
    )
    if not (result.returncode == 1 and (not result.stdout)):
        raise AssertionError(result)
    (root / "diagnostic.json").write_bytes(result.stderr)
    diagnostic = json.loads(result.stderr)
    if not (diagnostic["schema"] == 1 and diagnostic["kind"] == "semantic"):
        raise AssertionError("condition check failed: ide/dis/tests/source_navigation.py:56")
    if not (diagnostic["has_location"] is True):
        raise AssertionError("condition check failed: ide/dis/tests/source_navigation.py:57")
    first = diagnostic["byte_offset"]
    last = first + diagnostic["byte_length"]
    if not (0 <= first <= last <= len(source.encode("utf-8"))):
        raise AssertionError("condition check failed: ide/dis/tests/source_navigation.py:60")
    if not (source.encode("utf-8")[first:last] == b"missing"):
        raise AssertionError("condition check failed: ide/dis/tests/source_navigation.py:61")
    if not (output.read_bytes() == b"previous successful artifact"):
        raise AssertionError("condition check failed: ide/dis/tests/source_navigation.py:62")
    constants = (
        "# Generated from the retained source and actual FIR diagnostic.\n"
        "Source: con %s;\nByteFirst: con %d;\nByteLast: con %d;\n"
        % (json.dumps(source, ensure_ascii=False), first, last)
    )
    (root / "diagnostic-values.m").write_text(constants, encoding="utf-8")
    client = root / "main.b"
    client.write_bytes((example / "main.b").read_bytes())
    for source_file, module in [
        (source_root / "idecore.b", "idecore.dis"),
        (client, "compilerlocation.dis"),
    ]:
        subprocess.run(
            [
                str(args.limbo.resolve()),
                "-I",
                str(root),
                "-I",
                str(source_root),
                "-I",
                str(args.module_dir.resolve()),
                "-o",
                str(root / module),
                str(source_file),
            ],
            check=True,
            timeout=30,
            env=environment,
        )
    result = subprocess.run(
        [
            str(args.emu.resolve()),
            "-c" + args.mode,
            "-r" + str(root),
            "-d",
            "/compilerlocation.dis",
        ],
        cwd=root,
        capture_output=True,
        timeout=30,
        env=environment,
    )
    text = result.stdout.decode("utf-8", errors="replace")
    if not (result.returncode == 0 and "FAIL" not in text):
        raise AssertionError((text, result.stderr))
    if not ("PASS idecore compilerlocation " in text):
        raise AssertionError((text, result.stderr))
    print(text.strip())


if __name__ == "__main__":
    main()

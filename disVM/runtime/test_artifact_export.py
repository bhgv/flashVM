"""Verify real Dis binary exports against independent compiler CLI output and OS sentinels."""

from pathlib import Path
import json
import os
import sys
import tempfile
from test_compiler_service import call


# Explicit runtime/module/mode/CLIs supply actual execution and byte oracles.
# Every test owns one temporary directory; no user path is created or removed.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    mode = "--jit" if sys.argv[3] == "jit" else "--no-jit"
    source_cli, fir_cli, swl = (Path(value).resolve() for value in sys.argv[4:7])
    with tempfile.TemporaryDirectory(prefix="Dis artifact export ") as directory:
        work = Path(directory)
        environment = dict(os.environ, TMPDIR=directory, TMP=directory, TEMP=directory)
        (work / "sentinel.swf").write_bytes(b"keep existing\x00\xff")
        (work / "linked.swf").symlink_to("sentinel.swf")
        (work / "dangling.swf").symlink_to("missing-target")
        result = call([runtime, mode, "--stats", "--quantum", 1, module], work, environment)
        if (
            "PASS artifact export:" not in result.stdout
            or "tables_live=0 keys_live=0" not in result.stderr
        ):
            raise RuntimeError(result.stdout + result.stderr)
        if (work / "raw.bin").read_bytes() != bytes(range(256)):
            raise RuntimeError("binary I/O converted raw bytes")
        if (work / "sentinel.swf").read_bytes() != b"keep existing\x00\xff":
            raise RuntimeError("exclusive export modified an existing object")
        if not (work / "linked.swf").is_symlink() or not (work / "dangling.swf").is_symlink():
            raise RuntimeError("exclusive export replaced a symlink")
        for name in ("missing-target", "absent.dis", "wrong.dis", "stale.swf", "invalid.bin"):
            if (work / name).exists():
                raise RuntimeError("rejected export created " + name)
        debug = json.loads((work / "export1.firdbg").read_text())
        source = work / "source.b"
        source.write_text(debug["source"], encoding="utf-8")
        raw, optimized = work / "raw.fir", work / "optimized.fir"
        expected_debug, expected = work / "expected.firdbg", work / "expected"
        call(
            [
                source_cli,
                "limbo",
                "--input",
                source,
                raw,
                "--module",
                "ide_build",
                "-g",
                expected_debug,
            ],
            work,
            environment,
        )
        call([fir_cli, "opt", raw, optimized], work, environment)
        for target, ending in ((1, "dis"), (2, "fir"), (3, "swf")):
            if target == 1:
                call([fir_cli, "dis", optimized, expected], work, environment)
            elif target == 2:
                expected.write_bytes(optimized.read_bytes())
            else:
                call([fir_cli, "object", optimized, work / "object.swo"], work, environment)
                call(
                    [swl, "link", expected, "--size", 640, 480, work / "object.swo"],
                    work,
                    environment,
                )
            if (work / f"export{target}.{ending}").read_bytes() != expected.read_bytes():
                raise RuntimeError("exported binary differs from independent CLI for " + ending)
            if json.loads((work / f"export{target}.firdbg").read_text()) != json.loads(
                expected_debug.read_text()
            ):
                raise RuntimeError("exported original debug information changed")
        executed = call(
            [runtime, mode, "--inspect-words", 0, 2, work / "export1.dis"], work, environment
        )
        if "DATA 0 42\nDATA 8 1\n" not in executed.stdout:
            raise RuntimeError("exported Dis did not execute 42")
    print("PASS artifact export: exact CLI bytes/debug, real execution, OS sentinels " + mode)


if __name__ == "__main__":
    main()

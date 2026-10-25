"""Validate actual CLI diagnostics, byte spans, option parsing and rollback."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile


# Invoke real generated compiler; preserve existing destination on rejection.
def main():
    compiler = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="fir-json-") as directory:
        root = Path(directory)
        source, output = root / "source.as", root / "output.fir"
        for language in ("as1", "as2", "as3"):
            source.write_text("/*中*/\ttrace(missing);", encoding="utf-8")
            output.write_bytes(b"old artifact")
            command = [compiler, language, "--input", str(source), str(output)]
            result = subprocess.run(
                command + ["--diagnostics-json"], capture_output=True, timeout=10
            )
            if not (result.returncode == 1 and (not result.stdout)):
                raise AssertionError(result)
            item = json.loads(result.stderr)
            if not (
                item
                == dict(
                    schema=1,
                    kind="semantic",
                    code=0,
                    has_location=True,
                    byte_offset=14,
                    byte_length=7,
                    line=1,
                    column=15,
                    message="source line 1: read before initialized declaration",
                )
            ):
                raise AssertionError(item)
            if not (output.read_bytes() == b"old artifact"):
                raise AssertionError("condition check failed: fir/v1/tests/source_json.py:36")
            legacy = subprocess.run(command, capture_output=True, timeout=10)
            if not (legacy.returncode == 1 and legacy.stderr.startswith(b"source line 1:")):
                raise AssertionError(legacy)
        # Recovery/later lowering failures cannot replace the first identifier.
        source.write_text("trace(missing);\ntrace(other);", encoding="utf-8")
        result = subprocess.run(command + ["--diagnostics-json"], capture_output=True, timeout=10)
        item = json.loads(result.stderr)
        if not (
            result.returncode == 1
            and (item["byte_offset"], item["byte_length"], item["line"]) == (6, 7, 1)
        ):
            raise AssertionError(item)
        # A syntax failure at EOF is a point; canonical byte columns include tabs.
        source.write_bytes(b"\ttrace(1 +")
        result = subprocess.run(command + ["--diagnostics-json"], capture_output=True, timeout=10)
        item = json.loads(result.stderr)
        if not (result.returncode == 1 and item["kind"] == "syntax" and (item["code"] > 0)):
            raise AssertionError("condition check failed: fir/v1/tests/source_json.py:52")
        if not ((item["byte_offset"], item["byte_length"], item["column"]) == (10, 0, 11)):
            raise AssertionError(item)
        for bad, kind in [(b"\xff", "encoding"), (b"x\0", "encoding"), (b" " * 65537, "limit")]:
            source.write_bytes(bad)
            result = subprocess.run(
                command + ["--diagnostics-json"], capture_output=True, timeout=10
            )
            item = json.loads(result.stderr)
            if not (result.returncode == 1 and item["kind"] == kind and (not item["has_location"])):
                raise AssertionError("condition check failed: fir/v1/tests/source_json.py:60")
            if not (
                all((item[key] == 0 for key in ("byte_offset", "byte_length", "line", "column")))
            ):
                raise AssertionError("condition check failed: fir/v1/tests/source_json.py:61")
        source.write_text("trace(42);")
        for options in (
            ["--diagnostics-json", "--module", "json"],
            ["--module", "json", "--diagnostics-json"],
        ):
            result = subprocess.run(command + options, capture_output=True, timeout=10)
            if not (result.returncode == 0 and (not result.stderr) and (not result.stdout)):
                raise AssertionError(result)
            if not (b"module @json" in output.read_bytes()):
                raise AssertionError("condition check failed: fir/v1/tests/source_json.py:69")
        for options in (
            ["--diagnostics-json"] * 2,
            ["--module", "x", "--module", "y"],
            ["--module"],
            ["--unknown"],
        ):
            result = subprocess.run(command + options, capture_output=True, timeout=10)
            if not (result.returncode == 2 and result.stderr.startswith(b"usage:")):
                raise AssertionError(result)
        result = subprocess.run(
            command + ["--diagnostics-json", "--module", "bad-name"],
            capture_output=True,
            timeout=10,
        )
        if not (result.returncode == 1 and json.loads(result.stderr)["kind"] == "argument"):
            raise AssertionError("condition check failed: fir/v1/tests/source_json.py:83")
        # A multiline RHS must not move an operator error to the last literal.
        limbo = "implement Op; Op: module { value: fn(): int; }; value(): int { return 8 /\n2; }"
        source.write_text(limbo)
        result = subprocess.run(
            [compiler, "limbo", "--input", str(source), str(output), "--diagnostics-json"],
            capture_output=True,
            timeout=10,
        )
        item = json.loads(result.stderr)
        if not (result.returncode == 1 and item["kind"] == "semantic"):
            raise AssertionError("condition check failed: fir/v1/tests/source_json.py:93")
        if not (
            limbo.encode()[item["byte_offset"] : item["byte_offset"] + item["byte_length"]] == b"/"
        ):
            raise AssertionError(item)
        source.unlink()
        result = subprocess.run(command + ["--diagnostics-json"], capture_output=True, timeout=10)
        if not (result.returncode == 2 and json.loads(result.stderr)["kind"] == "input"):
            raise AssertionError("condition check failed: fir/v1/tests/source_json.py:99")
    print("PASS real source CLI JSON, legacy, options, UTF-8 and rollback")


if __name__ == "__main__":
    main()

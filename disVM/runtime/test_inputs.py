"""Reject incompatible/failed input and diagnose a real blocked-channel deadlock."""

from pathlib import Path
import subprocess
import sys
import tempfile


# Compile small genuine modules and inspect actual executable exit/diagnostic contracts.
# Arguments: runner, matching compiler, Sys header directory, legacy image. Returns on success.
def main():
    runtime, compiler, headers, legacy = map(lambda value: str(Path(value).resolve()), sys.argv[1:])
    with tempfile.TemporaryDirectory(dir=Path(runtime).parent) as temporary:
        root = Path(temporary)
        bad = root / "bad.dis"
        bad.write_bytes(b"not dis")
        cases = [
            (bad, "bad magic"),
            (Path(legacy), "bad magic"),
            (root / "missing.dis", "cannot open"),
        ]
        for path, expected in cases:
            result = subprocess.run([runtime, str(path)], capture_output=True, text=True, timeout=5)
            if result.returncode == 0 or "PASS" in result.stdout:
                raise RuntimeError("failed input reported success: " + str(path))
            if expected != "cannot open" and expected not in result.stderr:
                raise RuntimeError("missing input diagnostic: " + result.stderr)
        payload = root / "input.bin"
        payload.write_bytes(b"ab\x00cd")
        import json

        quoted = json.dumps(str(payload))
        read_source = (
            'implement Probe; include "sys.m"; Probe: module { init: fn(); }; '
            "init() { s := load Sys Sys->PATH; f := s->open(" + quoted + ", Sys->OREAD); "
            'if (f == nil) raise "fail: open"; b := array[3] of byte; '
            'if (s->read(f, b, 2) != 2 || b[0] != byte 97 || b[1] != byte 98) raise "fail: read"; '
            'if (s->readn(f, b, 3) != 3 || b[0] != byte 0 || b[1] != byte 99 || b[2] != byte 100) raise "fail: readn"; '
            'if (s->read(f, b, 1) != 0) raise "fail: EOF"; s->print("PASS file input\\n"); }'
        )
        for name, source, diagnostic in [
            ("read", read_source, None),
            (
                "blocked",
                "implement Probe; Probe: module { init: fn(); }; init() { c := chan of int; x := <-c; }",
                "deadlock",
            ),
            (
                "unsupported",
                'implement Probe; include "sys.m"; Probe: module { init: fn(); }; init() { s := load Sys Sys->PATH; s->sleep(1); }',
                "Sys_sleep",
            ),
            (
                "bytes",
                'implement Probe; include "sys.m"; Probe: module { init: fn(); }; negative := -1; init() { s := load Sys Sys->PATH; if (negative != -1) raise "fail: signed constant"; a := array[2] of byte; a[0] = byte 65; a[1] = byte 66; if (string a != "AB") raise "fail: byte descriptor"; s->print("PASS byte descriptor\\n"); }',
                None,
            ),
        ]:
            text = root / (name + ".b")
            output = root / (name + ".dis")
            text.write_text(source)
            subprocess.run(
                [compiler, "-g", "-I", headers, "-o", str(output), str(text)], check=True
            )
            result = subprocess.run(
                [runtime, str(output)], capture_output=True, text=True, timeout=5
            )
            if diagnostic:
                if result.returncode == 0 or diagnostic not in result.stderr:
                    raise RuntimeError("missing runtime failure: " + result.stdout + result.stderr)
            elif result.returncode or "PASS" not in result.stdout:
                raise RuntimeError(
                    "module-local byte array was rejected: " + result.stdout + result.stderr
                )
    print(
        "PASS native input rejection, deadlock, unsupported Sys and module-local byte descriptors"
    )


if __name__ == "__main__":
    main()

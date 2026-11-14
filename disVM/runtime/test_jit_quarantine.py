# Copyright (c) 2026 bhgv
# EN: Exercise real function quarantine and optional GDB argument forwarding.
# RU: Проверить настоящий карантин функций и optional forwarding аргументов GDB.
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
from relink_private import relink
from test_owner_location import compile_image
from resident_evidence import evidence


# EN/RU: Require concrete evidence; no expected-error run is reported as application success.
# Требовать concrete evidence; expected-error run не считается application success.
def require(condition, message):
    if not condition:
        raise RuntimeError(message)


# EN: Compile the public example and relink a safe explicit backend defect. Verify
# exact function-local fallback and ordinary error distinction with real VM runs.
# A fake GDB checks exec arguments only, not actual debugger behavior.
# RU: Собрать public example и relink safe explicit backend defect. Проверить
# function-local fallback и отличие ordinary error настоящими VM runs.
# Fake GDB проверяет только exec arguments, не actual debugger behavior.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="jit quarantine ") as temporary:
        work = Path(temporary)
        image = work / "main.dis"
        compile_image(
            compiler, include, root, root.parent / "examples/jit-quarantine/main.b", image
        )
        binary = relink(
            runtime,
            work,
            root / "test_jit_quarantine_probe.c",
            0,
            ["dvm_runtime_fast", "dvm_close_worker_stop", "dvm_jit_native_error"],
        )
        for locked in (False, True):
            arguments = ["--jit", "--stats", "--native-spawn", "--max-instructions", "10000"]
            if locked:
                arguments.append("--gc-locked-poll")
            result = subprocess.run(
                [str(binary), *arguments, str(image)], capture_output=True, text=True, timeout=40
            )
            output = result.stdout + result.stderr
            require(result.returncode == 0, output)
            evidence(output, 8)
            require("QUARANTINEDEMO caught=1 results=9,14" in output, output)
            require(
                output.count("JITFAULT ") == 1 and "action=interpret-function replay=0" in output,
                output,
            )
            interval = re.search(r"function=\[(\d+),(\d+)\)", output)
            require(interval, output)
            low, high = map(int, interval.groups())
            entries = re.findall(r"QUARANTINEENTRY pc=(\d+) op=\d+", output)
            require(entries and all(not low <= int(pc) < high for pc in entries), output)
            probe = re.search(r"QUARANTINEPROBE injected=1 other=([1-9][0-9]*)", output)
            require(probe and "ThreadSanitizer:" not in output, output)
            print("PASS JIT function quarantine locked=" + str(locked), flush=True)
        broken = relink(
            runtime,
            work,
            root / "test_jit_quarantine_probe.c",
            1,
            ["dvm_runtime_fast", "dvm_close_worker_stop", "dvm_jit_native_error"],
        )
        result = subprocess.run(
            [str(broken), "--jit", "--stats", "--max-instructions", "10000", str(image)],
            capture_output=True,
            text=True,
            timeout=40,
        )
        output = result.stdout + result.stderr
        require(result.returncode == 0 and "QUARANTINEDEMO caught=1 results=9,14" in output, output)
        evidence(output, 8)
        require("JITFAULT " not in output and "QUARANTINEENTRY " in output, output)
        print("DETECTED missing JIT function quarantine classification", flush=True)
        fake = work / "gdb"
        fake.write_text("#!/bin/sh\nprintf '%s\\n' \"$@\"\n")
        fake.chmod(0o700)
        result = subprocess.run(
            [str(runtime), "--gdb", "--jit", str(image)],
            env={**os.environ, "PATH": str(work)},
            capture_output=True,
            text=True,
            timeout=5,
        )
        require(
            result.returncode == 0
            and result.stdout.splitlines()[-4:] == ["--args", str(runtime), "--jit", str(image)],
            result.stdout + result.stderr,
        )
        print("PASS optional GDB argument forwarding (launcher only)", flush=True)


if __name__ == "__main__":
    main()

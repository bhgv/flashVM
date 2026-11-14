# Copyright (c) 2026 bhgv
# EN: Real diagnostic mailbox ownership, quota, native wait and retirement acceptance.
# RU: Actual diagnostic mailbox ownership, quota, native wait и retirement acceptance.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from relink_private import relink
from resident_evidence import evidence


# EN/RU: Validate normal completion, strict native GC and zero retained diagnostic storage.
# Проверить normal completion, strict native GC и отсутствие retained diagnostic storage.
def verify(output, children):
    evidence(output, children)
    row = re.search(
        r"NATIVEDEBUGMAILBOX created=(\d+) destroyed=(\d+) live=0 requests=0 pending=0", output
    )
    native_units = int("NATIVEDEBUGUNIT quota=1 fifo=1 close=1" in output)
    if not row or int(row[1]) != children + native_units or row[1] != row[2]:
        raise RuntimeError("diagnostic mailbox lifetime leak\n" + output[-10000:])


# EN: Compile matched private fixtures; wrapper observes only this owner's native identity.
# Each VM has bounded subprocess lifetime; compile/crash/sanitizer failure never counts.
# RU: Собрать matched private fixtures; wrapper читает только native identity своего owner.
# VM subprocess bounded; compile/crash/sanitizer failure не засчитываются.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    source = (root / "fixtures/debug-mailbox/main.b").read_text()
    with tempfile.TemporaryDirectory(prefix="debug mailbox ") as temporary:
        work = Path(temporary)
        images = []
        for mode in range(3):
            fixture = work / ("guest" + str(mode) + ".b")
            fixture.write_text(source.replace("Mode: con 0;", "Mode: con " + str(mode) + ";"))
            image = fixture.with_suffix(".dis")
            built = subprocess.run(
                [
                    str(compiler),
                    "-I",
                    str(include),
                    "-I",
                    str(root.parent / "module"),
                    "-o",
                    str(image),
                    str(fixture),
                ],
                capture_output=True,
                text=True,
            )
            if built.returncode:
                raise RuntimeError(
                    "diagnostic fixture compile failed\n" + built.stdout + built.stderr
                )
            images.append(image)
        symbols = [
            "dvm_diagnostic_answer",
            "dvm_diagnostic_pending",
            "dvm_diagnostic_forget_requester",
        ]
        binary = relink(runtime, work, root / "test_debug_mailbox_probe.c", 0, symbols)
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                result = subprocess.run(
                    [
                        str(binary),
                        "--stats",
                        mode,
                        *(["--gc-locked-poll"] if locked else []),
                        str(images[0]),
                    ],
                    capture_output=True,
                    text=True,
                    timeout=90,
                )
                output = result.stdout + result.stderr
                if (
                    result.returncode
                    or "ThreadSanitizer:" in output
                    or "runtime error:" in output
                    or "NATIVEDEBUGOWNER bound=1 budget=2" not in output
                    or "NATIVEDEBUGUNIT quota=1 fifo=1 close=1" not in output
                    or "PASS debug mailbox: owner replies bounded credits requester cleanup ordinary traffic native wait shared outputs joined"
                    not in output
                ):
                    raise RuntimeError("diagnostic mailbox acceptance failed\n" + output[-12000:])
                verify(output, children=6)
                print("PASS debug mailbox " + mode + " locked=" + str(locked), flush=True)
        for control, children, diagnostic in (
            (1, 1, "DETECTED debug mailbox wake ignored pending query"),
            (2, 2, "DETECTED debug mailbox requester quota was not released"),
        ):
            broken = relink(runtime, work, root / "test_debug_mailbox_probe.c", control, symbols)
            result = subprocess.run(
                [str(broken), "--stats", "--no-jit", str(images[control])],
                capture_output=True,
                text=True,
                timeout=90,
            )
            output = result.stdout + result.stderr
            if (
                result.returncode
                or "ThreadSanitizer:" in output
                or "runtime error:" in output
                or diagnostic not in output
                or "PASS debug mailbox" in output
            ):
                raise RuntimeError("diagnostic omission not detected cleanly\n" + output[-12000:])
            verify(output, children)
            print("PASS debug mailbox omission " + str(control), flush=True)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Check actual strict-C89 identity tutorial output against an independent integer PCM oracle."""
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile


def report(text):
    """Check bounded ownership evidence without assuming native object ABI sizes."""
    if not ("queued_duplicate=consumed retired=1 endpoint=8 accepted_frames=32" in text):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_swf_queued_identity_example.py:13"
        )
    if not ("after_vm_teardown=1" in text):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_swf_queued_identity_example.py:14"
        )
    value = re.search(r"identity_bytes=(\d+)", text)
    if not (value and int(value[1]) == 65536 * struct.calcsize("d")):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_swf_queued_identity_example.py:16"
        )


def main():
    """Run real native ownership/control paths and inspect every explicitly exported sample."""
    executable = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="flashvm-queued-id-c89-") as directory:
        root = Path(directory)
        result = subprocess.run(
            [executable, "--help"], cwd=root, capture_output=True, text=True, check=True
        )
        if not ("Usage:" in result.stdout and (not list(root.iterdir()))):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_queued_identity_example.py:27"
            )
        result = subprocess.run([executable], cwd=root, capture_output=True, text=True, check=True)
        report(result.stdout)
        if not (not list(root.iterdir())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_queued_identity_example.py:30"
            )
        output = root / "accepted.pcm"
        output.write_bytes(b"replace this old longer file" * 100)
        result = subprocess.run(
            [executable, str(output)], capture_output=True, text=True, check=True
        )
        report(result.stdout)
        block = []
        for frame in range(8):
            position = frame * 44100
            index, phase = divmod(position, 48000)
            left, right = 1000 + 100 * index, 1000 + 100 * min(index + 1, 6)
            value = (left * (48000 - phase) + right * phase) // 48000
            block.extend((value, value))
        block.extend([0] * 16)
        if not (output.read_bytes() == struct.pack("=64h", *block * 2)):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_queued_identity_example.py:45"
            )
        result = subprocess.run(
            [executable, str(root / "missing" / "output.pcm")], capture_output=True
        )
        if not (result.returncode and (not (root / "missing").exists())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_queued_identity_example.py:49"
            )
        absent = root / "absent.pcm"
        result = subprocess.run([executable, str(absent), "--bad"], capture_output=True)
        if not (result.returncode and (not absent.exists())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_queued_identity_example.py:52"
            )
    print(
        "PASS C89 queued identity: exact PCM, retirement/queued lifetime, failed write/retry, post-VM controls, CLI and file errors"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())

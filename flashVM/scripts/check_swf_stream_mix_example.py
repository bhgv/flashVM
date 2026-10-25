#!/usr/bin/env python3
"""Check actual C89 stream/event export using independent absolute rational sample positions."""
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile


def trunc(n, d):
    """Match specified truncation toward zero using exact Python integer arithmetic."""
    return (1 if n >= 0 else -1) * (abs(n) // d)


def sample(source, frame):
    """Hold each source's last sample inside its own duration; zero after exhaustion."""
    at, phase = divmod(frame * 44100, 48000)
    if at >= len(source):
        return 0
    right = source[min(at + 1, len(source) - 1)]
    return trunc(source[at] * (48000 - phase) + right * phase, 48000)


def main():
    """Exercise real file replacement, error cleanup, exact/adaptive tails and no-side-effect CLI usage."""
    executable = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="flashvm-stream-mix-") as directory:
        root = Path(directory)
        subprocess.run([executable], cwd=root, check=True, stdout=subprocess.PIPE)
        if not (not list(root.iterdir())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:31"
            )
        output = root / "mixed.pcm"
        invalid = subprocess.run(
            [executable, str(output), "extra"],
            cwd=root,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if not (invalid.returncode and (not output.exists())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:39"
            )
        output.write_bytes(b"replaced data" * 50)
        result = subprocess.run(
            [executable, str(output)], cwd=root, check=True, stdout=subprocess.PIPE, text=True
        )
        if not ("Mixed 16 frames after source VM teardown." in result.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:44"
            )
        base = [30000, -30000, 20000, -20000, 10000, -10000, 30000, -30000]
        expected = []
        for frame in range(16):
            wide = sample(base, frame) + sample([30000] * 8, frame) + sample([-30000] * 12, frame)
            value = max(-32768, min(32767, wide))
            expected.extend([value, value])
        if not (output.read_bytes() == struct.pack("=" + "h" * len(expected), *expected)):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:51"
            )
        if not (expected[:2] == [30000, 30000]):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:52"
            )  # Intermediate stream+positive clipping would corrupt this.
        if not (expected[18:28] == [-30000] * 10 and expected[28:] == [0] * 4):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:56"
            )
        queued_output = root / "queue.pcm"
        queued = subprocess.run(
            [executable, str(queued_output), "--queue"],
            cwd=root,
            check=True,
            stdout=subprocess.PIPE,
            text=True,
        )
        if not ("Queue retained 5 rejected blocks." in queued.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:65"
            )
        if not (queued_output.read_bytes() == output.read_bytes()):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:66"
            )
        tails_output = root / "tails.pcm"
        tails = subprocess.run(
            [executable, str(tails_output), "--tails"],
            cwd=root,
            check=True,
            stdout=subprocess.PIPE,
            text=True,
        )
        if not ("Queue retained 5 rejected blocks." in tails.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:75"
            )
        if not (tails_output.read_bytes() == output.read_bytes()):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:76"
            )
        bounded_output = root / "bounded.pcm"
        bounded = subprocess.run(
            [executable, str(bounded_output), "--bounded-tails"],
            cwd=root,
            check=True,
            stdout=subprocess.PIPE,
            text=True,
        )
        storage = re.search(r"Mix queue buffers=(\d+) fixed=(\d+) retained=(\d+)", bounded.stdout)
        if not (storage and int(storage[1]) == 32 and (int(storage[2]) > 0)):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:86"
            )
        if not (int(storage[3]) == int(storage[1]) + int(storage[2])):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:87"
            )
        if not (bounded_output.read_bytes() == output.read_bytes()):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:88"
            )
        clock_output = root / "clock.pcm"
        clocked = subprocess.run(
            [executable, str(clock_output), "--clock"],
            cwd=root,
            check=True,
            stdout=subprocess.PIPE,
            text=True,
        )
        if not ("Shared clock submitted=16 queued=16 played=0" in clocked.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:97"
            )
        if not (
            "Shared clock submitted=16 queued=0 played=16 seconds=0.000333333" in clocked.stdout
        ):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:98"
            )
        if not (clock_output.read_bytes() == output.read_bytes()):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:99"
            )
        identity_output = root / "identities.pcm"
        tracked = subprocess.run(
            [executable, str(identity_output), "--identities"],
            cwd=root,
            check=True,
            stdout=subprocess.PIPE,
            text=True,
        )
        if not ("Identity id=8 end=14 played=0 active=1" in tracked.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:108"
            )
        if not ("Identity id=8 end=14 played=16 active=0" in tracked.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:109"
            )
        if not (identity_output.read_bytes() == output.read_bytes()):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:110"
            )
        gated_output = root / "gated.pcm"
        gated = subprocess.run(
            [executable, str(gated_output), "--gated"],
            cwd=root,
            check=True,
            stdout=subprocess.PIPE,
            text=True,
        )
        gated_values = []
        for frame in range(17):
            wide = (
                (sample(base, frame - 8) if frame >= 8 else 0)
                + sample([30000] * 8, frame)
                + sample([-30000] * 12, frame)
            )
            gated_values.extend([max(-32768, min(32767, wide))] * 2)
        if not (
            gated_output.read_bytes() == struct.pack("=" + "h" * len(gated_values), *gated_values)
        ):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:127"
            )
        if not ("Mixed 17 frames after source VM teardown." in gated.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:130"
            )
        if not ("Queue retained 5 rejected blocks." in gated.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:131"
            )
        if not ("Identity id=8 end=14 played=17 active=0" in gated.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:132"
            )
        headroom_output = root / "headroom.pcm"
        headroom = subprocess.run(
            [executable, str(headroom_output), "--headroom"],
            cwd=root,
            check=True,
            stdout=subprocess.PIPE,
            text=True,
        )
        headroom_values = []
        for frame in range(14):
            wide = sample([30000] * 8, frame) + sample([-30000] * 12, frame)
            headroom_values.extend([max(-32768, min(32767, wide))] * 2)
        if not (
            headroom_output.read_bytes()
            == struct.pack("=" + "h" * len(headroom_values), *headroom_values)
        ):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:145"
            )
        if not ("Mixed 14 frames after source VM teardown." in headroom.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:148"
            )
        if not ("Queue retained 14 rejected blocks." in headroom.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:149"
            )
        if not ("Shared clock submitted=14 queued=14 played=0" in headroom.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:150"
            )
        if not (
            "Shared clock submitted=14 queued=0 played=14 seconds=0.000291667" in headroom.stdout
        ):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:151"
            )
        if not ("Identity id=8 end=14 played=0 active=1" in headroom.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:152"
            )
        if not ("Identity id=8 end=14 played=14 active=0" in headroom.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:153"
            )
        finite_output = root / "finite.pcm"
        finite = subprocess.run(
            [executable, str(finite_output), "--finite"],
            cwd=root,
            check=True,
            stdout=subprocess.PIPE,
            text=True,
        )
        if not (
            finite_output.read_bytes()
            == struct.pack("=" + "h" * len(headroom_values), *headroom_values)
        ):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:162"
            )
        if not ("Mixed 14 frames after source VM teardown." in finite.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:165"
            )
        if not ("Queue retained 4 rejected blocks." in finite.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:166"
            )
        if not ("Shared clock submitted=14 queued=14 played=0" in finite.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:167"
            )
        if not (
            "Shared clock submitted=14 queued=0 played=14 seconds=0.000291667" in finite.stdout
        ):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:168"
            )
        if not ("Identity id=8 end=14 played=14 active=0" in finite.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:169"
            )
        origin_output = root / "origins.pcm"
        origin = subprocess.run(
            [executable, str(origin_output), "--origins"],
            cwd=root,
            check=True,
            stdout=subprocess.PIPE,
            text=True,
        )
        if not (
            origin_output.read_bytes() == struct.pack("=" + "h" * len(gated_values), *gated_values)
        ):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:178"
            )
        if not ("Snapshot submitted=17 played=0 queued=17" in origin.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:181"
            )
        if not ("Snapshot submitted=17 played=17 queued=0" in origin.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:182"
            )
        if not ("Origins base=9/0 tail=8/0" in origin.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:183"
            )
        if not ("Origins base=9/9 tail=8/8" in origin.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:184"
            )
        if not ("Mixed 17 frames after source VM teardown." in origin.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:185"
            )
        if not ("Queue retained 5 rejected blocks." in origin.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:186"
            )
        failed = subprocess.run(
            [executable, str(root / "missing" / "file.pcm")],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if not (failed.returncode and (not (root / "missing").exists())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_mix_example.py:192"
            )
    print(
        "PASS ten-mode stream/event C89 output, gated/adaptive/finite retry, single saturation and native cleanup"
    )


if __name__ == "__main__":
    main()

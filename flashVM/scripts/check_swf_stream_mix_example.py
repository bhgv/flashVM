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
    with tempfile.TemporaryDirectory(prefix='flashvm-stream-mix-') as directory:
        root = Path(directory)
        subprocess.run([executable], cwd=root, check=True, stdout=subprocess.PIPE)
        assert not list(root.iterdir())
        output = root / 'mixed.pcm'
        invalid = subprocess.run([executable, str(output), 'extra'], cwd=root,
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert invalid.returncode and not output.exists()
        output.write_bytes(b'replaced data' * 50)
        result = subprocess.run([executable, str(output)], cwd=root, check=True,
                                stdout=subprocess.PIPE, text=True)
        assert 'Mixed 16 frames after source VM teardown.' in result.stdout
        base = [30000, -30000, 20000, -20000, 10000, -10000, 30000, -30000]
        expected = []
        for frame in range(16):
            wide = sample(base, frame) + sample([30000] * 8, frame) + sample([-30000] * 12, frame)
            value = max(-32768, min(32767, wide))
            expected.extend([value, value])
        assert output.read_bytes() == struct.pack('=' + 'h' * len(expected), *expected)
        assert expected[:2] == [30000, 30000]  # Intermediate stream+positive clipping would corrupt this.
        assert expected[18:28] == [-30000] * 10 and expected[28:] == [0] * 4
        queued_output = root / 'queue.pcm'
        queued = subprocess.run([executable, str(queued_output), '--queue'], cwd=root, check=True,
                                stdout=subprocess.PIPE, text=True)
        assert 'Queue retained 5 rejected blocks.' in queued.stdout
        assert queued_output.read_bytes() == output.read_bytes()
        tails_output = root / 'tails.pcm'
        tails = subprocess.run([executable, str(tails_output), '--tails'], cwd=root, check=True,
                               stdout=subprocess.PIPE, text=True)
        assert 'Queue retained 5 rejected blocks.' in tails.stdout
        assert tails_output.read_bytes() == output.read_bytes()
        bounded_output = root / 'bounded.pcm'
        bounded = subprocess.run([executable, str(bounded_output), '--bounded-tails'], cwd=root, check=True,
                                 stdout=subprocess.PIPE, text=True)
        storage = re.search(r'Mix queue buffers=(\d+) fixed=(\d+) retained=(\d+)', bounded.stdout)
        assert storage and int(storage[1]) == 32 and int(storage[2]) > 0
        assert int(storage[3]) == int(storage[1]) + int(storage[2])
        assert bounded_output.read_bytes() == output.read_bytes()
        clock_output = root / 'clock.pcm'
        clocked = subprocess.run([executable, str(clock_output), '--clock'], cwd=root, check=True,
                                 stdout=subprocess.PIPE, text=True)
        assert 'Shared clock submitted=16 queued=16 played=0' in clocked.stdout
        assert 'Shared clock submitted=16 queued=0 played=16 seconds=0.000333333' in clocked.stdout
        assert clock_output.read_bytes() == output.read_bytes()
        identity_output = root / 'identities.pcm'
        tracked = subprocess.run([executable, str(identity_output), '--identities'], cwd=root, check=True,
                                 stdout=subprocess.PIPE, text=True)
        assert 'Identity id=8 end=14 played=0 active=1' in tracked.stdout
        assert 'Identity id=8 end=14 played=16 active=0' in tracked.stdout
        assert identity_output.read_bytes() == output.read_bytes()
        gated_output = root / 'gated.pcm'
        gated = subprocess.run([executable, str(gated_output), '--gated'], cwd=root, check=True,
                               stdout=subprocess.PIPE, text=True)
        gated_values = []
        for frame in range(17):
            wide = (sample(base, frame - 8) if frame >= 8 else 0) + sample([30000] * 8, frame) + sample([-30000] * 12, frame)
            gated_values.extend([max(-32768, min(32767, wide))] * 2)
        assert gated_output.read_bytes() == struct.pack('=' + 'h' * len(gated_values), *gated_values)
        assert 'Mixed 17 frames after source VM teardown.' in gated.stdout
        assert 'Queue retained 5 rejected blocks.' in gated.stdout
        assert 'Identity id=8 end=14 played=17 active=0' in gated.stdout
        headroom_output = root / 'headroom.pcm'
        headroom = subprocess.run([executable, str(headroom_output), '--headroom'], cwd=root,
                                  check=True, stdout=subprocess.PIPE, text=True)
        headroom_values = []
        for frame in range(14):
            wide = sample([30000] * 8, frame) + sample([-30000] * 12, frame)
            headroom_values.extend([max(-32768, min(32767, wide))] * 2)
        assert headroom_output.read_bytes() == struct.pack('=' + 'h' * len(headroom_values), *headroom_values)
        assert 'Mixed 14 frames after source VM teardown.' in headroom.stdout
        assert 'Queue retained 14 rejected blocks.' in headroom.stdout
        assert 'Shared clock submitted=14 queued=14 played=0' in headroom.stdout
        assert 'Shared clock submitted=14 queued=0 played=14 seconds=0.000291667' in headroom.stdout
        assert 'Identity id=8 end=14 played=0 active=1' in headroom.stdout
        assert 'Identity id=8 end=14 played=14 active=0' in headroom.stdout
        finite_output = root / 'finite.pcm'
        finite = subprocess.run([executable, str(finite_output), '--finite'], cwd=root,
                                check=True, stdout=subprocess.PIPE, text=True)
        assert finite_output.read_bytes() == struct.pack('=' + 'h' * len(headroom_values), *headroom_values)
        assert 'Mixed 14 frames after source VM teardown.' in finite.stdout
        assert 'Queue retained 4 rejected blocks.' in finite.stdout
        assert 'Shared clock submitted=14 queued=14 played=0' in finite.stdout
        assert 'Shared clock submitted=14 queued=0 played=14 seconds=0.000291667' in finite.stdout
        assert 'Identity id=8 end=14 played=14 active=0' in finite.stdout
        origin_output = root / 'origins.pcm'
        origin = subprocess.run([executable, str(origin_output), '--origins'], cwd=root,
                                check=True, stdout=subprocess.PIPE, text=True)
        assert origin_output.read_bytes() == struct.pack('=' + 'h' * len(gated_values), *gated_values)
        assert 'Origins base=9/0 tail=8/0' in origin.stdout
        assert 'Origins base=9/9 tail=8/8' in origin.stdout
        assert 'Mixed 17 frames after source VM teardown.' in origin.stdout
        assert 'Queue retained 5 rejected blocks.' in origin.stdout
        failed = subprocess.run([executable, str(root / 'missing' / 'file.pcm')],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert failed.returncode and not (root / 'missing').exists()
    print('PASS ten-mode stream/event C89 output, gated/adaptive/finite retry, single saturation and native cleanup')


if __name__ == '__main__':
    main()

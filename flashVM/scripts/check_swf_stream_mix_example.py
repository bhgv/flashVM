#!/usr/bin/env python3
"""Check actual C89 stream/event export using independent absolute rational sample positions."""
from pathlib import Path
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
    """Exercise real file replacement, error cleanup, tails and no-side-effect CLI usage."""
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
        failed = subprocess.run([executable, str(root / 'missing' / 'file.pcm')],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert failed.returncode and not (root / 'missing').exists()
    print('PASS exact stream/event C89 output, unclipped cancellation, event tail and native cleanup')


if __name__ == '__main__':
    main()

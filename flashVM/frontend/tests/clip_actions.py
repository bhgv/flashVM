#!/usr/bin/env python3
"""Actual SDL dummy-video player executes retained EnterFrame actions before later parent frames."""
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
from smoke import rect, tag, matrix
from broadcaster import push, variable


def movie(version, wrong=False):
    # Independent raw ClipEventEnterFrame flag, UI16 before SWF6 and UI32 afterwards.
    code = variable('this') + push('seen') + push(42) + b'\x4f\x00'
    flags = lambda n: struct.pack('<H' if version < 6 else '<I', n)
    events = b'\0\0' + flags(2) + flags(2) + struct.pack('<I', len(code)) + code + flags(0)
    place = b'\xa6' + struct.pack('<HH', 1, 7) + matrix(0, 0) + b'actor\0' + events
    failure = push('clip action missing') + b'\x2a'
    check = variable('actor') + push('seen') + b'\x4e' + push(43 if wrong else 42) + b'\x49'
    check += b'\x9d\x02\0' + struct.pack('<h', len(failure)) + failure + b'\x00'
    sprite = struct.pack('<HH', 7, 1) + tag(1) + tag(0)
    content = rect(0, 640, 0, 640) + struct.pack('<HH', 120 * 256, 2)
    content += tag(39, sprite) + tag(26, place) + tag(1) + tag(12, check) + tag(1) + tag(0)
    return b'FWS' + bytes([version]) + struct.pack('<I', len(content) + 8) + content


def main():
    executable, mode = sys.argv[1:]
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='flashvm-test-no-audio-driver')
    with tempfile.TemporaryDirectory(prefix='flashvm-clip-actions-') as directory:
        for version in range(5, 9):
            path = Path(directory) / ('clip-%d.swf' % version)
            for wrong in (False, True):
                path.write_bytes(movie(version, wrong))
                result = subprocess.run([executable, '--mute', '--frames', '2', '--fps', '120', '--stats', mode, str(path)],
                                        env=env, capture_output=True, text=True, timeout=15)
                output = result.stdout + result.stderr
                if mode == '--jit' and 'requested JIT backend unavailable' in output:
                    return 77
                if wrong:
                    assert result.returncode != 0 and 'failed (VM status' in output, output
                else:
                    assert result.returncode == 0, (version, mode, output)
                    stats = re.search(r'frames=(\d+) native=(\d+)', output)
                    assert stats and int(stats[1]) == 2, output
                    assert (int(stats[2]) > 0) == (mode == '--jit'), output
            print('PASS ClipActions SWF%d %s and wrong-value control' % (version, mode))
    return 0


if __name__ == '__main__':
    sys.exit(main())

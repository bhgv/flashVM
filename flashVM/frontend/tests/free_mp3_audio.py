#!/usr/bin/env python3
"""Compare actual free-format SDL streams and three-codec event mixing against libmpg123."""
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import wave
import zlib
from smoke import rect
from stream_segments import converted
from mp3_events import movie as event_movie
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'scripts'))
from check_free_mp3_example import fixtures, movie as stream_movie
from free_mp3_reference import Reference




def launch(executable, root, wire, mode, fps):
    """Read the exact PCM accepted by the real SDL backend and its submission markers."""
    source, base = root/'movie.swf', root/'trace'
    source.write_bytes(wire)
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software',
               SDL_AUDIODRIVER='dummy', FVM_AUDIO_TRACE=str(base))
    result = subprocess.run([executable, mode, '--once', '--fps', str(fps), '--stats', str(source)],
                            env=env, capture_output=True, text=True, timeout=10)
    if mode == '--jit' and 'requested JIT backend unavailable' in result.stderr:
        raise NotImplementedError('native JIT unavailable')
    assert result.returncode == 0, result.stderr
    with wave.open(str(base)+'.wav') as output:
        assert (output.getnchannels(), output.getframerate(), output.getsampwidth()) == (2, 48000, 2)
        actual = list(struct.iter_unpack('<hh', output.readframes(output.getnframes())))
    report = Path(str(base)+'.txt').read_text()
    assert 'truncated=0 dropped_events=0' in report
    return actual, report, result.stderr


def compare(actual, expected):
    """Compare every accepted channel sample after one independent final clipping step."""
    assert len(actual) == len(expected), (len(actual), len(expected))
    error = max((abs(a-b) for pair, reference in zip(actual, expected) for a, b in zip(pair, reference)), default=0)
    assert error <= 3, error


def main(cases_fixtures=None, label='free-format'):
    """Exercise packet grouping, header compression, both engines and independent event onset."""
    try:
        reference = Reference()
    except OSError:
        print('SKIP optional system libmpg123.so.0 independent oracle')
        return 77
    cases = 0
    with tempfile.TemporaryDirectory(prefix='flashvm-free-sdl-') as directory:
        root = Path(directory)
        for fixture in (fixtures() if cases_fixtures is None else cases_fixtures):
            samples = reference.decode(root, fixture)
            rate = fixture[1]
            mp3 = list(struct.iter_unpack('<hh', converted([samples], rate)))
            voices = [mp3,
                      list(struct.iter_unpack('<hh', converted([[(30000, 30000)]*4608], 44100))),
                      list(struct.iter_unpack('<hh', converted([[(-28000, -28000)]*4096], 44100)))]
            for compressed in (False, True):
                for mode in ('--interpreter', '--jit'):
                    for group in (1, 2, 5):
                        wire = stream_movie(fixture, group)
                        body = rect(0, 640, 0, 640)+wire[10:]
                        wire = (b'CWS' if compressed else b'FWS')+b'\x08'+struct.pack('<I', len(body)+8)+(zlib.compress(body) if compressed else body)
                        actual, _, _ = launch(sys.argv[1], root, wire, mode, 1000)
                        compare(actual, mp3)
                        cases += 1
                    actual, report, stats = launch(sys.argv[1], root, event_movie(fixture[0], 5760, compressed), mode, 1)
                    counter = 'native' if mode == '--jit' else 'interpreter'
                    assert re.search(counter+r'=[1-9]\d*', stats), stats
                    starts = [row.split() for row in report.splitlines() if len(row.split()) == 5 and row.split()[1] == 'event']
                    assert len(starts) == 3 and [int(x[2]) for x in starts] == [0, 1, 2]
                    expected = [[0, 0] for _ in actual]
                    for marker, voice in zip(starts, voices):
                        first = int(marker[4])
                        assert first+len(voice) <= len(actual)
                        for i, pair in enumerate(voice):
                            for channel in range(2):
                                expected[first+i][channel] += pair[channel]
                    expected = [(max(-32768, min(32767, p[0])), max(-32768, min(32767, p[1]))) for p in expected]
                    compare(actual, expected)
                    cases += 1
    print('PASS', label, 'real SDL audio:', cases, 'stream/event FWS/CWS interpreter/JIT cases')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except NotImplementedError as error:
        print('SKIP', error)
        sys.exit(77)

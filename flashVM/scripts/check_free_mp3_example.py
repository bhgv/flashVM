#!/usr/bin/env python3
"""Run the real C89 SWF file exporter against original free-format wire and mpg123 goldens."""
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import zlib


def fixtures():
    """Read checked-in original encoder bytes and independent S16 golden values, never VM output."""
    text = (Path(__file__).resolve().parents[1] / 'tests/swf_free_mp3_fixture.hpp').read_text()
    def array(name):
        return list(map(int, re.findall(r'-?\d+', re.search(r'\b'+name+r'\[\] = \{(.*?)\};', text, re.S)[1])))
    positions = array('positions')
    configs = [(44100, 2, 128), (44100, 1, 400), (48000, 2, 640), (22050, 1, 80)]
    return [(bytes(array('encoded'+str(i))), rate, channels, kbps, positions,
             array('golden'+str(i))) for i, (rate, channels, kbps) in enumerate(configs)]


def tag(code, body=b''):
    """Serialize independent SWF short/long tags, including exact payload extents."""
    return (struct.pack('<H', code*64+len(body)) if len(body)<63 else
            struct.pack('<HI', code*64+63, len(body)))+body


def movie(fixture, group, latency=529, compressed=False, version=8):
    """Use encoder-known frame spans to group packets without invoking the runtime framer."""
    data, rate, channels, kbps, _, _ = fixture
    samples = 1152 if rate >= 32000 else 576
    base = (144000 if rate >= 32000 else 72000)*kbps//rate
    tags = tag(18, b'\x0e\x2e\x01\0'+struct.pack('<h', latency))
    at = 0
    count = 0
    while at < len(data):
        end = at
        packets = 0
        while end < len(data) and packets < group:
            end += base+bool(data[end+2]&2)
            packets += 1
        assert end <= len(data)
        tags += tag(19, struct.pack('<Hh', packets*samples, latency if not at else 37)+data[at:end])+tag(1)
        count += 1
        if not at:
            tags += tag(19, bytes(4))+tag(1)
            count += 1
        at = end
    body = b'\x08\0'+struct.pack('<HH', 12*256, count)+tags+tag(0)
    return (b'CWS' if compressed else b'FWS')+bytes([version])+struct.pack('<I', len(body)+8)+(zlib.compress(body) if compressed else body)


def main():
    """Check real files, effective geometry, latency, sampled native PCM and safe output timing."""
    with tempfile.TemporaryDirectory(prefix='flashvm-free-mp3-') as directory:
        root = Path(directory)
        source, target = root/'source.swf', root/'result.pcm'
        for fixture in fixtures():
            for group in (1, 2, 5):
                for version, compressed in ((4, False), (8, False), (8, True)):
                    source.write_bytes(movie(fixture, group, compressed=compressed, version=version))
                    result = subprocess.run([sys.argv[1], '--decode', str(source), str(target)],
                                            capture_output=True, text=True, check=True)
                    _, rate, channels, _, positions, golden = fixture
                    assert result.stdout == f'Decoded 5231 frames at {rate} Hz, {channels} channels\n', result.stdout
                    raw = target.read_bytes()
                    assert len(raw) == 5231*channels*2
                    pcm = struct.unpack('='+str(len(raw)//2)+'h', raw)
                    for p, position in enumerate(positions):
                        if position < 529:
                            continue
                        for channel in range(channels):
                            assert abs(pcm[(position-529)*channels+channel]-golden[p*channels+channel]) <= 2
        # Invalid source must not replace the selected output before preparation succeeds.
        source.write_bytes(b'not a movie')
        target.write_bytes(b'unchanged')
        result = subprocess.run([sys.argv[1], '--decode', str(source), str(target)], capture_output=True)
        assert result.returncode != 0 and target.read_bytes() == b'unchanged'
    print('PASS C89 free-format export: four encodings, grouped packets, FWS/CWS, detached native PCM')


if __name__ == '__main__':
    main()

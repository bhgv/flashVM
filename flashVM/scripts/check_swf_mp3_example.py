#!/usr/bin/env python3
"""Check the actual C89 MP3 exporter against independent golden vectors and optional full FFmpeg PCM."""
from pathlib import Path
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib


def tag(code, payload=b''):
    """Build independent SWF records with explicit payload bounds."""
    return struct.pack('<HI', code*64+63, len(payload))+payload


def wire(encoded, frames, stream, compressed):
    """One MP3 source with a known 529-frame initial delay and deliberately false rate advice."""
    if stream:
        tags = tag(18, b'\x0e\x2e\x01\0'+struct.pack('<h', 529))
        tags += tag(19, struct.pack('<Hh', frames, 529)+encoded)
    else:
        tags = tag(14, struct.pack('<HBIh', 7, 0x22, frames-529, 529)+encoded)
        tags += tag(15, b'\x07\0\0')
    body = b'\x08\0'+struct.pack('<HH', 12*256, 1)+tags+tag(1)+tag(0)
    return (b'CWS' if compressed else b'FWS')+b'\x08'+struct.pack('<I', len(body)+8)+(zlib.compress(body) if compressed else body)


def main():
    """Exercise both native lifetimes, all MPEG fixture geometries, containers and terminal errors."""
    executable = str(Path(sys.argv[1]).resolve())
    root_source = Path(__file__).resolve().parents[1]
    fixture = (root_source/'tests/swf_mp3_fixture.hpp').read_text()
    encoded = {int(i): bytes(map(int, re.findall(r'\d+', body))) for i, body in
               re.findall(r'encoded(\d+)\[\] = \{(.*?)\};', fixture, re.S)}
    golden = {int(i): list(map(int, re.findall(r'-?\d+', body))) for i, body in
              re.findall(r'golden(\d+)\[\] = \{(.*?)\};', fixture, re.S)}
    positions = list(map(int, re.findall(r'\d+', re.search(r'positions\[\] = \{(.*?)\};', fixture, re.S)[1])))
    records = re.findall(r'\{encoded(\d+), sizeof\(encoded\d+\), (\d+), (\d+), (\d+), golden\d+\}', fixture)
    ffmpeg = shutil.which('ffmpeg')
    with tempfile.TemporaryDirectory(prefix='flashvm-mp3-tutorial-') as directory:
        root = Path(directory)
        subprocess.run([executable], cwd=root, check=True, stdout=subprocess.PIPE)
        assert not list(root.iterdir())
        source, output = root/'input.swf', root/'output.pcm'
        invalid = subprocess.run([executable, str(source), str(output), '--wrong'], capture_output=True)
        assert invalid.returncode and not output.exists()
        for number, rate, channels, frames in records:
            number, rate, channels, frames = map(int, (number, rate, channels, frames))
            expected = None
            if ffmpeg:
                mp3, pcm = root/'oracle.mp3', root/'oracle.pcm'
                mp3.write_bytes(encoded[number])
                subprocess.run([ffmpeg, '-v', 'error', '-f', 'mp3', '-i', str(mp3), '-f', 's16le',
                                '-y', str(pcm)], check=True, capture_output=True)
                all_values = list(struct.unpack('<%dh' % (pcm.stat().st_size//2), pcm.read_bytes()))
                expected = all_values[529*channels:frames*channels]
            for stream in (False, True):
                for compressed in (False, True):
                    source.write_bytes(wire(encoded[number], frames, stream, compressed))
                    output.write_bytes(b'old data' * 20000)
                    command = [executable, str(source), str(output)]+(['--stream'] if stream else [])
                    result = subprocess.run(command, check=True, capture_output=True, text=True)
                    assert 'after VM teardown.' in result.stdout
                    actual = struct.unpack('=%dh' % ((frames-529)*channels), output.read_bytes())
                    for p, position in enumerate(positions):
                        if position >= 529:
                            for channel in range(channels):
                                assert abs(actual[(position-529)*channels+channel]-golden[number][p*channels+channel]) <= 2
                    if expected is not None:
                        assert len(actual) == len(expected) and max(abs(a-b) for a, b in zip(actual, expected)) <= 3
            failed = subprocess.run([executable, str(source), str(root/'missing'/'output.pcm'), '--stream'], capture_output=True)
            assert failed.returncode and not (root/'missing').exists()
        output.unlink()
        source.write_bytes(b'FW')
        failed = subprocess.run([executable, str(source), str(output)], capture_output=True)
        assert failed.returncode and not output.exists()
    print('PASS C89 MP3: six geometries, FWS/CWS, native voice/stream, lifetime, CLI and file errors' +
          ('; full FFmpeg PCM oracle' if ffmpeg else '; checked-in independent golden positions'))


if __name__ == '__main__':
    main()

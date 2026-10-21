#!/usr/bin/env python3
"""Regenerate analytical MP3 fixtures and independent FFmpeg sample oracles.
Requires ffmpeg with libmp3lame. Encoding may vary by tool version; normal tests
use the checked-in vectors and never invoke an encoder or download dependencies.
"""
from pathlib import Path
import argparse
import struct
import subprocess
import tempfile


def array(name, values, ctype, columns):
    """Write a compact C++98 literal array without platform byte-order assumptions."""
    lines = ['static const %s %s[] = {' % (ctype, name)]
    for at in range(0, len(values), columns):
        lines.append('    ' + ', '.join(map(str, values[at:at + columns])) + ',')
    return '\n'.join(lines + ['};', ''])


def main():
    """Encode three MPEG generations, with independent raw decoding and selected golden values."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
                        default=Path(__file__).resolve().parents[1] / 'tests/swf_mp3_fixture.hpp')
    args = parser.parse_args()
    text = ['/* Analytical Layer-III test vectors; generated with FFmpeg/libmp3lame, no third-party media. */',
            '#ifndef FVM_TEST_SWF_MP3_FIXTURE_HPP', '#define FVM_TEST_SWF_MP3_FIXTURE_HPP',
            '#include "flashvm/swf_sound.h"', 'namespace mp3_fixture {']
    positions = [0, 1, 16, 100, 528, 529, 575, 576, 1104, 1105, 1151, 1152, 2047, 3072]
    text.append(array('positions', positions, 'size_t', 14))
    records = []
    with tempfile.TemporaryDirectory(prefix='flashvm-mp3-vectors-') as directory:
        root = Path(directory)
        for index, (rate, channels, bitrate) in enumerate([(11025, 1, '32k'), (22050, 2, '64k'),
                                                           (44100, 2, '128k')]):
            values = [((frame * (71 if channel else 37)) % 24000) - 12000
                      for frame in range(3072) for channel in range(channels)]
            source, encoded, decoded = root/'source.pcm', root/'sound.mp3', root/'oracle.pcm'
            source.write_bytes(struct.pack('<%dh' % len(values), *values))
            subprocess.run(['ffmpeg', '-v', 'error', '-f', 's16le', '-ar', str(rate), '-ac', str(channels),
                            '-i', str(source), '-c:a', 'libmp3lame', '-b:a', bitrate, '-write_xing', '0',
                            '-id3v2_version', '0', '-map_metadata', '-1', '-y', str(encoded)], check=True)
            subprocess.run(['ffmpeg', '-v', 'error', '-f', 'mp3', '-i', str(encoded),
                            '-f', 's16le', '-y', str(decoded)], check=True)
            payload = encoded.read_bytes()
            pcm = struct.unpack('<%dh' % (decoded.stat().st_size // 2), decoded.read_bytes())
            frames = len(pcm) // channels
            assert frames > max(positions)
            golden = [pcm[frame * channels + ch] for frame in positions for ch in range(channels)]
            text.append(array('encoded%d' % index, payload, 'unsigned char', 16))
            text.append(array('golden%d' % index, golden, 'short', 14))
            records.append('    {encoded%d, sizeof(encoded%d), %d, %d, %d, golden%d},' %
                           (index, index, rate, channels, frames, index))
    text.extend(['struct Fixture { const unsigned char *data; size_t size; unsigned rate, channels; '
                 'size_t frames; const short *golden; };', 'static const Fixture fixtures[] = {',
                 *records, '};', '}', '#endif', ''])
    args.output.write_text('\n'.join(text), encoding='utf-8')
    print('Wrote', args.output)


if __name__ == '__main__':
    main()
